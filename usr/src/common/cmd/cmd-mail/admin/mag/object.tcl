#ident "@(#)object.tcl	11.3"
#
# mail administration onscreen object routines.
# main calls a fixed set of entrypoints.
# knowledge of the onscreen_db structure is here
# as well as in the main program.
# knowledge of mainscreen widgets is here as well as in main.
# mainscreen_db structure is doc'ed in main.
#
# object services visible from here are named for Mail Admin Object (mao).
# internal routines are mai (i for internal, synonym for static).
#
#	mao_init() - set onscreen_db to it's initial state and load
#		the drawn list.
#	mao_activate(index) - does the appropriate action, modify, expand,
#		or contract
#	mao_default(index,prompt) - set the object and it's children to
#		default values.  prompt is yes if prompt for container default.
#	mao_select(index) - select the new object, takes care of stippling
#		of options, knows about screen widgets.
#	mao_move(index, direction) - move object up or down.
#	mao_add(index) - add a new child object.
#	mao_delete(index) - delete object and it's children.
#
# called by ma_activate:
#	mai_modify(index) - call edit box for the given property.
#	mai_expand(index) - expand the container given.
#	mai_collapse(index) - collapse the container given.
#
# internal utilities
#	mai_child_length(index) - find number of onscreen children for an index.
#		called by collapse and move.
#	mai_ch_names_get() - get channel names in sorted order
#	mai_ch_find(index) - search back to find channel container from index.
#		return index.
#	mai_prop_find(index,name) - given an open container, find named prop.
#		return index.
#
# internal routines that isolate the back end for properties.
#	mai_get_children(object) - get a list of an object's children
#		in mainscreen_db format.  Only calls back end for containers.
#		For properties it calls mai_prop_get.
#	mai_prop_get(object) - for a property, get the current value from the
#		back end. Returns the new object.
#	mai_prop_set(oldobject,newobject) - saves changes to internal database,
#		may change the object a bit more.
#		Returns the new object, "conflict", or "badname" if new
#		value is rejected.
#	mai_prop_def(object) - for a property,
#		return the new object with the default value in it.
#		For simplicity's sake, objects that do not have defaults,
#		such as channel names and channel types retain
#		their original value.
#
# modify, expand, and compress are all called by the activate object
# event which comes from double click or enter.
#

# after cf_open and ms1_open are complete, this routine builds the
# mainscreen database in it's initial form
#
proc \
mao_init {} \
{
	global mainscreen_db

	set db ""

	set list [list container "" config 0 "" [mag_msg BASIC] closed]
	lappend db $list
	set list [list container "" folder 0 "" [mag_msg FOLDER] closed]
	lappend db $list
	set list [list container "" alias 0 add [mag_msg ALIAS] closed]
	lappend db $list
	set list [list container "" altname 0 add [mag_msg ALTNAME] closed]
	lappend db $list
	set list [list container "" channels 0 add [mag_msg CHANNEL] closed]
	lappend db $list

	set mainscreen_db $db
}

# activate action is the edit action.
proc \
mao_activate { index } \
{
	global mainscreen_db MS

	set list [lindex $mainscreen_db $index]
	set class [lindex $list $MS(class)]
	if {"$class" == "property"} {
		set action [lindex $list $MS(action)]
@if notused
		if {[lsearch $action mod] == -1} {
			return
		}
@endif
		mai_modify $index
		return
	}
@if debug
	if {"$class" != "container"} {
		error "unknown class $class"
	}
@endif
	set state [lindex $list $MS(state)]
	if {"$state" == "closed"} {
		mai_expand $index
		return
	}
	if {"$state" == "open"} {
		mai_collapse $index
		return
	}
	error "unknown state $state"
}

proc \
mao_default { index prompt } \
{
	global mainscreen_db MS MAILALIASES DIRTY SLOCAL

	set list [lindex $mainscreen_db $index]
	set class [lindex $list $MS(class)]

	if {"$class" == "property"} {
		set newlist [mai_prop_def $list]
		if {"$newlist" != "$list"} {
			set DIRTY 1
			set newlist [mai_prop_set $list $newlist]
			set mainscreen_db [lreplace $mainscreen_db \
				$index $index $newlist]
			# check other state changes
			mai_state $list $newlist $index
			# redraw main screen
			set stop [expr $index + 1]
			mag_display_mainlist $index $stop $index
		}
		return
	}
	set type [lindex $list $MS(type)]
	set state [lindex $list $MS(state)]

	if {"$prompt" == "yes"} {
		VtUnLock
		set ret [mag_query_qyn CONT_DEFAULT ""]
		VtLock
		if {"$ret" == "no"} {
			return
		}
	}

	set DIRTY 1
	# default all elements inside a container
	switch $type {
	"config" {
		if {"$state" == "open"} {
			mai_collapse $index
		}
		set children [mai_get_children $list]
		foreach child $children {
			set list [mai_prop_def $child]
			mai_prop_set $child $list
		}
		if {"$state" == "open"} {
			mai_expand $index
			return
		}
		return
	}
	"folder" {
		# same code as "config"
		if {"$state" == "open"} {
			mai_collapse $index
		}
		set children [mai_get_children $list]
		foreach child $children {
			set list [mai_prop_def $child]
			mai_prop_set $child $list
		}
		if {"$state" == "open"} {
			mai_expand $index
			return
		}
		return
	}
	"alias" {
		# the list of alias names must change

		# collapse if needed
		if {"$state" == "open"} {
			mai_collapse $index
		}
		ma_aliases_set $MAILALIASES
		if {"$state" == "open"} {
			mai_expand $index
		}
		return
	}
	"altname" {
		# the list of alternate names must change

		# collapse if needed
		if {"$state" == "open"} {
			mai_collapse $index
		}
		ma_alternate_names_set ""
		if {"$state" == "open"} {
			mai_expand $index
		}
		return
	}
	"channels" {
		# delete all channels and add back in the default ones.
		# collapse if needed
		if {"$state" == "open"} {
			mai_collapse $index
		}
		set children [mai_get_children $list]
		foreach child $children {
			set chname [lindex $child $MS(name)]
			ma_ch_delete $chname
		}
		ma_ch_create local
			ma_ch_sequence_set local 0
			ma_ch_equate_set local P $SLOCAL
			ma_ch_table_type_set local local
		ma_ch_create SMTP
			ma_ch_sequence_set SMTP 1
			ma_ch_equate_set SMTP P "\[IPC\]"
			ma_ch_table_type_set SMTP DNS
		ma_ch_create badhost
			ma_ch_sequence_set badhost 2
			ma_ch_equate_set badhost P "\[IPC\]"
			ma_ch_table_type_set badhost remote
		# set the channel props to defaults for new children
		set children [mai_get_children $list]
		foreach child $children {
			# get props for this channel
			set tmpchildren [mai_get_children $child]
			foreach tmpchild $tmpchildren {
				set tmplist [mai_prop_def $tmpchild]
				mai_prop_set $tmpchild $tmplist
			}
		}
		# don't preserve individual channel expansions as they might
		# not be there any more.
		if {"$state" == "open"} {
			mai_expand $index
		}
		return
	}
	"channel" {
		if {"$state" == "open"} {
			mai_collapse $index
		}
		# get props for this channel
		set children [mai_get_children $list]
		foreach child $children {
			set tmplist [mai_prop_def $child]
			mai_prop_set $child $tmplist
		}
		if {"$state" == "open"} {
			mai_expand $index
		}
		return
	}
	}
}

