#ident "@(#)main.tcl	11.1"
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
# mail administration. Stuff for main screen goes here.
#

# app globals go here

# version number
set versionNumber [SaGetSSOVersion]

# resource path
set RESPATH /etc/mail/admin/px
# our pid
set PID [pid]
# our path since we call ourself via exec
set ME "/etc/mail/admin/mailadmin"

# the files we are editting
set MAILDEF /etc/default/mail
set MAILDEFTMP /tmp/mail$PID
set MAILDEFDEF /etc/mail/mail.factory
set MAILCF /etc/sendmail.cf
set MAILCFTMP /tmp/sendmail.cf$PID
set MAILCFDEF /etc/mail/sendmailcf.factory
set UUCP /usr/lib/uucp/Systems
set UUCPDB /etc/mail/table/uucp.db
# mail tables are normally kept in
set MAILTABLE /etc/mail/table
# default alias file
set MAILALIASES /etc/mail/aliases

# file editors
set ALIAS_EDITOR /etc/mail/admin/aliases
set CTABLE_EDITOR /etc/mail/admin/channel
set DTABLE_EDITOR /etc/mail/admin/domain

set MAKEMAP /etc/mail/makemap
set SENDMAILPID /etc/sendmail.pid

# slocal path
set SLOCAL /etc/mail/slocal
# uux path
set UUX /usr/bin/uux
# multihome mailer path
set MULTIHOME /etc/mail/multihome
set MDOMAINDB /var/internet/ip/127.0.0.1/mail/virtdomains
# mqueue path
set MQUEUE /var/spool/mqueue

# the system mail spool directory
set MAILSPOOL /var/mail
# the default mailbox name
set INBOXNAME .mailbox
set mag_iconinit 0
# main list length inits to zero
set main_list_length 0

set RESOLVER /usr/sbin/host
# our managed host starts as local
set mag_host_name [mag_host_local]
set mag_local_host $mag_host_name


# this exceptionally long comment serves as a partial design document
# for this program.
#
# mainscreen database (structure) definition
#
# to get a hierarchical drawn list we implement an onscreen database
# that contains the onscreen view, as objects are expanded.
# data is fetched from the ma_* api, as objects are contracted
# the onscreen copy is trashed as the ma_* api is updated as things are
# changed.
#
# a tcl list where each list item contains the following entries:
#
# class:	container or property
# name:		name of object, used primarily for properties.
#		If more than one of a container type exists then
#		the name differentiates them.
# type:		special types for each container, plus generic
#		data types for use by properties.
# level:	indent level, object containment is inferred from this value.
# action:	Objects have optional actions, this list enumerates them.
#		The corresponding menu item is unstippled.
#		add does not imply delete.
#		actions are strings in a list.
# label:	The left hand side of the display text.
#
#	properties only:
# value:	The right hand side of the display text, properties only.
# criteria:	If the type needs additional data such as minimum and maximum
#		values for integers it goes here and is type dependent.
#	containers only:
# state:	containers only, folder open or closed (affects icon).
#
#
# Items are displayed in the drawn list in the order they are in the db.
# Initially only containers are displayed (maybe the first one should be open).
#
# mainscreen_db subscript (MS) definitions:
set MS(class) 0
set MS(name) 1
set MS(type) 2
set MS(level) 3
set MS(action) 4
set MS(label) 5
set MS(value) 6
set MS(state) 6
set MS(criteria) 7
set MS(chname) 8
set MS(custom) 8

