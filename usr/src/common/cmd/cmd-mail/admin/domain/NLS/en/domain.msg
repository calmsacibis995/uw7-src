$ ==============================================================================
$
$	ident @(#) domain.msg 11.1 97/10/30 
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
$ message catalog for SCO Mail Administration Domain client
$
$ Modification History
$
$ M000, 20-Feb-97, andrean
$	- Created.
$
$ ==============================================================================

$moduleset SCO_MAIL_ADMIN_DOMAIN tcl=domain

$ mail helpbook name -- do NOT translate (unless doc specifies
$ a different help hook file name for your locale)
SCO_MAIL_ADMIN_DOMAIN_MSG_HELPBOOK			"domain"

SCO_MAIL_ADMIN_DOMAIN_MSG_TITLE				"Domain Table File %1$s"
SCO_MAIL_ADMIN_DOMAIN_MSG_ITEMS				"Entries"
SCO_MAIL_ADMIN_DOMAIN_MSG_FILE_DOESNT_EXIST		"File does not exist"

SCO_MAIL_ADMIN_DOMAIN_MSG_OK				"OK"
SCO_MAIL_ADMIN_DOMAIN_MSG_CANCEL			"Cancel"
SCO_MAIL_ADMIN_DOMAIN_MSG_HELP				"Help"
SCO_MAIL_ADMIN_DOMAIN_MSG_COLUMN_PARTIAL_LBL		"*"
SCO_MAIL_ADMIN_DOMAIN_MSG_COLUMN_PARTIAL_LBL_CHARM	"x"
SCO_MAIL_ADMIN_DOMAIN_MSG_COLUMN_PARTIAL_NONE		" "
SCO_MAIL_ADMIN_DOMAIN_MSG_COLUMN_HOST_LBL		"Host"
SCO_MAIL_ADMIN_DOMAIN_MSG_COLUMN_ROUTE_LBL		"Route Via"

$ Menus/Options

SCO_MAIL_ADMIN_DOMAIN_MSG_OBJECT		"Entry"
SCO_MAIL_ADMIN_DOMAIN_MSG_OBJECT_MN		"E"

SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_ADD_LBL	"Add..."
SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_ADD_MN	"A"
SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_ADD_SH	"Add a domain table entry..."

SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_DELETE_LBL	"Delete"
SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_DELETE_MN	"D"
SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_DELETE_SH	"Delete a domain table entry"

SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_MODIFY_LBL	"Modify..."
SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_MODIFY_MN	"M"
SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_MODIFY_SH	"Modify a domain table entry..."

SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_UP_LBL	"Move Up"
SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_UP_MN	"U"
SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_UP_SH	"Move a domain table entry up in order"

SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_DOWN_LBL	"Move Down"
SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_DOWN_MN	"o"
SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_DOWN_SH	"Move a domain table entry down in order"

SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_SAVE_LBL	"Save"
SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_SAVE_MN	"S"
SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_SAVE_SH	"Save any changes"

SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_EXIT_LBL	"Exit"
SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_EXIT_MN	"x"
SCO_MAIL_ADMIN_DOMAIN_MSG_ENTRY_MENU_EXIT_SH	"Exit %s"

SCO_MAIL_ADMIN_DOMAIN_MSG_OPTIONS_MENU		"Options"
SCO_MAIL_ADMIN_DOMAIN_MSG_OPTIONS_MN		"O"
SCO_MAIL_ADMIN_DOMAIN_MSG_OPTIONS_TOOLBAR	"Toolbar"
SCO_MAIL_ADMIN_DOMAIN_MSG_OPTIONS_TOOLBAR_SH	"Show/hide Toolbar"

$ Add/Modify Entry
SCO_MAIL_ADMIN_DOMAIN_MSG_ADD_TITLE		"Add Domain Table Entry"
SCO_MAIL_ADMIN_DOMAIN_MSG_MODIFY_TITLE		"Modify Domain Table Entry"
SCO_MAIL_ADMIN_DOMAIN_MSG_NAME_LBL		"Name"
SCO_MAIL_ADMIN_DOMAIN_MSG_NAME_SH		"Enter the host name to be matched for this entry"
SCO_MAIL_ADMIN_DOMAIN_ERR_NAME_MISSING		"No host name specified for \"Name\" field"
SCO_MAIL_ADMIN_DOMAIN_MSG_REAL_NAME_LBL		"Real Name"
SCO_MAIL_ADMIN_DOMAIN_MSG_REAL_NAME_SH		"Enter the real host name mail will be sent to"
SCO_MAIL_ADMIN_DOMAIN_ERR_REAL_NAME_MISSING	"No host name specified for \"Real Name\" field"
SCO_MAIL_ADMIN_DOMAIN_ERR_NAME_SPACE		"A host name may not contain a space"
SCO_MAIL_ADMIN_DOMAIN_ERR_NAME_DUPLICATE	"An entry for host name \"%1$s\" already exists"
SCO_MAIL_ADMIN_DOMAIN_ERR_NUMERIC		"Subdomain lookups are not allowed on numeric addresses"
SCO_MAIL_ADMIN_DOMAIN_MSG_ROUTE_LIST_LBL	"Route to host:"
SCO_MAIL_ADMIN_DOMAIN_ERR_ENTER_NAME		"You did not enter a host name"
SCO_MAIL_ADMIN_DOMAIN_MSG_ROUTE_ADD_BTN_LBL	"  Add...  "
SCO_MAIL_ADMIN_DOMAIN_MSG_ROUTE_ADD_BTN_SH	"Add a host to the route"
SCO_MAIL_ADMIN_DOMAIN_MSG_ROUTE_DELETE_BTN_LBL	"  Delete  "
SCO_MAIL_ADMIN_DOMAIN_MSG_ROUTE_DELETE_BTN_SH 	"Delete a host from the route"
SCO_MAIL_ADMIN_DOMAIN_MSG_ROUTE_UP_BTN_SH	"Move this host name up in the route"
SCO_MAIL_ADMIN_DOMAIN_MSG_ROUTE_DOWN_BTN_SH 	"Move this host name down in the route"
SCO_MAIL_ADMIN_DOMAIN_MSG_DO_PARTIAL_LBL	"Do subdomain lookups"
SCO_MAIL_ADMIN_DOMAIN_MSG_DO_PARTIAL_SH		"Allow subdomain lookups on the host name"
SCO_MAIL_ADMIN_DOMAIN_MSG_ROUTE_ADD_DLG_TITLE		"Add To Route"
SCO_MAIL_ADMIN_DOMAIN_MSG_ROUTE_ADD_DLG_TEXT_LBL	"Enter host name(s):"
SCO_MAIL_ADMIN_DOMAIN_MSG_ROUTE_ADD_DLG_BTN_LBL		"Select..."

$ SaveConfirm
SCO_MAIL_ADMIN_DOMAIN_MSG_SAVE_CONFIRM_TITLE		"Save Changes?"
SCO_MAIL_ADMIN_DOMAIN_MSG_SAVE_CONFIRM_QUERY		"Do you wish to save the changes made?"
SCO_MAIL_ADMIN_DOMAIN_MSG_SAVE_CONFIRM_BTN_YES		"Yes"
SCO_MAIL_ADMIN_DOMAIN_MSG_SAVE_CONFIRM_BTN_NO		"No"
SCO_MAIL_ADMIN_DOMAIN_MSG_SAVE_CONFIRM_BTN_CANCEL	"Cancel"

$ icons/pixmaps
SCO_MAIL_ADMIN_DOMAIN_MSG_TB_ADD	"Add..."
SCO_MAIL_ADMIN_DOMAIN_MSG_TB_DELETE	"Delete"
SCO_MAIL_ADMIN_DOMAIN_MSG_TB_MODIFY	"Modify..."
SCO_MAIL_ADMIN_DOMAIN_MSG_DL_ARROW_PX	"/etc/mail/admin/px/arrow.px"
SCO_MAIL_ADMIN_DOMAIN_MSG_DL_CHECK_PX	"/etc/mail/admin/px/check.px"
SCO_MAIL_ADMIN_DOMAIN_MSG_BTN_UP_PX	"/etc/mail/admin/px/prev.px"
SCO_MAIL_ADMIN_DOMAIN_MSG_BTN_DOWN_PX	"/etc/mail/admin/px/next.px"

$ Labels
SCO_MAIL_ADMIN_DOMAIN_MSG_SELECTED		"Selected"
SCO_MAIL_ADMIN_DOMAIN_MSG_COUNT_LABEL_STR	"%1$s %2$s, %3$s %4$s"
SCO_MAIL_ADMIN_DOMAIN_MSG_PARTIAL_LABEL_STR	"*  = entry allows subdomain lookups on the host name"
SCO_MAIL_ADMIN_DOMAIN_MSG_PARTIAL_LABEL_STR_CHARM	"x  = entry allows subdomain lookups on the host name"

$ Status
SCO_MAIL_ADMIN_DOMAIN_MSG_INIT		"Initializing..."
SCO_MAIL_ADMIN_DOMAIN_MSG_REFRESH	"Loading domain table data..."
SCO_MAIL_ADMIN_DOMAIN_MSG_SAVE		"Saving domain table data..."

$ Errors
SCO_MAIL_ADMIN_DOMAIN_ERR_USAGE		"Usage: %1$s -f filename [-h hostname]"
SCO_MAIL_ADMIN_DOMAIN_ERR_DIRECTORY	"\"%1$s\" is a directory, not a file"
SCO_MAIL_ADMIN_DOMAIN_ERR_DATABASE	"Unable to create domain table database\n\"%1$s\""
SCO_MAIL_ADMIN_DOMAIN_ERR_WRITE		"Unable to write domain table file\n\"%1$s\""
SCO_MAIL_ADMIN_DOMAIN_ERR_LOAD		"Unable to load domain table file\n\"%1$s\""
SCO_MAIL_ADMIN_DOMAIN_ERR_NONSTDCMD	"%s"