#
# the motif front end will always call select before activate on double click.
proc \
mao_select { index } \
{
	global mainscreen_db MS
	global btn_mod btn_add btn_del btn_up btn_dn
	global menu_mod menu_add menu_del menu_up menu_dn

	set list [lindex $mainscreen_db $index]
	set actions [lindex $list $MS(action)]

	set add FALSE
	set del FALSE
	set mod FALSE
	set mov FALSE

	foreach i $actions {
		if {"$i" == "mod"} {
			set mod TRUE
			continue
		}
		if {"$i" == "add"} {
			set add TRUE
			continue
		}
		if {"$i" == "mov"} {
			set mov TRUE
			continue
		}
		if {"$i" == "del"} {
			set del TRUE
			continue
		}
		error "unknown action $i"
	}
	if {[VtInfo -charm] == 0} {
		VtSetSensitive $btn_mod $mod
		VtSetSensitive $btn_add $add
		VtSetSensitive $btn_up $mov
		VtSetSensitive $btn_dn $mov
		VtSetSensitive $btn_del $del
	}

	VtSetSensitive $menu_mod $mod
	VtSetSensitive $menu_add $add
	VtSetSensitive $menu_up $mov
	VtSetSensitive $menu_dn $mov
	VtSetSensitive $menu_del $del
}

proc \
mao_move { index direction } \
{
	global mainscreen_db MS DIRTY

	# find number of children of current object
	set childcount [mai_child_length $index]
	# total length of current object
	set length [expr $childcount + 1]
	# length of mainlist
	set mainlen [llength $mainscreen_db]

	# our current object
	set list [lindex $mainscreen_db $index]
	set level [lindex $list $MS(level)]

	# index of last predecessor - 1
	set start [expr $index - 1]
	set stop 0
	set newlevel 0
	while {$start >= $stop} {
		set newlist [lindex $mainscreen_db $start]
		set newlevel [lindex $newlist $MS(level)]
		if {$newlevel <= $level} {
			break
		}
		set start [expr $start - 1]
	}
	if {$newlevel == $level} {
		set prev $start
	} else {
		set prev $index
	}

	# index of successor + 1
	set start [expr $index + 1]
	set stop $mainlen
	set newlevel 0
	while {$start < $stop} {
		set newlist [lindex $mainscreen_db $start]
		set newlevel [lindex $newlist $MS(level)]
		if {$newlevel <= $level} {
			break
		}
		set start [expr $start + 1]
	}
	if {$newlevel == $level} {
		set nextobj $start
		set next $start
		# open object check
		set class [lindex $newlist $MS(class)]
		if {"$class" == "container"} {
			set state [lindex $newlist $MS(state)]
			if {"$state" == "open"} {
				set tmpchildren [mai_get_children $newlist]
				set tmplength [llength $tmpchildren]
				set next [expr $next + $tmplength]
			}
		}
	} else {
		set next $index
	}

	# check if no-op
	if {"$direction" == "up" && $prev == $index} {
		return
	}
	if {"$direction" == "down"} {
		if {$next == $index} {
			return
		}
		# now check if next cannot be moved (baduser check)
		set action [lindex $newlist $MS(action)]
		if {[lsearch $action mov] == -1} {
			return
		}
	}

	set DIRTY 1
	# now fix internal database
	# can move three types, alias maps, alternate names, and channels
	set class [lindex $list $MS(class)]
	if {"$class" == "container"} {
		set type channel
	} else {
		set type [lindex $list $MS(name)]
	}
	# swap list with other
	if {"$direction" == "up"} {
		set other [lindex $mainscreen_db $prev]
	} else {
		set other [lindex $mainscreen_db $nextobj]
	}
	switch "$type" {
	"channel" {
		set chname1 [lindex $list $MS(name)]
		set chname2 [lindex $other $MS(name)]
		set order1 [ma_ch_sequence_get $chname1]
		set order2 [ma_ch_sequence_get $chname2]
		ma_ch_sequence_set $chname1 $order2
		ma_ch_sequence_set $chname2 $order1
	}
	"altname" {
		set alts [ma_alternate_names_get]
		set alt1 [lindex $list $MS(value)]
		set alt2 [lindex $other $MS(value)]
		set order1 [lsearch $alts $alt1]
		set order2 [lsearch $alts $alt2]
		set alts [lreplace $alts $order1 $order1 $alt2]
		set alts [lreplace $alts $order2 $order2 $alt1]
		ma_alternate_names_set $alts
	}
	"alias" {
		set aliases [ma_aliases_get]
		set alias1 [lindex $list $MS(value)]
		set alias2 [lindex $other $MS(value)]
		set order1 [lsearch $aliases $alias1]
		set order2 [lsearch $aliases $alias2]
		set aliases [lreplace $aliases $order1 $order1 $alias2]
		set aliases [lreplace $aliases $order2 $order2 $alias1]
		ma_aliases_set $aliases
	}
	}

	# ready to do mainscreen_db and main_list
	# first delete this item, both from mainscreen_db and main_list.
	set start $index
	set stop [expr $index + $childcount]
	set tmplist [lrange $mainscreen_db $start $stop]
	set mainscreen_db [lreplace $mainscreen_db $start $stop]
	set minus1 [expr $index - 1]
	mag_delete_mainlist $minus1 $length
	mag_display_mainlist $minus1 $index -1

	# update next index
	set next [expr $next - $length]

	# ready to insert at the appropriate location
	if {"$direction" == "up"} {
		# insert mainscreen_db before this index
		set insert $prev
	}
	if {"$direction" == "down"} {
		# insert mainscreen_db before this index
		set insert [expr $next + 1]
	}
	set pos $insert
	loop i 0 $length {
		set insertlist [lindex $tmplist $i]
		set mainscreen_db [linsert $mainscreen_db $pos $insertlist]
		set pos [expr $pos + 1]
	}
	# put new inserted data on screen
	set newstart [expr $insert - 1]
	mag_insert_mainlist $newstart $length
	# must redraw previous object through end of mainlist to get lines right
	mag_display_mainlist $start $mainlen $insert
}