# here is a complete list of all of our containers and their values
# note that to uniquely identify object, an it's class, name, and type
# are required.  For simplicity we will just pass around the index
# of the object in the mainscreen_db list.
#
# class      name    type     level action   label  state
# container   *      config     0    ""        -    closed
# container   *      folder     0    ""        -    closed
# container   *      alias      0    add       -    closed
# container   *      altname    0    add       -    closed
# container   *      channels   0    add       -    closed
# container  chname  channel    1    mov/del calias closed
#
# note than some channel names
# are aliased to internationalized values:
#	Internal name		English Internationalized name
#	"local"			Local
#	"badhost"		Badhost
#	"baduser"		Baduser
#	"multihome"		Multihome
#
# for english channel names, the capitalization is the only difference.
#
# here is a complete list of all of our properties and their values
#
# config properties
#
# class    name       type     level action label value criteria
# property host       fqdn       1    mod     -     -
# property from       fromdom    1    mod     -     -   {built in mai_pre}
# property domain     boolean    1    mod     -     -
# property dtable     domtable   1    mod     -     -
#
# folder properties
# class    name       type     level action label value criteria
# property flocation  enum       1    mod     -     -	{list from IntlMessage}
#	custom entry contains directory and filename.
# property fformat    enum       1    mod     -     -	{list from IntlMessage}
# property fsync      boolean    1    mod     -     -
# property fccheck    boolean    1    mod     -     -
# property fincore    boolean    1    mod     -     -
# property fthreshold intrange   1    mod     -     -   {0 100}
# property ftimeout   intrange   1    mod     -     -   {1 999}
# property ffilelock  boolean    1    mod     -     -
# property fumask     umask      1    mod     -     -
#
# alias properties
# class    name       type     level action     label value criteria
# property alias      alias      1   mov/del/mod  -     -
#
# altname properties
# class    name       type     level action     label value criteria
# property altname    fqdn       1   mov/del/mod  -     -
#
# channel properties
# in addition, each channel property contains the channel name (chname)
# which acts as a backlink to the channel container for get/set operations
# class    name       type     level action label value criteria
# property cname      rstring    2    mod     -     -
# property cprogram   chpath     2    mod     -     -
# property cargs      string     2    mod     -     -
# property cdir       path       2    mod     -     -
# property ctable     enum       2    mod     -     -   {list from IntlMessage}
# property cfile      chtable    2    mod     -     -
# property chost      ofqdn      2    mod     -     -
# property cflags     flags      2    mod     -     -
# property ceol       rstring    2    mod     -     -
# property cmaxmsg    ointrange  2    mod     -     -   {0 2000000000}
# property cmaxline   ointrange  2    mod     -     -   {0 10000}
# property cnice      intrange   2    mod     -     -   {-20 20}
# property cuser      user       2    mod     -     -
# property cgroup     group      2    mod     -     -
# property crecip     ruleset    2    mod     -     -
# property chrecip    ruleset    2    mod     -     -
# property csender    ruleset    2    mod     -     -
# property chsender   ruleset    2    mod     -     -
#
# valid actions are: mod add mov del
# more than one can be preset in the action list
#
# Key:
#	-  value is determined by configuration data or i18n strings.
#	*  item is ignored for this combination of other values.
#
# for each object type a raft of services need to be provided:
#
# stippling of actions as the object is selected.
# simple data types (inside a switch statement):
#	enum - enumerated legal values
#	fqdn - fully qualified domain name (DNS edit box).
#	ofqdn - optional fqdn <none> is legal value.
#	fromdom - from domain full length name is based on host name.
#	boolean - Yes or No value.
#	intrange - ranged integer, Criteria min and max values inclusive.
#	ointrange - optional ranged integer, is an integer or NONE.
#		Criteria min and max values inclusive.
#	rstring - token name (no spaces), cannot be null.
#	string - string, can be null.
#	path - path to a file, warning is generated if the file does not exist.
#	chtable - channel table, like path but also has edit feature.
#	chpath - choose from known channel program names or your own path.
#	domtable - short cut to allow editting of the domain table.
#		The path name is not configurable, but it is visible.
#	
# complex data types:
#	alias - alias file or NIS map entry.
#	flags - the whole list of F= Sendmail flags.
#	user - a defined username or number.
#	group - a defined groupname or number.
#	ruleset - one of our known ruleset strings, or a positive integer (0-n).
#
# object services are mapped one-to-one with GUI events.  The following
#	events are defined:
#
# init:
#	set onscreen database to it's initial state.
# default:
#	set an object and it's children to default values.
#	children are deleted and restored so only default
#	children are present.
# select:
#	select an object, primarily stippling of menus is done.
# activate:
#	does the activate action on that object which is one of:
#	expand:		closed containers are expanded.
#	collapse:	open containers are collapsed.
#	modify:		properties are opened for modify (dialog box).
# move:
#	move a movable object up or down in it's hierarchy.
#	open and closed state of an object is maintained.
#	The moved object remains selected, as the drawn list widget
#	is updated.
# 

