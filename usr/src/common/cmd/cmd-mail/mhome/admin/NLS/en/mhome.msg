$
$	ident @(#) mhome.msg 11.1 97/10/30 
$
$ ###########################################################################
$
$	Copyright (c) 1997 The Santa Cruz Operation, Inc.. 
$		All Rights Reserved. 
$
$	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF 
$		THE SANTA CRUZ OPERATION INC.
$	The copyright notice above does not evidence any
$	actual or intended publication of such source code.
$
$ ###########################################################################
$
$ message catalog for multihome gui.
$
$ Modification History
$
$ M000, 08-Sep-97, andrean
$	- us97-24561: Fixed typo in SCO_USER_MHOME_ERR_RESOLV
$	- New copyright.
$
$

$moduleset SCO_USER_MHOME Tcl=mhome

$ mail helpbook name -- do NOT translate (unless doc specifies
$ a different help hook file name for your locale)
SCO_USER_MHOME_MSG_HELPBOOK		"multihome"

$ error messages, can be multiple lines of text 75 chars max per line.
SCO_USER_MHOME_ERR_OPEN_FAIL		"Unable to open file: %1$s"
SCO_USER_MHOME_ERR_NO_MAP		"No map created for %1$s,\nfile contents may be invalid."
SCO_USER_MHOME_ERR_NOHOST		"Host (%1$s) not found."
SCO_USER_MHOME_ERR_BADHOST		"Host (%1$s) not responding.\nPlease check the following:\n   * your network configuration\n   * your network connection\n   * rhost permissions on the remote site\n   * that SCOadmin is installed at the remote site"
SCO_USER_MHOME_ERR_COPY_OUT		"Unable to fetch %1$s\nCreate new file?"
SCO_USER_MHOME_ERR_COPY_BACK		"Unable to copy back to %1$s."
SCO_USER_MHOME_ERR_PARSE		"Parse errors are contained in %1$s\nContinue and remove bad lines?"
SCO_USER_MHOME_ERR_WRITE		"Unable to write %1$s."
SCO_USER_MHOME_ERR_NO_VDOMAINS		"No virtual domains found."
SCO_USER_MHOME_ERR_BAD_SUSER		"Bad system user."
SCO_USER_MHOME_ERR_BAD_VUSER		"Bad virtual user."
SCO_USER_MHOME_ERR_DUP_VUSER		"Duplicate virtual user."
SCO_USER_MHOME_ERR_EXEC			"Unable to exec (%1$s)."
SCO_USER_MHOME_ERR_RESOLV		"Unable to resolve name (%1$s)."
SCO_USER_MHOME_ERR_TFADMIN		"Failure in tfadmin authorization check"

$ these can also be multiline 75 chars per line.
SCO_USER_MHOME_MSG_DIRTY		"Do you wish to save your changes?"
SCO_USER_MHOME_MSG_DOMAIN_UPDATE	"The virtual domain map file is out of date:\n(%1$s)\nDo you wish to rebuild it?"
SCO_USER_MHOME_MSG_GO_BACK		"Do you wish to return to host (%1$s)?"

$ labels, single line up to 75 chars.
SCO_USER_MHOME_MSG_USERS_FOR		"Users for %1$s"
SCO_USER_MHOME_MSG_USER_ADD		"Add User"
SCO_USER_MHOME_MSG_APP_TITLE		"Virtual Domain User Manager"
SCO_USER_MHOME_MSG_TITLE		"Virtual Domain User Manager on %1$s"
SCO_USER_MHOME_MSG_STATUS		"Virtual Domains on %1$s"

$ Menu bar and button entries
SCO_USER_MHOME_MSG_MENU_HOST		"Host"
SCO_USER_MHOME_MSG_MENU_HOST_MN		"t"
SCO_USER_MHOME_MSG_MENU_OPEN		"Open Host..."
SCO_USER_MHOME_MSG_TITLE_OPEN		"Open Host"
SCO_USER_MHOME_MSG_MENU_OPEN_MN		"O"
SCO_USER_MHOME_MSG_MENU_EXIT		"Exit"
SCO_USER_MHOME_MSG_MENU_EXIT_MN		"x"
SCO_USER_MHOME_MSG_MENU_EDIT		"Edit"
SCO_USER_MHOME_MSG_MENU_EDIT_MN		"E"
SCO_USER_MHOME_MSG_MENU_USER		"Users..."
SCO_USER_MHOME_MSG_MENU_USER_MN		"U"
SCO_USER_MHOME_MSG_MENU_ALIAS		"Mail Aliases..."
SCO_USER_MHOME_MSG_MENU_ALIAS_MN	"A"
SCO_USER_MHOME_MSG_MENU_RETIRE		"Retire User..."
SCO_USER_MHOME_MSG_MENU_RETIRE_MN	"R"

$ Short help entries (75 chars max).
SCO_USER_MHOME_MSG_SHORT_OPEN		"Manage virtual domain users on another host"
SCO_USER_MHOME_MSG_SHORT_EXIT		"Exit Virtual Domain User Manager"
SCO_USER_MHOME_MSG_SHORT_USER		"Edit Users in a virtual domain"
SCO_USER_MHOME_MSG_SHORT_ALIAS		"Edit Mail Aliases in a virtual domain"
SCO_USER_MHOME_MSG_SHORT_RETIRE		"Retire a User from all virtual domains"

$ More labels limited to 75 chars max.
SCO_USER_MHOME_MSG_LOADING		"Loading..."
SCO_USER_MHOME_MSG_SAVING		"Saving configuration..."

$ button labels, keep short.
SCO_USER_MHOME_MSG_STR_OK		"OK"
SCO_USER_MHOME_MSG_STR_YES		"Yes"
SCO_USER_MHOME_MSG_STR_NO		"No"
SCO_USER_MHOME_MSG_STR_CANCEL		"Cancel"
SCO_USER_MHOME_MSG_STR_BROWSE		"Select..."
SCO_USER_MHOME_MSG_STR_ADD_USER		"Add User..."
SCO_USER_MHOME_MSG_STR_DEL_USER		"Delete User"
SCO_USER_MHOME_MSG_VIRTUAL		"Virtual User"
SCO_USER_MHOME_MSG_SYSTEM		"System User"