proc \
mao_add { index } \
{
	global mainscreen_db DIRTY MS

	# we can add three types, alias maps, alternate names, and channels
	#
	# for maps and alt names our strategy is to create an empty
	# object onscreen named "new" and call mai_modify.  If mai_modify
	# returns yes, we keep the object otherwise we delete it.
	#
	# alias and altname are similar enough to share code,
	# channels are special.

	set container [lindex $mainscreen_db $index]
	set type [lindex $container $MS(type)]

	# new channel
	if {"$type" == "channels"} {
		mag_channel_create $index
		return
	}

	# common code for new alias or altername name

	# collapse onscreen container
	mai_collapse $index

	# add new item to top of list
	if {"$type" == "alias"} {
		set list [ma_aliases_get]
		set list "[mag_msg NEW] $list"
		ma_aliases_set $list
	}
	if {"$type" == "altname"} {
		set list [ma_alternate_names_get]
		set list "[mag_msg NEW] $list"
		ma_alternate_names_set $list
	}

	# make it visible
	mai_expand $index

	# select it
	set start [expr $index + 1]
	mag_display_mainlist $start $start $start

	# call mai_modify
	set ret [mai_modify $start]

	# keep the new entry
	if {"$ret" == "yes"} {
		set DIRTY 1
		return
	}

	# delete the new entry
	mao_delete $start

	# select the container
	mag_display_mainlist $index $index $index
}

proc \
mao_delete { index } \
{
	global mainscreen_db MS DIRTY

	set DIRTY 1
	# can delete three types, alias maps, alternate names, and channels

	set list [lindex $mainscreen_db $index]
	set class [lindex $list $MS(class)]
	set childcount 0
	if {"$class" == "container"} {
		set state [lindex $list $MS(state)]
		if {"$state" == "open"} {
			set children [mai_get_children $list]
			set childcount [llength $children]
		}
	}

	# first delete from the database
	if {"$class" == "container"} {
		set type channel
	} else {
		set type [lindex $list $MS(name)]
	}
	switch "$type" {
	"channel" {
		set chname [lindex $list $MS(name)]
		set channels [mai_ch_names_get]
		set order 0
		# resequence channels
		foreach i $channels {
			if {"$i" == "$chname"} {
				ma_ch_delete $chname
				continue
			}
			ma_ch_sequence_set $i $order
			set order [expr $order + 1]
		}
	}
	"altname" {
		set alts [ma_alternate_names_get]
		set alt [lindex $list $MS(value)]
		set order [lsearch $alts $alt]
		set alts [lreplace $alts $order $order]
		ma_alternate_names_set $alts
	}
	"alias" {
		set aliases [ma_aliases_get]
		set alias [lindex $list $MS(value)]
		set order [lsearch $aliases $alias]
		set aliases [lreplace $aliases $order $order]
		ma_aliases_set $aliases
	}
	}

	# now delete from the screen
	set length [expr $childcount + 1]
	set stop [expr $index + $childcount]
	set mainscreen_db [lreplace $mainscreen_db $index $stop]
	set start [expr $index - 1]
	mag_delete_mainlist $start $length
	# just reset current selected item
	set length [llength $mainscreen_db]
	if {$length == $index} {
		set select $start
	} else {
		set select $index
	}
	mag_display_mainlist $start [expr $start + 1] $select
	# check if current container needs to be redrawn,
	# last item is channel attribute
	set list [lindex $mainscreen_db $select]
	if {$select == $start && [lindex $list $MS(level)] == 2} {
		set start [mai_ch_find $select]
		mag_display_mainlist $start [expr $select + 1] -1
	}
}

proc \
mai_expand { index } \
{
	global mainscreen_db MS

	set list [lindex $mainscreen_db $index]
	set children [mai_get_children $list]

	# set open state
	set list [lreplace $list $MS(state) $MS(state) open]
	set mainscreen_db [lreplace $mainscreen_db $index $index $list]

	# redraw open folder before insert for appropriate visual effect
	set stop [expr $index + 1]
	mag_display_mainlist $index $stop -1

	# add to mainscreen_db
	set length [llength $mainscreen_db]
	set insert [expr $index + 1]
	# yes we can "insert" after the end of a list
	set length [llength $children]
	loop i 0 $length {
		set tmplist [lindex $children $i]
		set mainscreen_db [linsert $mainscreen_db $insert $tmplist]
		set insert [expr $insert + 1]
		if {$i == 0} {
			# redraw container to add line
			set stop [expr $index + 1]
			mag_display_mainlist $index $stop -1
		}
	}
	# update screen
	mag_insert_mainlist $index $length
	# reset selected item
	mag_display_mainlist $index $index $index
}