#
# given an index in mainscreen_db, return a list that is the
# fieldlist and formatlist respectively to call VtDrawnList routines with.
#
proc \
mag_display_item { index } \
{
	global main_list mainscreen_db MS

	set list [lindex $mainscreen_db $index]
	set class [lindex $list $MS(class)]
	set name [lindex $list $MS(name)]
	set type [lindex $list $MS(type)]
	set level [lindex $list $MS(level)]
	set action [lindex $list $MS(action)]
	set label [lindex $list $MS(label)]
	if {"$class" == "container"} {
		set container 1
		set state [lindex $list $MS(state)]
		if {"$state" == "open"} {
			set container 2
		}
	} else {
		set container 0
		set value [lindex $list $MS(value)]
		if {[llength $list] == 8} {
			set criteria [lindex $list $MS(criteria)]
		} else {
			set criteria ""
		}
	}

	# make iconlist and formatlist
	#
	# to do this we need to check at each level if
	# equal or higher levels of indents occur for each
	# level we are already at, simulating a parent-child
	# relationship for each level, only two levels supported here.

	# any more indent level one children?
	set more1children 0
	set length [llength $mainscreen_db]
	loop j [expr $index + 1] $length {
		set nextlist [lindex $mainscreen_db $j]
		set nextlevel [lindex $nextlist $MS(level)]
		if {$nextlevel == 2} {
			continue
		}
		if {$nextlevel == 0} {
			break
		}
		if {$nextlevel == 1} {
			set more1children 1
			break
		}
		error "Unknown indent level 1 $nextlevel"
	}
	set more2children 0
	loop j [expr $index + 1] $length {
		set nextlist [lindex $mainscreen_db $j]
		set nextlevel [lindex $nextlist $MS(level)]
		if {$nextlevel == 1} {
			break
		}
		if {$nextlevel == 0} {
			break
		}
		if {$nextlevel == 2} {
			set more2children 1
			break
		}
		error "Unknown indent level 2 $nextlevel"
	}

	# now we index a table of icons to draw using the values
	# container, level, more1children and more2children.

	set icons [mag_get_icons $container $level \
		$more1children $more2children]

	# now build formatlist and fieldlist
	set formatlist ""
	set fieldlist ""
	foreach j $icons {
		# all our icons are two chars wide
		lappend formatlist "ICON 1"
		lappend fieldlist $j
	}
	if {"$class" == "container"} {
		lappend formatlist "STRING 75"
		lappend fieldlist $label
	} else {
		lappend formatlist "STRING 46"
		lappend fieldlist $label:
		set len [clength $value]
		set len [expr $len + 2]
		lappend formatlist "STRING $len"
		lappend fieldlist $value
	}
	return [list $fieldlist $formatlist]
}

#
# function that syncs up what is different between the drawn
# list and the mainscreen_db.
# Start is the lowest entry in the list that changed
# stop is highest entry in list that changed plus 1, if stop is zero
# then end of list is used.
# Index is new selected item or -1 for don't call VtDrawnListSelect.
#	no select feature allows this routine to be
#	called multiple times with the select on the last call,
#	to avoid the selection bar jumping around.
#
proc \
mag_display_mainlist { start stop index } \
{
	global main_list mainscreen_db MS

	set length [llength $mainscreen_db]

	if {$stop == 0} {
		set stop $length
	}

	loop i $start $stop {
		set list [mag_display_item $i]
		set fieldlist [lindex $list 0]
		set formatlist [lindex $list 1]

		set position [expr $i + 1]
		VtDrawnListSetItem $main_list \
			-position $position \
			-fieldList $fieldlist \
			-formatList $formatlist
	}
	if {$index >= 0} {
		set select [expr $index + 1]
		VtDrawnListSelectItem $main_list -position $select
		mao_select $index
	}
}

# assumes that $index exists in main list
# actually the index points to the container, items are inserted after that
# container can be -1 for initial list creation
proc \
mag_insert_mainlist { index count } \
{
	global main_list

	loop i 0 $count {
		set item [expr $index + $i + 1]
		set list [mag_display_item $item]
		set fieldlist [lindex $list 0]
		set formatlist [lindex $list 1]

		set position [expr $index + $i + 2]
		VtDrawnListAddItem $main_list -position $position \
			-fieldList $fieldlist -formatList $formatlist
	}
}

# assumes that $index exists in main list
# actually the index points to the container, items are deleted after that
proc \
mag_delete_mainlist { index count } \
{
	global main_list

	set delete [expr $index + 2]
	set deletelist ""
	loop i 0 $count {
		lappend deletelist [expr $delete + $i]
	}
	VtDrawnListDeleteItem $main_list -positionList $deletelist
}

