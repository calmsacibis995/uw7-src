$ ==============================================================================
$
$	ident @(#) channel.msg 11.1 97/10/30 
$
$ *****************************************************************************
$
$	Copyright (C) 1993-1997 The Santa Cruz Operation, Inc.
$		All Rights Reserved.
$
$	The information in this file is provided for the exclusive use of
$	the licensees of The Santa Cruz Operation, Inc.  Such users have the
$	right to use, modify, and incorporate this code into other products
$	for purposes authorized by the license agreement provided they include
$	this notice and the associated copyright notice with any such product.
$	The information in this file is provided "AS IS" without warranty.
$
$ *****************************************************************************
$
$ message catalog for SCO Mail Administration Channel client
$
$ Modification History
$
$ M000, 20-Feb-97, andrean
$	- Created.
$
$ ==============================================================================

$moduleset SCO_MAIL_ADMIN_CHANNEL tcl=channel

$ mail helpbook name -- do NOT translate (unless doc specifies
$ a different help hook file name for your locale)
SCO_MAIL_ADMIN_CHANNEL_MSG_HELPBOOK		"channel"

SCO_MAIL_ADMIN_CHANNEL_MSG_TITLE		"Channel Table File %1$s"
SCO_MAIL_ADMIN_CHANNEL_MSG_ITEMS		"Entries"
SCO_MAIL_ADMIN_CHANNEL_MSG_FILE_DOESNT_EXIST	"File does not exist"
SCO_MAIL_ADMIN_CHANNEL_MSG_OK			"OK"
SCO_MAIL_ADMIN_CHANNEL_MSG_CANCEL		"Cancel"
SCO_MAIL_ADMIN_CHANNEL_MSG_HELP			"Help"

SCO_MAIL_ADMIN_CHANNEL_MSG_COLUMN_HOST_LBL	"Host"
SCO_MAIL_ADMIN_CHANNEL_MSG_COLUMN_FORWARD_LBL	"Forward Via"

$ Menus/Options

SCO_MAIL_ADMIN_CHANNEL_MSG_OBJECT		"Entry"
SCO_MAIL_ADMIN_CHANNEL_MSG_OBJECT_MN		"E"

SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_ADD_LBL	"Add..."
SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_ADD_MN	"A"
SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_ADD_SH	"Add a channel table entry..."

SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_DELETE_LBL	"Delete"
SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_DELETE_MN	"D"
SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_DELETE_SH	"Delete a channel table entry"

SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_MODIFY_LBL	"Modify..."
SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_MODIFY_MN	"M"
SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_MODIFY_SH	"Modify a channel table entry..."

SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_UP_LBL	"Move Up"
SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_UP_MN	"U"
SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_UP_SH	"Move a channel table entry up in order"

SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_DOWN_LBL	"Move Down"
SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_DOWN_MN	"o"
SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_DOWN_SH	"Move a channel table entry down in order"

SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_SAVE_LBL	"Save"
SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_SAVE_MN	"S"
SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_SAVE_SH	"Save any changes"

SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_EXIT_LBL	"Exit"
SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_EXIT_MN	"x"
SCO_MAIL_ADMIN_CHANNEL_MSG_ENTRY_MENU_EXIT_SH	"Exit %s"

SCO_MAIL_ADMIN_CHANNEL_MSG_OPTIONS_MENU		"Options"
SCO_MAIL_ADMIN_CHANNEL_MSG_OPTIONS_MN		"O"
SCO_MAIL_ADMIN_CHANNEL_MSG_OPTIONS_TOOLBAR	"Toolbar"
SCO_MAIL_ADMIN_CHANNEL_MSG_OPTIONS_TOOLBAR_SH	"Show/hide Toolbar"

$ Add/Modify Entry
SCO_MAIL_ADMIN_CHANNEL_MSG_ADD_TITLE		"Add Channel Table Entry"
SCO_MAIL_ADMIN_CHANNEL_MSG_MODIFY_TITLE		"Modify Channel Table Entry"
SCO_MAIL_ADMIN_CHANNEL_MSG_NAME_LBL		"Host Name"
SCO_MAIL_ADMIN_CHANNEL_MSG_NAME_SH		"Enter the host name to be matched for this entry"
SCO_MAIL_ADMIN_CHANNEL_ERR_NAME_MISSING		"No host name specified for \"Host Name\" field"
SCO_MAIL_ADMIN_CHANNEL_ERR_NAME_DUPLICATE	"An entry for host name \"%1$s\" already exists"
SCO_MAIL_ADMIN_CHANNEL_MSG_REAL_NAME_LBL	"Forwarding Host Name"
SCO_MAIL_ADMIN_CHANNEL_MSG_REAL_NAME_SH		"Enter the host mail will be forwarded to"
SCO_MAIL_ADMIN_CHANNEL_ERR_REAL_NAME_MISSING	"No host name specified for \"Forwarding Host Name\" field"
SCO_MAIL_ADMIN_CHANNEL_MSG_SELECT_BTN_LBL	"Select..."
SCO_MAIL_ADMIN_CHANNEL_ERR_NAME_SPACE		"A host name may not contain a space"
SCO_MAIL_ADMIN_CHANNEL_MSG_ROUTE_LIST_LBL	"Route to host:"
SCO_MAIL_ADMIN_CHANNEL_ERR_ENTER_NAME		"You did not enter a host name"
SCO_MAIL_ADMIN_CHANNEL_MSG_ROUTE_ADD_BTN_LBL	"    Add...   "
SCO_MAIL_ADMIN_CHANNEL_MSG_ROUTE_ADD_BTN_SH	"Add a host to the route"
SCO_MAIL_ADMIN_CHANNEL_MSG_ROUTE_DELETE_BTN_LBL "   Delete   "
SCO_MAIL_ADMIN_CHANNEL_MSG_ROUTE_DELETE_BTN_SH 	"Delete a host from the route"
SCO_MAIL_ADMIN_CHANNEL_MSG_ROUTE_UP_BTN_SH	"Move this host name up in the route"
SCO_MAIL_ADMIN_CHANNEL_MSG_ROUTE_DOWN_BTN_SH 	"Move this host name down in the route"
SCO_MAIL_ADMIN_CHANNEL_MSG_DO_PARTIAL_LBL	"Do subdomain lookups"
SCO_MAIL_ADMIN_CHANNEL_MSG_DO_PARTIAL_SH		"Allow subdomain lookups on the host name"
SCO_MAIL_ADMIN_CHANNEL_MSG_ROUTE_ADD_DLG_TITLE	"Add To Route"
SCO_MAIL_ADMIN_CHANNEL_MSG_ROUTE_ADD_DLG_TEXT_LBL	"Enter host name(s):"

$ SaveConfirm
SCO_MAIL_ADMIN_CHANNEL_MSG_SAVE_CONFIRM_TITLE		"Save Changes?"
SCO_MAIL_ADMIN_CHANNEL_MSG_SAVE_CONFIRM_QUERY		"Do you wish to save the changes made?"
SCO_MAIL_ADMIN_CHANNEL_MSG_SAVE_CONFIRM_BTN_YES		"Yes"
SCO_MAIL_ADMIN_CHANNEL_MSG_SAVE_CONFIRM_BTN_NO		"No"
SCO_MAIL_ADMIN_CHANNEL_MSG_SAVE_CONFIRM_BTN_CANCEL	"Cancel"

$ icons/pixmaps
SCO_MAIL_ADMIN_CHANNEL_MSG_TB_ADD	"Add..."
SCO_MAIL_ADMIN_CHANNEL_MSG_TB_DELETE	"Delete"
SCO_MAIL_ADMIN_CHANNEL_MSG_TB_MODIFY	"Modify..."
SCO_MAIL_ADMIN_CHANNEL_MSG_BTN_UP_PX	"/etc/mail/admin/px/prev.px"
SCO_MAIL_ADMIN_CHANNEL_MSG_BTN_DOWN_PX	"/etc/mail/admin/px/next.px"

$ Labels
SCO_MAIL_ADMIN_CHANNEL_MSG_SELECTED		"Selected"
SCO_MAIL_ADMIN_CHANNEL_MSG_COUNT_LABEL_STRING	"%1$s %2$s, %3$s %4$s"
SCO_MAIL_ADMIN_CHANNEL_MSG_PARTIAL_LABEL_STRING	"* this entry allows subdomain lookups on the host name"

$ Status
SCO_MAIL_ADMIN_CHANNEL_MSG_INIT		"Initializing..."
SCO_MAIL_ADMIN_CHANNEL_MSG_REFRESH	"Loading channel table data..."
SCO_MAIL_ADMIN_CHANNEL_MSG_SAVE		"Saving channel table data..."

$ Errors
SCO_MAIL_ADMIN_CHANNEL_ERR_USAGE	"Usage: %1$s -f filename [-h hostname]"
SCO_MAIL_ADMIN_CHANNEL_ERR_DIRECTORY	"\"%1$s\" is a directory, not a file"
SCO_MAIL_ADMIN_CHANNEL_ERR_DATABASE	"Unable to create channel table database\n\"%1$s\""
SCO_MAIL_ADMIN_CHANNEL_ERR_WRITE	"Unable to write channel table file\n\"%1$s\""
SCO_MAIL_ADMIN_CHANNEL_ERR_LOAD		"Unable to load channel table file\n\"%1$s\""
SCO_MAIL_ADMIN_CHANNEL_ERR_NONSTDCMD	"%s"