proc \
mai_collapse { index } \
{
	global mainscreen_db MS

	# find length of collapse operation
	set length [mai_child_length $index]
	if {$length > 0} {
		set start [expr $index + 1]
		set stop [expr $start + $length - 1]
		set mainscreen_db [lreplace $mainscreen_db $start $stop]
		mag_delete_mainlist $index $length
	}
	# set closed state
	set list [lindex $mainscreen_db $index]
	set list [lreplace $list $MS(state) $MS(state) closed]
	set mainscreen_db [lreplace $mainscreen_db $index $index $list]
	# redraw container
	set stop [expr $index + 1]
	mag_display_mainlist $index $stop $index
}

proc \
mai_child_length { index } \
{
	global mainscreen_db MS

	set list [lindex $mainscreen_db $index]
	set length 0
	set level [lindex $list $MS(level)]
	set start [expr $index + 1]
	set stop [llength $mainscreen_db]
	while {$start < $stop} {
		set list [lindex $mainscreen_db $start]
		set newlevel [lindex $list $MS(level)]
		if {$newlevel <= $level} {
			break
		}
		set start [expr $start + 1]
		set length [expr $length + 1]
	}
	return $length
}

proc \
mai_get_children { object } \
{
	global mainscreen_db MS
	global MAILSPOOL INBOXNAME

	set list $object
	set class [lindex $list $MS(class)]
	set type [lindex $list $MS(type)]

	set children ""
	switch $type {
	"config" {
		set child [list property host fqdn 1 mod [mag_msg HOST] ""]
		set child [mai_prop_get $child]
		lappend children $child

		set child [list property from fromdom 1 mod [mag_msg FROM] "" ""]
		set child [mai_prop_get $child]
		lappend children $child

		set child [list property domain boolean 1 mod \
			[mag_msg DTABLE] ""]
		set child [mai_prop_get $child]
		lappend children $child

		set child [list property dtable domtable 1 mod \
			[mag_msg DTABLE_FILE] ""]
		set child [mai_prop_get $child]
		lappend children $child
	}
	"folder" {
		# folder location
		set enum [list [mag_msg FOLDER_SYSTEM] [mag_msg FOLDER_HOME] \
			[mag_msg FOLDER_CUSTOM]]
		set child [list property flocation enum 1 mod \
			[mag_msg FOLDER_LOCATION] "" $enum ""]
		set child [mai_prop_get $child]
		lappend children $child
		# folder format
		set enum [list [mag_msg FOLDER_SENDMAIL] [mag_msg FOLDER_MMDF]]
		set child [list property fformat enum 1 mod \
			[mag_msg FOLDER_FORMAT] "" $enum]
		set child [mai_prop_get $child]
		lappend children $child
		# folder sync
		set child [list property fsync boolean 1 mod \
			[mag_msg FOLDER_FSYNC] ""]
		set child [mai_prop_get $child]
		lappend children $child
		# folder checks
		set child [list property fccheck boolean 1 mod \
			[mag_msg FOLDER_CCHECK] ""]
		set child [mai_prop_get $child]
		lappend children $child
		# folders incore
		set child [list property fincore boolean 1 mod \
			[mag_msg FOLDER_INCORE] ""]
		set child [mai_prop_get $child]
		lappend children $child
		# expunge threshold
		set range [list 0 100]
		set child [list property fthreshold intrange 1 mod \
			[mag_msg FOLDER_THRESHOLD] "" $range]
		set child [mai_prop_get $child]
		lappend children $child
		# lock timeout
		set range [list 1 999]
		set child [list property ftimeout intrange 1 mod \
			[mag_msg FOLDER_TIMEOUT] "" $range]
		set child [mai_prop_get $child]
		lappend children $child
		# file based locking
		set child [list property ffilelock boolean 1 mod \
			[mag_msg FOLDER_FILELOCK] ""]
		set child [mai_prop_get $child]
		lappend children $child
		# umask
		set child [list property fumask umask 1 mod \
			[mag_msg FOLDER_UMASK] ""]
		set child [mai_prop_get $child]
		lappend children $child
	}
	"alias" {
		set aliases [ma_aliases_get]
		set action [list mov del mod]
		foreach i $aliases {
			set test [csubstr $i 0 4]
			if {"$test" == "nis:"} {
				set label [mag_msg MAP]
			} else {
				set label [mag_msg FILE]
			}
			set child [list property alias alias 1 $action \
				$label $i]
			lappend children $child
		}
	}
	"altname" {
		set alternates [ma_alternate_names_get]
		set action [list mov del mod]
		foreach i $alternates {
			set child [list property altname fqdn 1 $action \
				[mag_msg ALTERNATE] $i]
			lappend children $child
		}
	}
	"channels" {
		set channels [mai_ch_names_get]
		# default action list
		set action1 [list mov del]
		# bad user channel does not have "mov" action
		set action2 [list del]
		foreach i $channels {
			set chtable [ma_ch_table_type_get $i]
			if {"$chtable" != "baduser"} {
				set action $action1
			} else {
				set action $action2
			}
			set child [list container $i channel 1 $action \
				"" closed]
			set child [mai_prop_get $child]
			lappend children $child
		}
	}
	"channel" {
		set chname [lindex $list $MS(name)]
		# channel name
		set child [list property cname rstring 2 mod \
			[mag_msg CH_NAME] "" "" $chname]
		set child [mai_prop_get $child]
		lappend children $child
		# program name
		set child [list property cprogram chpath 2 mod \
			[mag_msg CH_PROGRAM] "" "" $chname]
		set child [mai_prop_get $child]
		lappend children $child
		# program arguments
		set child [list property cargs string 2 mod \
			[mag_msg CH_ARGS] "" "" $chname]
		set child [mai_prop_get $child]
		lappend children $child
		# channel program directory
		set child [list property cdir path 2 mod \
			[mag_msg CH_PROGDIR] "" "" $chname]
		set child [mai_prop_get $child]
		lappend children $child
		# channel table type
		set enum [mag_table_enum]
		set child [list property ctable enum 2 mod \
			[mag_msg CH_TABLE] "" $enum $chname]
		set child [mai_prop_get $child]
		lappend children $child
		# channel file
		set child [list property cfile chtable 2 mod \
			[mag_msg CH_FILE] "" "" $chname]
		set child [mai_prop_get $child]
		lappend children $child
		# channel host
		set child [list property chost ofqdn 2 mod \
			[mag_msg CH_HOST] "" "" $chname]
		set child [mai_prop_get $child]
		lappend children $child
		# channel flags
		set child [list property cflags flags 2 mod \
			[mag_msg CH_FLAGS] "" "" $chname]
		set child [mai_prop_get $child]
		lappend children $child
		# channel EOL
		set child [list property ceol rstring 2 mod \
			[mag_msg CH_EOL] "" "" $chname]
		set child [mai_prop_get $child]
		lappend children $child
		# channel max msg size
		set range [list 0 2000000000]
		set child [list property cmaxmsg ointrange 2 mod \
			[mag_msg CH_MAXMSG] "" $range $chname]
		set child [mai_prop_get $child]
		lappend children $child
		# channel max line size
		set range [list 0 10000]
		set child [list property cmaxline ointrange 2 mod \
			[mag_msg CH_MAXLINE] "" $range $chname]
		set child [mai_prop_get $child]
		lappend children $child
		# channel program nice value
		set range [list -20 20]
		set child [list property cnice intrange 2 mod \
			[mag_msg CH_NICE] "" $range $chname]
		set child [mai_prop_get $child]
		lappend children $child
		# channel user
		set child [list property cuser user 2 mod \
			[mag_msg CH_USER] "" "" $chname]
		set child [mai_prop_get $child]
		lappend children $child
		# channel group
		set child [list property cgroup group 2 mod \
			[mag_msg CH_GROUP] "" "" $chname]
		set child [mai_prop_get $child]
		lappend children $child
		# recipient envelope ruleset
		set child [list property crecip ruleset 2 mod \
			[mag_msg CH_RECIP] "" "" $chname]
		set child [mai_prop_get $child]
		lappend children $child
		# recipient header ruleset
		set child [list property chrecip ruleset 2 mod \
			[mag_msg CH_HRECIP] "" "" $chname]
		set child [mai_prop_get $child]
		lappend children $child
		# sender envelope ruleset
		set child [list property csender ruleset 2 mod \
			[mag_msg CH_SENDER] "" "" $chname]
		set child [mai_prop_get $child]
		lappend children $child
		# sender header ruleset
		set child [list property chsender ruleset 2 mod \
			[mag_msg CH_HSENDER] "" "" $chname]
		set child [mai_prop_get $child]
		lappend children $child
	}
	}
	return $children
}