#
# Nobody will ever be able to change this with out drawing
# out what every icon is and does, if the below names could be symbolic
# it would help a lot but the DrawnList widget wants indexes.
# see the drawn list creation line for the pixmap files which attempt
# to symbolically name the icons.
#
# the array is keyed by each of our four variables as "0" or "1" or "2"
# strung together in the calling order to make a string.
#
# arguments are:
#		object is a container: 0 or 1 (closed) or 2 (open)
#		current indent level: 0, 1, or 2
#		more children at level 1: 0 or 1
#		more children at level 2: 0 or 1
#
proc \
mag_get_icons { con in c1 c2 } \
{
	global mag_iconarray mag_iconinit

	if {$mag_iconinit == 0} {
		# last two bits are don't care for these,
		# but we have to set them all.
		set mag_iconarray("0000") 0
		set mag_iconarray("0001") 0
		set mag_iconarray("0010") 0
		set mag_iconarray("0011") 0

		# last bit is don't care for these
		set mag_iconarray("1000") 1
		set mag_iconarray("1001") 1

		set mag_iconarray("2000") 9
		set mag_iconarray("2001") 9

		set mag_iconarray("1010") 2
		set mag_iconarray("1011") 2

		set mag_iconarray("2010") 10
		set mag_iconarray("2011") 10

		set mag_iconarray("0100") [list 5 8]
		set mag_iconarray("0101") [list 5 8]

		set mag_iconarray("0110") [list 6 8]
		set mag_iconarray("0111") [list 6 8]

		# all bits are significant for the rest

		# containers
		set mag_iconarray("1100") [list 5 3]
		set mag_iconarray("1101") [list 5 4]
		set mag_iconarray("1110") [list 6 3]
		set mag_iconarray("1111") [list 6 4]

		set mag_iconarray("2100") [list 5 11]
		set mag_iconarray("2101") [list 5 12]
		set mag_iconarray("2110") [list 6 11]
		set mag_iconarray("2111") [list 6 12]

		# simple objects
		set mag_iconarray("0200") [list 0 5 8]
		set mag_iconarray("0201") [list 0 6 8]
		set mag_iconarray("0210") [list 7 5 8]
		set mag_iconarray("0211") [list 7 6 8]

		set mag_iconinit 1
	}
	set key "$con$in$c1$c2"
	set icons $mag_iconarray("$key")
	return $icons
}

