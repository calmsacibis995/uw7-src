$ ==============================================================================
$
$	ident @(#) alias.msg 11.1 97/10/30 
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
$ Message definitions for SCO Mail Administration Aliases client
$
$ Modification History
$
$ M000, 20-Feb-97, andrean
$	- Created -- based upon OSR 5.0.2 MMDF Aliases client message file.
$ 
$ ==============================================================================

$moduleset SCO_MAIL_ADMIN_ALIAS Tcl=alias

$ the help book name -- do NOT translate (unless doc specifies a
$ different help hook file name for your locale)
SCO_MAIL_ADMIN_ALIAS_MSG_HELPBOOK      "aliases"

$ =============================================================================
$  "saveConfirm" client message section

SCO_MAIL_ADMIN_ALIAS_MSG_SAVE_CONFIRM_TITLE   "Save Changes?"
SCO_MAIL_ADMIN_ALIAS_MSG_SAVE_CONFIRM_QUERY   "Do you wish to save the changes made?"
SCO_MAIL_ADMIN_ALIAS_MSG_SAVE_CONFIRM_PB_YES        " Yes "
SCO_MAIL_ADMIN_ALIAS_MSG_SAVE_CONFIRM_PB_NO         "  No  "
SCO_MAIL_ADMIN_ALIAS_MSG_SAVE_CONFIRM_PB_CANCEL     "Cancel"


$ =============================================================================
$  "myStatusBox" message section

SCO_MAIL_ADMIN_ALIAS_MSG_STATUS_BOX_TITLE     "Information"


$ =============================================================================
$  "aliases" client message section

SCO_MAIL_ADMIN_ALIAS_MSG_TITLE          	"Alias File %1$s"
SCO_MAIL_ADMIN_ALIAS_MSG_FILE_DOESNT_EXIST	"File does not exist"
SCO_MAIL_ADMIN_ALIAS_ERR_DIRECTORY		"\"%1$s\" is a directory, not a file"

SCO_MAIL_ADMIN_ALIAS_MSG_SEARCH         "Search:"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_SEARCH    "Type the first few letters of a name to search for"

SCO_MAIL_ADMIN_ALIAS_ERR_NAME		"Alias names cannot contain \" \", \":\" or \",\" characters."
SCO_MAIL_ADMIN_ALIAS_MSG_READING        "Reading alias configuration.\nPlease wait..."
SCO_MAIL_ADMIN_ALIAS_MSG_SAVING         "Saving alias configuration.\nPlease wait..."
SCO_MAIL_ADMIN_ALIAS_MSG_REMOVE         "Really remove %1$s \"%2$s\"?"
SCO_MAIL_ADMIN_ALIAS_MSG_ALIAS          "alias"
SCO_MAIL_ADMIN_ALIAS_MSG_MEMBER         "member"
SCO_MAIL_ADMIN_ALIAS_MSG_REMOVE_MEMBERS "Really remove all the selected members?"
SCO_MAIL_ADMIN_ALIAS_ERR_NONAMES        "The alias must have both a name and at least one member"
SCO_MAIL_ADMIN_ALIAS_ERR_DUPLICATE      "An alias with that name already exists"
SCO_MAIL_ADMIN_ALIAS_ERR_NO_REDIR       "You must specify a mail redirection address"

SCO_MAIL_ADMIN_ALIAS_MSG_ADD_TITLE         "Add Alias"
SCO_MAIL_ADMIN_ALIAS_MSG_MFY_TITLE         "Modify Alias \"%1$s\""
SCO_MAIL_ADMIN_ALIAS_MSG_ADD_NAME          "Name"
SCO_MAIL_ADMIN_ALIAS_MSG_ADD_MEMBERS_TITLE "Name list"
SCO_MAIL_ADMIN_ALIAS_MSG_ADD_MEMBERS       "Members"
SCO_MAIL_ADMIN_ALIAS_ERR_NOMEMBERS         "You must add at least one member"

SCO_MAIL_ADMIN_ALIAS_MSG_MEMBER_TITLE      "Add Member"
SCO_MAIL_ADMIN_ALIAS_MSG_MEMBER_ADD        "Member(s) to add to \"%1$s\"?"\
SCO_MAIL_ADMIN_ALIAS_MSG_MEMBER_ADD_INST   "(Enter names separated with spaces)"
SCO_MAIL_ADMIN_ALIAS_ERR_LAST_MEMBER       "You cannot remove the last member of\nan alias.  Delete the alias instead.\n"

SCO_MAIL_ADMIN_ALIAS_MSG_LIST_TITLE         "Aliases"
SCO_MAIL_ADMIN_ALIAS_MSG_MEMBERS_TITLE      "Members"

SCO_MAIL_ADMIN_ALIAS_MSG_MENU_LABEL         "Aliases"
SCO_MAIL_ADMIN_ALIAS_MSG_MENU_MNEM          "A"
SCO_MAIL_ADMIN_ALIAS_MSG_MENU_ADD_LABEL     "Add..."
SCO_MAIL_ADMIN_ALIAS_MSG_MENU_ADD_MNEM      "A"

SCO_MAIL_ADMIN_ALIAS_MSG_MENU_REMOVE_LABEL  "Remove"
SCO_MAIL_ADMIN_ALIAS_MSG_MENU_REMOVE_MNEM   "R"
SCO_MAIL_ADMIN_ALIAS_MSG_MENU_MODIFY_LABEL  "Modify..."
SCO_MAIL_ADMIN_ALIAS_MSG_MENU_MODIFY_MNEM   "M"
SCO_MAIL_ADMIN_ALIAS_MSG_MENU_SAVE_LABEL    "Save"
SCO_MAIL_ADMIN_ALIAS_MSG_MENU_SAVE_MNEM     "S"
SCO_MAIL_ADMIN_ALIAS_MSG_MENU_EXIT_LABEL    "Exit"
SCO_MAIL_ADMIN_ALIAS_MSG_MENU_EXIT_MNEM     "x"

SCO_MAIL_ADMIN_ALIAS_MSG_MEMB_MENU_LABEL    	"Members"
SCO_MAIL_ADMIN_ALIAS_MSG_MEMB_MENU_MNEM     	"M"
SCO_MAIL_ADMIN_ALIAS_MSG_MEMB_MENU_ADD      	"Add..."
SCO_MAIL_ADMIN_ALIAS_MSG_MEMB_MENU_ADD_MNEM 	"A"
SCO_MAIL_ADMIN_ALIAS_MSG_MEMB_MENU_REMOVE   	"Remove"
SCO_MAIL_ADMIN_ALIAS_MSG_MEMB_MENU_REMOVE_MNEM  "R"

SCO_MAIL_ADMIN_ALIAS_MSG_USERS_MENU_LABEL   	"Users"
SCO_MAIL_ADMIN_ALIAS_MSG_USERS_MENU_MNEM    	"U"
SCO_MAIL_ADMIN_ALIAS_MSG_USERS_MENU_SUB     	"Subscribe..."
SCO_MAIL_ADMIN_ALIAS_MSG_USERS_MENU_SUB_MNEM     "S"
SCO_MAIL_ADMIN_ALIAS_MSG_USERS_MENU_RET     	"Retire..."
SCO_MAIL_ADMIN_ALIAS_MSG_USERS_MENU_RET_MNEM     "R"

SCO_MAIL_ADMIN_ALIAS_MSG_ADD_PB             " Add name(s) "
SCO_MAIL_ADMIN_ALIAS_MSG_REMOVE_PB          " Remove "
SCO_MAIL_ADMIN_ALIAS_MSG_EDIT_PB            " Edit "
SCO_MAIL_ADMIN_ALIAS_MSG_SELECT_PB          " Select name... "

SCO_MAIL_ADMIN_ALIAS_MSG_ADD_TYPE           "Enter name(s):"

SCO_MAIL_ADMIN_ALIAS_MSG_USER_SELECT_TITLE  "Select User(s)"
SCO_MAIL_ADMIN_ALIAS_MSG_USER_LIST_TITLE    "Select one or more users:"
SCO_MAIL_ADMIN_ALIAS_MSG_USER_SELECT_PB     "Select..."

SCO_MAIL_ADMIN_ALIAS_MSG_USER_SUB_TITLE     "Subscribe user %1$s"
SCO_MAIL_ADMIN_ALIAS_MSG_USER_SUB_INST      "Select aliases for %1$s to join:"
SCO_MAIL_ADMIN_ALIAS_MSG_USER_SUB_INST_SPACE "Select aliases for '%1$s' to join:"
SCO_MAIL_ADMIN_ALIAS_MSG_USER_SUB_ADD_PB    "   ADD >>"
SCO_MAIL_ADMIN_ALIAS_MSG_USER_SUB_RMV_PB    "<< REMOVE"
SCO_MAIL_ADMIN_ALIAS_MSG_USER_SUB_SYSLIST   "System aliases"
SCO_MAIL_ADMIN_ALIAS_MSG_USER_SUB_ULIST     "Aliases %1$s subscribes to"

SCO_MAIL_ADMIN_ALIAS_MSG_USER_RET_TITLE     "Remove User %1$s"

SCO_MAIL_ADMIN_ALIAS_MSG_USER_RET_INST      "This removes user %1$s from all aliases in\n%2$s.\n"
SCO_MAIL_ADMIN_ALIAS_MSG_USER_RET_REDIR     "Redirect mail for %1$s to:"

$ Error messages for command line options
SCO_MAIL_ADMIN_ALIAS_ERR_USAGE			"Usage: aliases -f [dbm:|btree:|hash:|implicit:]filename\n\t[-h hostname]\n\t[-a alias:member[,member...]] [-d alias[,alias...]]\n\t[-s user:alias[,alias...]] [-S user[,user...]]\n\t[-r user:alias[,alias...]] [-R user[,user...]]"
SCO_MAIL_ADMIN_ALIAS_ERR_MISSING_ADD_ARG	"Missing recipient list argument for -a option"
SCO_MAIL_ADMIN_ALIAS_ERR_MISSING_FILE_OPT       "Missing filename argument"
SCO_MAIL_ADMIN_ALIAS_ERR_UNKNOWN_ARG		"Unknown option \"%1$s\""
SCO_MAIL_ADMIN_ALIAS_ERR_UNKNOWN_CLASS		"Unknown database class \"%1$s\" specified in -f argument"


$ Short help messages
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_ALIASES       "Add and Remove aliases, Save, Quit"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_ALIASES_ADD   "Add an alias to the system"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_ALIASES_RMV   "Remove the selected alias from the system"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_ALIASES_MFY   "Modify the selected alias"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_ALIASES_SAVE  "Save any changes"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_ALIASES_QUIT  "Exit this program"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_MEMBERS       "Add and Remove members"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_MEMBERS_ADD   "Add a member to the selected alias to the system"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_MEMBERS_RMV   "Remove the selected member from the selected alias"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_ALIAS_ADD_NAME     "Type the name of the alias you wish to create"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_ALIAS_ADD_MEMBERS  "The list of members for the alias"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_ALIASES_LIST       "The list of aliases in file %1$s"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MEMBERS_LIST       "The list of members of a selected alias"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_ALIAS_ADD_PB       "Press to add typed names(s) to list"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_ALIAS_REM_PB       "Press to remove selected names(s)"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_ALIAS_EDIT_PB      "Press to edit selected names(s)"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_ALIAS_SEL_PB       "Press to select names from a list"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_ALIAS_ADD_TYPE     "Type name(s) to be added, separated by spaces"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_USERS         "Subscribe or Retire users"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_USERS_SUB     "Subscribe user(s) to aliases"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_USERS_RET     "Retire user(s)"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_USER_SUB_SYSLIST   "Aliases in file %1$s"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_USER_SUB_ADD_PB    "Moves the selected alias(es) to the user's subscription list"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_USER_SUB_RMV_PB    "Removes the selected alias(es) back to the system list"
SCO_MAIL_ADMIN_ALIAS_MSG_HELP_USER_SUB_ULIST     "The aliases that the user will subscribe to"

SCO_MAIL_ADMIN_ALIAS_ERR_CANT_SAVE	"aliases: Unable to save: %1$s"
SCO_MAIL_ADMIN_ALIAS_ERR_NEWALIASES	"aliases: Unable to compile alias database: %1$s"
SCO_MAIL_ADMIN_ALIAS_ERR_CANT_LOAD	"Failed to load alias file:\n%1$s"
SCO_MAIL_ADMIN_ALIAS_ERR_WRITE		"Failed to write alias file:\n%1$s"
SCO_MAIL_ADMIN_ALIAS_ERR_DB		"Failed to create alias database:\n%1$s"
SCO_MAIL_ADMIN_ALIAS_ERR_NONSTDCMD	"%s"