proc \
mai_ch_names_get {} \
{
	set list [ma_ch_names_get]
	set order ""
	foreach i $list {
		lappend order [ma_ch_sequence_get $i]
	}
	set sorted ""
	set length [llength $list]
	loop i 0 $length {
		# find the appropriate one in order
		loop j 0 $length {
			set item [lindex $order $j]
			if {$item == $i} {
				lappend sorted [lindex $list $j]
				break
			}
		}
	}
	return $sorted
}

proc \
mai_prop_def { object } \
{
	global mainscreen_db MS
	global MAILSPOOL INBOXNAME MAILTABLE MQUEUE UUX
	global MDOMAINDB
	global mag_host_name

	set list $object
	set class [lindex $list $MS(class)]
	set name [lindex $list $MS(name)]

@if debug
	if {"$class" == "container"} {
		error "Cannot get default value for a container"
	}
@endif

	switch $name {
	"host" {
		set value $mag_host_name
	}
	"from" {
		set value $mag_host_name
	}
	"domain" {
		set value [mag_num_to_bool 0]
	}
	"dtable" {
		# does nothing, keep original value
		set value [lindex $list $MS(value)]
	}
	}

	# folder properties
	switch $name {
	"flocation" {
		set value [mag_msg FOLDER_SYSTEM]
		set custom [list $MAILSPOOL ""]
		set list [lreplace $list $MS(custom) $MS(custom) $custom]
	}
	"fformat" {
		set value [mag_msg FOLDER_SENDMAIL]
	}
	"fsync" {
		set value [mag_num_to_bool 0]
	}
	"fccheck" {
		set value [mag_num_to_bool 0]
	}
	"fincore" {
		set value [mag_num_to_bool 0]
	}
	"fthreshold" {
		set value 50
	}
	"ftimeout" {
		set value 10
	}
	"ffilelock" {
		set value [mag_num_to_bool 0]
	}
	"fumask" {
		set value "077"
	}
	}

	# alias and altname properties
	switch $name {
	"alias" {
		set value [lindex $list $MS(value)]
	}
	"altname" {
		set value [lindex $list $MS(value)]
	}
	}

	# if a channel property, get channel name from backlink
	if {[catch {set tmp $value}] != 0} {
		set chname [lindex $list $MS(chname)]

		set chprogram [ma_ch_equate_get $chname P]
		if {"$chprogram" == "\[IPC\]"} {
			set chprogram SMTP
		}
		set chtable [ma_ch_table_type_get $chname]
	}

	# channel properties with no defaults
	switch $name {
	"cname" {
		set value [lindex $list $MS(value)]
	}
	"cprogram" {
		set value $chprogram
	}
	"ctable" {
		set value [mag_table_to_external $chtable]
	}
	}

	# channel properties with defaults based upon the above values
	switch $name {
	"cargs" {
		set prog $chprogram
		# default value
		set base [exec basename $prog]
		set value "$base \$u"
		# special cases
		switch $base {
		"SMTP" {
			set value "IPC \$h"
		}
		"uux" {
			set value "uux - -r -a\$f -gmedium \$h!rmail (\$u)"
		}
		"slocal" {
			set value "slocal \$u"
		}
		}
	}
	"cdir" {
		set value $MQUEUE
	}
	"cfile" {
		# note that the file is not renamed
		if {"[exec basename $chprogram]" == "multihome"} {
			set value $MDOMAINDB
		} elseif {"$chtable" == "file"} {
			set value "$MAILTABLE/$chname"
		} else {
			set value ""
		}
	}
	"chost" {
		set value [mag_msg NONE]
	}
	"cflags" {
		# custom same as SMTP
		set value "mlsDFMPeu8"
		switch [exec basename $chprogram] {
		"SMTP" {
			set value "mlsDFMPeu8"
		}
		"uux" {
			set value "mDFMhuU8"
		}
		"slocal" {
			set value "lsDFMPhoAw5:|/@8"
		}
		}
	}
	"ceol" {
		if {"$chprogram" == "SMTP"} {
			set value "\\r\\n"
		} else {
			set value "\\n"
		}
	}
	"cmaxmsg" {
		if {"[exec basename $chprogram]" == "uux"} {
			set value 100000
		} else {
			#set value [mag_msg NONE]
			set value 20000000
		}
	}
	"cmaxline" {
		if {"$chprogram" == "SMTP"} {
			set value 990
		} else {
			set value [mag_msg NONE]
		}
	}
	"cnice" {
		set value 0
	}
	"cuser" {
		set value [mag_msg DEFAULT]
	}
	"cgroup" {
		set value [mag_msg DEFAULT]
	}
	"crecip" {
		set value ap822_re
		switch [exec basename $chprogram] {
		"SMTP" {
			set value ap822_re
		}
		"uux" {
			set value ap976_re
		}
		"slocal" {
			set value aplocal_re
		}
		}
	}
	"chrecip" {
		set value ap822_rh
		switch [exec basename $chprogram] {
		"SMTP" {
			set value ap822_rh
		}
		"uux" {
			set value ap976_rh
		}
		"slocal" {
			set value aplocal_rh
		}
		}
	}
	"csender" {
		set value ap822_se
		switch [exec basename $chprogram] {
		"SMTP" {
			set value ap822_se
		}
		"uux" {
			set value ap976_se
		}
		"slocal" {
			set value aplocal_se
		}
		}
	}
	"chsender" {
		set value ap822_sh
		switch [exec basename $chprogram] {
		"SMTP" {
			set value ap822_sh
		}
		"uux" {
			set value ap976_sh
		}
		"slocal" {
			set value aplocal_sh
		}
		}
	}
	}
@if debug
	if {[catch {set tmp $value}] != 0} {
		error "Unknown object <$object>"
	}
@endif
	set list [lreplace $list $MS(value) $MS(value) $value]
	return $list
}