# add our widgets
proc \
mag_build_widgets { parent } \
{
	global RESPATH main_list status_line
	global btn_mod btn_add btn_del btn_up btn_dn
	global menu_mod menu_add menu_del menu_up menu_dn
	global mag_host_name mag_host_label

	# menubar
	set menuBar [VtMenuBar $parent.menubar \
		-helpMenuItemList [SaHelpGetOptionsList]]

	set hostMenu [VtPulldown $menuBar.hostMenu -label [mag_msg MENU_HOST] \
		-mnemonic [mag_msg MENU_HOST_MN]]
	VtPushButton $hostMenu.open -label [mag_msg MENU_OPEN] \
		-mnemonic [mag_msg MENU_OPEN_MN] \
		-callback mag_open_host_cb \
		-shortHelpString [mag_msg SHORT_OPEN] \
		-shortHelpCallback mag_short_help
	VtPushButton $hostMenu.exit -label [mag_msg MENU_EXIT] \
		-mnemonic [mag_msg MENU_EXIT_MN] \
		-callback "quit_cb 1"\
		-shortHelpString [mag_msg SHORT_EXIT] \
		-shortHelpCallback mag_short_help

	set editMenu [VtPulldown $menuBar.editMenu -label [mag_msg MENU_EDIT] \
		-mnemonic [mag_msg MENU_EDIT_MN]]
	set menu_mod [VtPushButton $editMenu.Mod -label [mag_msg MENU_MOD] \
		-mnemonic [mag_msg MENU_MOD_MN] \
		-callback mag_modify_cb \
		-shortHelpString [mag_msg SHORT_MOD] \
		-shortHelpCallback mag_short_help]
	VtSeparator $editMenu.sep1
	set menu_add [VtPushButton $editMenu.Add -label [mag_msg MENU_ADD] \
		-mnemonic [mag_msg MENU_ADD_MN] \
		-callback mag_add_cb \
		-shortHelpString [mag_msg SHORT_ADD] \
		-shortHelpCallback mag_short_help]
	set menu_del [VtPushButton $editMenu.del -label [mag_msg MENU_DEL] \
		-mnemonic [mag_msg MENU_DEL_MN] \
		-callback mag_delete_cb \
		-shortHelpString [mag_msg SHORT_DEL] \
		-shortHelpCallback mag_short_help]
	VtSeparator $editMenu.sep2
	set menu_up [VtPushButton $editMenu.up -label [mag_msg MENU_UP] \
		-mnemonic [mag_msg MENU_UP_MN] \
		-callback mag_up_cb \
		-shortHelpString [mag_msg SHORT_UP] \
		-shortHelpCallback mag_short_help]
	set menu_dn [VtPushButton $editMenu.dn -label [mag_msg MENU_DN] \
		-mnemonic [mag_msg MENU_DN_MN] \
		-callback mag_down_cb \
		-shortHelpString [mag_msg SHORT_DN] \
		-shortHelpCallback mag_short_help]

	set setMenu [VtPulldown $menuBar.setMenu -label [mag_msg MENU_SET] \
		-mnemonic [mag_msg MENU_SET_MN]]
	VtPushButton $setMenu.def -label [mag_msg MENU_DEF] \
		-mnemonic [mag_msg MENU_DEF_MN] \
		-callback mag_default_cb \
		-shortHelpString [mag_msg SHORT_DEF] \
		-shortHelpCallback mag_short_help

	VtPushButton $setMenu.badhost -label [mag_msg MENU_BADHOST] \
		-mnemonic [mag_msg MENU_BADHOST_MN] \
		-callback mag_badhost_cb \
		-shortHelpString [mag_msg SHORT_BADHOST] \
		-shortHelpCallback mag_short_help

	VtPushButton $setMenu.baduser -label [mag_msg MENU_BADUSER] \
		-mnemonic [mag_msg MENU_BADUSER_MN] \
		-callback mag_baduser_cb \
		-shortHelpString [mag_msg SHORT_BADUSER] \
		-shortHelpCallback mag_short_help

	VtPushButton $setMenu.uucp -label [mag_msg MENU_UUCP] \
		-mnemonic [mag_msg MENU_UUCP_MN] \
		-callback mag_uucp_cb \
		-shortHelpString [mag_msg SHORT_UUCP] \
		-shortHelpCallback mag_short_help

	VtPushButton $setMenu.mhome -label [mag_msg MENU_MHOME] \
		-mnemonic [mag_msg MENU_MHOME_MN] \
		-callback mag_mhome_cb \
		-shortHelpString [mag_msg SHORT_MHOME] \
		-shortHelpCallback mag_short_help

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
		set btn_mod [VtPushButton $toolBar.mod \
			-label [mag_msg MENU_MOD] \
			-leftSide FORM \
			-topSide FORM \
			-bottomSide FORM \
			-callback mag_modify_cb \
			-shortHelpString [mag_msg SHORT_MOD] \
			-shortHelpCallback mag_short_help \
		]
		set tmp1 [VtLabel $toolBar.sep1 \
			-label "  " \
			-leftSide $btn_mod \
			-topSide FORM \
		]
		set btn_add [VtPushButton $toolBar.add \
			-label [mag_msg MENU_ADD] \
			-leftSide $tmp1 \
			-topSide FORM \
			-bottomSide FORM \
			-callback mag_add_cb \
			-shortHelpString [mag_msg SHORT_ADD] \
			-shortHelpCallback mag_short_help \
		]
		set btn_del [VtPushButton $toolBar.del \
			-label [mag_msg MENU_DEL] \
			-leftSide $btn_add \
			-topSide FORM \
			-bottomSide FORM \
			-callback mag_delete_cb \
			-shortHelpString [mag_msg SHORT_DEL] \
			-shortHelpCallback mag_short_help \
		]
		set tmp2 [VtLabel $toolBar.sep2 \
			-label "  " \
			-leftSide $btn_del \
			-topSide FORM \
		]
		set btn_up [VtPushButton $toolBar.up -pixmap $RESPATH/prev.px \
			-callback mag_up_cb \
			-leftSide $tmp2 \
			-topSide FORM \
			-shortHelpString [mag_msg SHORT_UP] \
			-shortHelpCallback mag_short_help \
		]
		set btn_dn [VtPushButton $toolBar.dn -pixmap $RESPATH/next.px \
			-callback mag_down_cb \
			-leftSide $btn_up \
			-topSide FORM \
			-rightSide NONE \
			-shortHelpString [mag_msg SHORT_DN] \
			-shortHelpCallback mag_short_help \
		]
	}

	# Top status line
	set mag_host_label [VtLabel $parent.toplab \
		-leftSide FORM -rightSide FORM \
		-label [mag_msg1 TITLE $mag_host_name] \
	]

	# list widget
	set main_list [VtDrawnList $parent.list -columns 78 -rows 16 \
		-autoSelect TRUE -selection BROWSE \
		-callback mag_select_cb \
		-defaultCallback mag_activate_cb \
		-leftSide FORM -rightSide FORM \
		-horizontalScrollBar 1 \
		-iconList [list \
			$RESPATH/blank.px \
			$RESPATH/folder_alone.px \
			$RESPATH/folder_has.px \
			$RESPATH/folder_next_alone.px \
			$RESPATH/folder_next_has.px \
			$RESPATH/item_last.px \
			$RESPATH/item_middle.px \
			$RESPATH/vbar.px \
			$RESPATH/hbar.px \
			$RESPATH/folder_open_alone.px \
			$RESPATH/folder_open_has.px \
			$RESPATH/folder_open_next_alone.px \
			$RESPATH/folder_open_next_has.px \
		]]

	# status line
	set status_line [SaStatusBar $parent.status 1]
	SaStatusBarSet $status_line [mag_msg LOADING]

	VtSetValues $main_list -bottomSide $status_line
}

proc \
mag_open_host_cb { cbs } \
{
	VtLock
	mag_open_host
	VtUnLock
}

proc \
mag_select_cb { cbs } \
{
	VtLock
	set index [keylget cbs itemPosition]
	set index [expr $index - 1]

	mao_select $index
	VtUnLock
}

proc \
mag_activate_cb { cbs } \
{
	VtLock
	set index [keylget cbs itemPosition]
	set index [expr $index - 1]

	mao_activate $index
	VtUnLock
}

proc \
mag_default_cb { cbs } \
{
	global main_list

	VtLock
	set index [VtDrawnListGetSelectedItem $main_list]
	set index [expr $index - 1]

	mao_default $index yes
	VtUnLock
}

proc \
mag_modify_cb { cbs } \
{
	global main_list

	VtLock
	set index [VtDrawnListGetSelectedItem $main_list]
	set index [expr $index - 1]

	mao_activate $index
	VtUnLock
}

proc \
mag_add_cb { cbs } \
{
	global main_list

	VtLock
	set index [VtDrawnListGetSelectedItem $main_list]
	set index [expr $index - 1]

	mao_add $index
	VtUnLock
}

proc \
mag_delete_cb { cbs } \
{
	global main_list

	VtLock
	set index [VtDrawnListGetSelectedItem $main_list]
	set index [expr $index - 1]

	mao_delete $index
	VtUnLock
}

proc \
mag_up_cb { cbs } \
{
	global main_list

	VtLock
	set index [VtDrawnListGetSelectedItem $main_list]
	set index [expr $index - 1]

	mao_move $index up
	VtUnLock
}

proc \
mag_down_cb { cbs } \
{
	global main_list

	VtLock
	set index [VtDrawnListGetSelectedItem $main_list]
	set index [expr $index - 1]

	mao_move $index down
	VtUnLock
}

proc \
mag_badhost_cb { cbs } \
{
	VtLock
	mao_badhost
	VtUnLock
}

proc \
mag_baduser_cb { cbs } \
{
	VtLock
	mao_baduser
	VtUnLock
}

proc \
mag_uucp_cb { cbs } \
{
	VtLock
	mao_uucp
	VtUnLock
}

proc \
mag_mhome_cb { cbs } \
{
	VtLock
	mao_mhome
	VtUnLock
}

proc \
mag_short_help { cbs } \
{
	global status_line

	set helpstring [keylget cbs helpString]
	SaStatusBarSet $status_line $helpstring
}