proc \
mai_prop_get { object } \
{
	global mainscreen_db MS mag_host_name
	global MAILSPOOL INBOXNAME MAILTABLE MQUEUE

	set list $object
	set class [lindex $list $MS(class)]
	set name [lindex $list $MS(name)]
	set type [lindex $list $MS(type)]

	switch $name {
	"host" {
		set value [ma_machine_name_get]
		if {"$value" == ""} {
			set value $mag_host_name
		}
	}
	"from" {
		set value [ma_from_domain_get]
		if {"$value" == ""} {
			set value $mag_host_name
		}
	}
	"domain" {
		set value [mag_num_to_bool [ma_domain_table_enabled_get]]
	}
	"dtable" {
		set value [ma_domain_table_file_get]
	}
	}

	# folder properties
	switch $name {
	"flocation" {
		set dir [ma_ms1_get MS1_INBOX_DIR]
		set name [ma_ms1_get MS1_INBOX_NAME]
		set value [mag_msg FOLDER_CUSTOM]
		if {"$dir" == "$MAILSPOOL" && "$name" == ""} {
			set value [mag_msg FOLDER_SYSTEM]
		}
		if {"$dir" == "" && "$name" == "$INBOXNAME"} {
			set value [mag_msg FOLDER_HOME]
		}
		set tmplist [list $dir $name]
		set list [lreplace $list \
			$MS(custom) $MS(custom) $tmplist]
	}
	"fformat" {
		set tmp [ma_ms1_get MS1_FOLDER_FORMAT]
		if {"$tmp" == "Sendmail"} {
			set value [mag_msg FOLDER_SENDMAIL]
		}
		if {"$tmp" == "MMDF"} {
			set value [mag_msg FOLDER_MMDF]
		}
	}
	"fsync" {
		set value [mag_string_to_bool [ma_ms1_get MS1_FSYNC]]
	}
	"fccheck" {
		set value [mag_string_to_bool [ma_ms1_get MS1_EXTENDED_CHECKS]]
	}
	"fincore" {
		set value [mag_string_to_bool [ma_ms1_get MS1_FOLDERS_INCORE]]
	}
	"fthreshold" {
		set value [ma_ms1_get MS1_EXPUNGE_THRESHOLD]
	}
	"ftimeout" {
		set value [ma_ms1_get MS1_LOCK_TIMEOUT]
	}
	"ffilelock" {
		set value [mag_string_to_bool [ma_ms1_get MS1_FILE_LOCKING]]
	}
	"fumask" {
		set value [ma_ms1_get MS1_UMASK]
	}
	}

	if {"$type" == "channel"} {
		# a channel container, special work here
		# here the label rather than the value is changed
		set chname [lindex $list $MS(name)]
		set label [mag_channel_to_alias $chname]
		set list [lreplace $list $MS(label) $MS(label) $label]
		return $list
	}

	# is a channel property, get chname
	set chname [lindex $list $MS(chname)]

	switch $name {
	"cname" {
		set value [mag_channel_to_alias $chname]
	}
	"cprogram" {
		set value [ma_ch_equate_get $chname P]
		if {"$value" == "\[IPC\]"} {
			set value SMTP
		}
	}
	"cargs" {
		set value [ma_ch_equate_get $chname A]
	}
	"cdir" {
		set value [ma_ch_equate_get $chname D]
		if {"$value" == ""} {
			set value $MQUEUE
		}
	}
	"ctable" {
		set value [ma_ch_table_type_get $chname]
		set value [mag_table_to_external $value]
	}
	"cfile" {
		set value [ma_ch_table_file_get $chname]
	}
	"chost" {
		set value [ma_ch_host_get $chname]
		if {"$value" == ""} {
			set value [mag_msg NONE]
		}
	}
	"cflags" {
		set value [ma_ch_equate_get $chname F]
	}
	"ceol" {
		set value [ma_ch_equate_get $chname E]
		if {"$value" == ""} {
			set value "\\n"
		}
	}
	"cmaxmsg" {
		set value [ma_ch_equate_get $chname M]
		if {"$value" == ""} {
			set value [mag_msg NONE]
		}
	}
	"cmaxline" {
		set value [ma_ch_equate_get $chname L]
		if {"$value" == ""} {
			set value [mag_msg NONE]
		}
	}
	"cnice" {
		set value [ma_ch_equate_get $chname N]
		if {"$value" == ""} {
			set value 0
		}
	}
	"cuser" {
		set both [ma_ch_equate_get $chname U]
		if {"$both" == ""} {
			set value [mag_msg DEFAULT]
		} else {
			set tmplist [split $both ":"]
			set value [lindex $tmplist 0]
		}
	}
	"cgroup" {
		set both [ma_ch_equate_get $chname U]
		if {"$both" == ""} {
			set value [mag_msg DEFAULT]
		} else {
			set tmplist [split $both ":"]
			set value [lindex $tmplist 1]
			if {"$value" == ""} {
				set value [mag_msg DEFAULT]
			}
		}
	}
	"crecip" {
		set recip [ma_ch_equate_get $chname R]
		set reciplist [split $recip "/"]
		set value [lindex $reciplist 0]
		if {"$value" == "" || "$value" == "0"} {
			set value [mag_msg NONE]
		}
	}
	"chrecip" {
		set recip [ma_ch_equate_get $chname R]
		set reciplist [split $recip "/"]
		set value [lindex $reciplist 1]
		if {"$value" == "" || "$value" == "0"} {
			set value [mag_msg RULEDEF]
		}
	}
	"csender" {
		set sender [ma_ch_equate_get $chname S]
		set senderlist [split $sender "/"]
		set value [lindex $senderlist 0]
		if {"$value" == "" || "$value" == "0"} {
			set value [mag_msg NONE]
		}
	}
	"chsender" {
		set sender [ma_ch_equate_get $chname S]
		set senderlist [split $sender "/"]
		set value [lindex $senderlist 1]
		if {"$value" == "" || "$value" == "0"} {
			set value [mag_msg RULEDEF]
		}
	}
	}
@if debug
	if {[catch {set tmp $value}] != 0} {
		error "Unknown object <$object>"
	}
@endif
	set list [lreplace $list $MS(value) $MS(value) $value]
	return $list
}