# get /etc/sendmail.cf or a factory default version into /tmp
# set dirty if factory default file used.
# returns ok, fail, or no (user said no).
proc \
mag_cf_get {} \
{
	global MAILCF MAILCFTMP MAILCFDEF DIRTY
@if test
	global TEST
@endif

	system "rm -fr $MAILCFTMP"
	set ret [mag_remote_copyin $MAILCF $MAILCFTMP]
	if {$ret != "ok"} {
		VtUnLock
		set ret [mag_query_eyn CF_COPY_OUT $MAILCF]
		VtLock
		if {$ret == "no"} {
			return no
		}
		# restore factory file
		VtUnLock
		set ret [mag_local_copy $MAILCFDEF $MAILCFTMP]
		VtLock
		if {$ret != "ok"} {
			VtUnLock
			mag_error1 CF_FACTORY_RESTORE $MAILCF
			VtLock
			return fail
		}
		set DIRTY 1
	} else {
		# validate old file
		if {[ma_cf_valid $MAILCFTMP] == "fail"} {
			# invalid old file
			VtUnLock
			set ret [mag_query_eyn CF_INVALID $MAILCF]
			VtLock
			if {$ret == "no"} {
				return no
			}
		}
	}
	# ready to attempt open.
	set ret start
	while {"$ret" != "ok"} {
		set ret [ma_cf_open $MAILCFTMP]
@if test
			if {"$TEST" == "main_test6"} {
				set ret fail
			}
@endif
		switch $ret {
		"ok"	{
		}
		"fail"	{
			VtUnLock
			mag_error1 CF_OPEN_FAIL $MAILCFTMP
			VtLock
			return fail
		}
		"parserr" {
			VtUnLock
			set ret [mag_query_eyn CF_PARSE $MAILCF]
			VtLock
			if {$ret == "no"} {
				return no
			}
			# restore factory file
			set ret [mag_local_copy $MAILCFDEF $MAILCFTMP]
			if {$ret != "ok"} {
				VtUnLock
				mag_error1 CF_FACTORY_RESTORE $MAILCF
				VtLock
				return fail
			}
			set ret retry
			set DIRTY 1
		}
		}
	}
	return ok
}

# get /etc/default/mail or a factory default version into /tmp
# set dirty if factory default file used.
# returns ok, fail, or no (user said no).
proc \
mag_ms1_get {} \
{
	global MAILDEF MAILDEFTMP MAILDEFDEF DIRTY
@if test
	global TEST
@endif

	system "rm -fr $MAILDEFTMP"
	set ret [mag_remote_copyin $MAILDEF $MAILDEFTMP]
	if {$ret != "ok"} {
		VtUnLock
		set ret [mag_query_eyn CF_COPY_OUT $MAILDEF]
		VtLock
		if {$ret == "no"} {
			return no
		}
		# restore factory file
		set ret [mag_local_copy $MAILDEFDEF $MAILDEFTMP]
		if {$ret != "ok"} {
			VtUnLock
			mag_error1 CF_FACTORY_RESTORE $MAILDEF
			VtLock
			return fail
		}
		set DIRTY 1
	}
	# ready to attempt open.
	set ret start
	while {"$ret" != "ok"} {
		set ret [ma_ms1_open $MAILDEFTMP]
@if test
		if {"$TEST" == "main_test12"} {
			set ret fail
		}
@endif
		switch $ret {
		"ok" 	{
		}
		"fail"	{
			VtUnLock
			mag_error1 CF_OPEN_FAIL $MAILDEFTMP
			VtLock
			return fail
		}
		"parserr" {
			VtUnLock
			set ret [mag_query_eyn MS1_PARSE $MAILDEF]
			VtLock
			if {$ret == "yes"} {
				# restore factory file
				set ret [mag_local_copy $MAILDEFDEF $MAILDEFTMP]
				if {$ret != "ok"} {
					VtUnLock
					mag_error1 CF_FACTORY_RESTORE $MAILDEF
					VtLock
					return fail
				}
				# to re-load the new MAILDEFTMP
				set ret "parserr"
			} else {
				set ret "ok"
			}
			set DIRTY 1
		}
		}
	}
	return ok
}

#
# write out current stuff and close
# rebuild UUCP map if needed
# stop and restart sendmail
# return ok, fail, or cancel.
#
proc \
mag_all_put {} \
{
	global app DIRTY
	global MAILCF MAILCFTMP MAILDEF MAILDEFTMP

	set saved 0
	if {$DIRTY == 1} {
		VtUnLock
		set ret [mag_query_qync DIRTY notused]
		VtLock
		if {"$ret" == "cancel"} {
			return cancel
		}
		if {"$ret" == "yes"} {
			set ret [ma_cf_write]
			if {"$ret" != "ok"} {
				VtUnLock
				mag_query_eok CF_WRITE $MAILCFTMP
				VtLock
				return fail
			}
			set ret [mag_remote_copyout $MAILCFTMP $MAILCF]
			if {"$ret" != "ok"} {
				VtUnLock
				mag_query_eok COPY_BACK $MAILCF
				VtLock
				return fail
			}

			set ret [ma_ms1_write]
			if {"$ret" != "ok"} {
				VtUnLock
				mag_query_eok CF_WRITE $MAILDEFTMP
				VtLock
				return fail
			}
			set ret [mag_remote_copyout $MAILDEFTMP $MAILDEF]
			if {"$ret" != "ok"} {
				VtUnLock
				mag_query_eok COPY_BACK $MAILDEF
				VtLock
				return fail
			}
			set saved 1
		}
		set DIRTY 0
	}
	mag_rebuild_uucp_map yes
	if {"$saved" == 1} {
		mag_restart_sendmail yes
	}
	return ok
}