proc \
mai_prop_set { oldobject newobject } \
{
	global mainscreen_db MS mag_host_name
	global MAILSPOOL INBOXNAME MAILTABLE MQUEUE

	set list $newobject
	set class [lindex $list $MS(class)]
	set name [lindex $list $MS(name)]
	set value [lindex $list $MS(value)]

	switch $name {
	"host" {
		if {"$value" == "$mag_host_name"} {
			set value ""
		}
		ma_machine_name_set $value
		set set ok
	}
	"from" {
		if {"$value" == "$mag_host_name"} {
			set value ""
		}
		ma_from_domain_set $value
		set set ok
	}
	"domain" {
		ma_domain_table_enabled_set [mag_bool_to_num $value]
		set set ok
	}
	"dtable" {
		# does nothing
		set set ok
	}
	}

	# have to remove old name, put in new name, preserve order
	# no check for name conflicts yet
	if {"$name" == "alias" || "$name" == "altname"} {
		set oldname [lindex $oldobject $MS(value)]
		set newname $value
		# get old list
		switch "$name" {
		"alias" {
			set tmplist [ma_aliases_get]
		}
		"altname" {
			set tmplist [ma_alternate_names_get]
		}
		}
		# replace item
		set index [lsearch $tmplist $oldname]
		set tmplist [lreplace $tmplist $index $index $newname]
		switch "$name" {
		"alias" {
			set tmplist [ma_aliases_set $tmplist]
		}
		"altname" {
			set tmplist [ma_alternate_names_set $tmplist]
		}
		}
		set set ok
	}

	# folder properties
	switch $name {
	"flocation" {
		if {"$value" == "[mag_msg FOLDER_SYSTEM]"} {
			ma_ms1_set MS1_INBOX_DIR $MAILSPOOL
			ma_ms1_set MS1_INBOX_NAME ""
			set set ok
		}
		if {"$value" == "[mag_msg FOLDER_HOME]"} {
			ma_ms1_set MS1_INBOX_DIR ""
			ma_ms1_set MS1_INBOX_NAME $INBOXNAME
			set set ok
		}
		# use custom spot for custom values
		if {"$value" == "[mag_msg FOLDER_CUSTOM]"} {
			set tmplist [lindex $list $MS(custom)]
			ma_ms1_set MS1_INBOX_DIR [lindex $tmplist 0]
			ma_ms1_set MS1_INBOX_NAME [lindex $tmplist 1]
			set set ok
		}
	}
	"fformat" {
		if {"$value" == "[mag_msg FOLDER_SENDMAIL]"} {
			ma_ms1_set MS1_FOLDER_FORMAT Sendmail
			set set ok
		}
		if {"$value" == "[mag_msg FOLDER_MMDF]"} {
			ma_ms1_set MS1_FOLDER_FORMAT MMDF
			set set ok
		}
	}
	"fsync" {
		ma_ms1_set MS1_FSYNC [mag_bool_to_string $value]
		set set ok
	}
	"fccheck" {
		ma_ms1_set MS1_EXTENDED_CHECKS [mag_bool_to_string $value]
		set set ok
	}
	"fincore" {
		ma_ms1_set MS1_FOLDERS_INCORE [mag_bool_to_string $value]
		set set ok
	}
	"fthreshold" {
		ma_ms1_set MS1_EXPUNGE_THRESHOLD $value
		set set ok
	}
	"ftimeout" {
		ma_ms1_set MS1_LOCK_TIMEOUT $value
		set set ok
	}
	"ffilelock" {
		ma_ms1_set MS1_FILE_LOCKING [mag_bool_to_string $value]
		set set ok
	}
	"fumask" {
		ma_ms1_set MS1_UMASK $value
		set set ok
	}
	}

	# is a channel property, get chname
	set chname [lindex $list $MS(chname)]

	switch $name {
	"cname" {
		set newvalue [mag_channel_to_internal $value]
		set oldvalue [lindex $oldobject $MS(chname)]
		# rename channel
		set set [ma_ch_rename $oldvalue $newvalue]
		if {"$set" != "ok"} {
			# return error code if one.
			set newobject $set
		} else {
			# set new name in channel-name object
			# the alias has already been set
			set newobject [lreplace $newobject \
				$MS(chname) $MS(chname) $newvalue]
		}
	}
	"cprogram" {
		if {"$value" == "SMTP"} {
			set value "\[IPC\]"
		}
		ma_ch_equate_set $chname P $value
		set set ok
	}
	"cargs" {
		ma_ch_equate_set $chname A $value
		set set ok
	}
	"cdir" {
		if {"$value" == "$MQUEUE"} {
			set value ""
		}
		ma_ch_equate_set $chname D $value
		set set ok
	}
	"ctable" {
		ma_ch_table_type_set $chname [mag_table_to_internal $value]
		set set ok
	}
	"cfile" {
		ma_ch_table_file_set $chname $value
		set set ok
	}
	"chost" {
		if {"$value" == "[mag_msg NONE]"} {
			set value ""
		}
		ma_ch_host_set $chname $value
		set set ok
	}
	"cflags" {
		ma_ch_equate_set $chname F $value
		set set ok
	}
	"ceol" {
		if {"$value" == "\\n"} {
			set value ""
		}
		ma_ch_equate_set $chname E $value
		set set ok
	}
	"cmaxmsg" {
		if {"$value" == "[mag_msg NONE]"} {
			set value ""
		}
		ma_ch_equate_set $chname M $value
		set set ok
	}
	"cmaxline" {
		if {"$value" == "[mag_msg NONE]"} {
			set value ""
		}
		ma_ch_equate_set $chname L $value
		set set ok
	}
	"cnice" {
		if {$value == 0} {
			set value ""
		}
		ma_ch_equate_set $chname N $value
		set set ok
	}
	"cuser" {
		if {"$value" == "[mag_msg DEFAULT]"} {
			set value ""
		}
		set both [ma_ch_equate_get $chname U]
		set tmplist [split $both ":"]
		set user [lindex $tmplist 0]
		set group [lindex $tmplist 1]
		if {"$value" == ""} {
			set group ""
		}
		if {"$group" != ""} {
			set both "$value:$group"
		} else {
			set both $value
		}
		ma_ch_equate_set $chname U $both
		set set ok
	}
	"cgroup" {
		if {"$value" == "[mag_msg DEFAULT]"} {
			set value ""
		}
		set both [ma_ch_equate_get $chname U]
		set tmplist [split $both ":"]
		set user [lindex $tmplist 0]
		set group [lindex $tmplist 1]
		# assume user is set first
		if {"$value" != ""} {
			set both "$user:$value"
		} else {
			set both $user
		}
		ma_ch_equate_set $chname U $both
		set set ok
	}
	"crecip" {
		set recip [ma_ch_equate_get $chname R]
		set reciplist [split $recip "/"]
		set env [lindex $reciplist 0]
		set hdr [lindex $reciplist 1]

		set env $value
		if {"$value" == "[mag_msg NONE]"} {
			set env ""
		}
		set value [mag_build_ruleset $env $hdr]
		ma_ch_equate_set $chname R $value
		set set ok
	}
	"chrecip" {
		set recip [ma_ch_equate_get $chname R]
		set reciplist [split $recip "/"]
		set env [lindex $reciplist 0]
		set hdr [lindex $reciplist 1]

		set hdr $value
		if {"$value" == "[mag_msg RULEDEF]"} {
			set hdr ""
		}
		set value [mag_build_ruleset $env $hdr]
		ma_ch_equate_set $chname R $value
		set set ok
	}
	"csender" {
		set sender [ma_ch_equate_get $chname S]
		set senderlist [split $sender "/"]
		set env [lindex $senderlist 0]
		set hdr [lindex $senderlist 1]

		set env $value
		if {"$value" == "[mag_msg NONE]"} {
			set env ""
		}
		set value [mag_build_ruleset $env $hdr]
		ma_ch_equate_set $chname S $value
		set set ok
	}
	"chsender" {
		set sender [ma_ch_equate_get $chname S]
		set senderlist [split $sender "/"]
		set env [lindex $senderlist 0]
		set hdr [lindex $senderlist 1]

		set hdr $value
		if {"$value" == "[mag_msg RULEDEF]"} {
			set hdr ""
		}
		set value [mag_build_ruleset $env $hdr]
		ma_ch_equate_set $chname S $value
		set set ok
	}
	}
@if debug
	if {[catch {set tmp $set}] != 0} {
		error "Unknown object <$newobject>"
	}
@endif
	return $newobject
}

proc \
mai_ch_find { index } \
{
	global mainscreen_db MS

	loop i $index 0 -1 {
		set list [lindex $mainscreen_db $i]
		set class [lindex $list $MS(class)]
		if {"$class" == "container"} {
			return $i
		}
	}
@if debug
	error "Nofind container"
@endif
}

proc \
mai_prop_find { index name } \
{
	global mainscreen_db MS

	set start [expr $index + 1]
	set stop [llength $mainscreen_db]
	loop i $start $stop {
		set list [lindex $mainscreen_db $i]
		set class [lindex $list $MS(class)]
@if debug
		if {"$class" == "container"} {
			error "Nofind property"
		}
@endif
		set tmpname [lindex $list $MS(name)]
		if {"$tmpname" == "$name"} {
			return $i
		}
	}
@if debug
	error "Nofind property"
@endif
}