proc \
mag_remoteCommand {host cmd args errMsgId} \
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
mag_authorized {host} \
{
	set uid [id effective userid]

	if {$uid != 0} {
		set cmd "/sbin/tfadmin"
		set args1 "-t mailadmin"
		set args2 "-t cpfile"
		set args3 "-t kill"
		if {[ErrorCatch errStack 0 {mag_remoteCommand $host \
			$cmd $args1 SCO_MAIL_ADMIN_ERR_TFADMIN} ret1] || \
		    [ErrorCatch errStack 0 {mag_remoteCommand $host \
			$cmd $args2 SCO_MAIL_ADMIN_ERR_TFADMIN} ret2] || \
		    [ErrorCatch errStack 0 {mag_remoteCommand $host \
			$cmd $args3 SCO_MAIL_ADMIN_ERR_TFADMIN} ret3]} {
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
		SaStatusBarSet $status_line [mag_msg SAVING]
		set ret [mag_all_put]
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
	exit 0
}

proc \
cleanup {} \
{
	global MAILDEFTMP MAILCFTMP

	system "rm -fr $MAILDEFTMP $MAILCFTMP"
}

proc \
main {} \
{
	global app DIRTY status_line main_list outerMainForm mainscreen_db
	global mag_host_name versionNumber

	# if command line mode, this routine does not return
	mag_cmd_line_main

	mag_setcat SCO_MAIL_ADMIN
	set app [VtOpen mailadmin [mag_msg HELPBOOK]]
	set versionString [mag_msg APP_TITLE]
	set versionString "$versionString $versionNumber"
	VtSetAppValues $app -versionString $versionString

	set outerMainForm [ \
		VtFormDialog $app.main \
		-resizable FALSE \
		-wmCloseCallback "quit_cb 1"\
		-title [mag_msg1 WIN_TITLE [mag_short_name_default]] \
		]
	mag_build_widgets $outerMainForm

	VtShow $outerMainForm
	VtLock

	# form is visible and we are locked, now init stuff
	set DIRTY 0
        # check if host if ok
        if {"[mag_host_check $mag_host_name]" != "ok"} {
                VtUnLock
                mag_error1 BADHOST $mag_host_name
                quit
        }
	# check if authorized
	if {"[mag_authorized $mag_host_name]" == "fail"} {
		SaDisplayNoAuths $outerMainForm.noAuths [mag_msg APP_TITLE] \
			"quit_cb 0" [mag_short_name_default]
		VtUnLock
		VtMainLoop
		return
	}		
	# get sendmail.cf either current one or new factory default one
	if {"[mag_cf_get]" != "ok"} {
		quit
	}
	# get /etc/default/mail either current or new one.
	if {"[mag_ms1_get]" != "ok"} {
		quit
	}
	# build onscreen db
	mao_init
	mag_insert_mainlist -1 [llength $mainscreen_db]
	mag_display_mainlist 0 1 0
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
		mag_cmd_shortcut
	} else {
		set TEST $tmp
	}
}
if {"$shortcut" == "yes"} {
	mag_cmd_quit 0
}
@endif

ErrorTopLevelCatch {

# hostname flag.
if {"[lindex $argv 0]" == "-h"} {
	lvarpop argv
	set mag_host_name [lvarpop argv]
	if {"$mag_host_name" == ""} {
		echo "mailadmin: No host name"
		exit 1
	}
}

# edit dialog boxes
if {[lindex $argv 0] == "-mag_stand_edit"} {
	mag_stand_edit [lindex $argv 1] [lindex $argv 2]
	exit
}

# channel dialog box
if {[lindex $argv 0] == "-mag_stand_channel"} {
	mag_stand_channel [lindex $argv 1] [lindex $argv 2]
	exit
}

# open host dialog box
if {[lindex $argv 0] == "-mag_stand_host"} {
	mag_stand_host [lindex $argv 1]
	exit
}

main} "mailadmin"
