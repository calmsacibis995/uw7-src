$ ==============================================================================
$
$	ident @(#) vacation.msg 11.1 97/10/30 
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
$ English messages for user vacation admin.
$
$ Modification History
$
$ M000, 14-Apr-97, andrean
$	- Initial revision -- based upon OSR vacation client messages.
$ 
$ ==============================================================================

$moduleset SCO_MAIL_ADMIN_VACATION tcl=vacation

$ helpbook name -- do NOT translate (unless doc specified a
$ different help hook file name for your locale)
SCO_MAIL_ADMIN_VACATION_MSG_HELPBOOK      "vacation"

SCO_MAIL_ADMIN_VACATION_ERR_UNK_ARG         "vacation: unknown argument \"%1$s\""
SCO_MAIL_ADMIN_VACATION_MSG_TITLE           "Vacation Notification"
SCO_MAIL_ADMIN_VACATION_MSG_SET             "Set Vacation Notices"
SCO_MAIL_ADMIN_VACATION_MSG_NOTICE_ON       "On"
SCO_MAIL_ADMIN_VACATION_MSG_NOTICE_OFF      "Off"
SCO_MAIL_ADMIN_VACATION_MSG_DEFAULT         "Default"
SCO_MAIL_ADMIN_VACATION_MSG_AKA_TITLE       "Other mail addresses for <%1$s>"
SCO_MAIL_ADMIN_VACATION_MSG_AKA_ADD         "Add..."
SCO_MAIL_ADMIN_VACATION_MSG_AKA_REMOVE      "Remove"
SCO_MAIL_ADMIN_VACATION_MSG_NOTICE_TITLE    "Notices Were Sent To:"
SCO_MAIL_ADMIN_VACATION_MSG_NOTICE_CLEAR    "  Clear  "
SCO_MAIL_ADMIN_VACATION_MSG_SUBJECT         "Notification Subject"
SCO_MAIL_ADMIN_VACATION_MSG_MESSAGE         "Notification Message"
SCO_MAIL_ADMIN_VACATION_MSG_WANT_CLR_TITLE  "Clear Notices?"
SCO_MAIL_ADMIN_VACATION_MSG_WANT_CLEARED    "Do you wish to clear the Notices Sent list?"
SCO_MAIL_ADMIN_VACATION_MSG_YES             "Yes"
SCO_MAIL_ADMIN_VACATION_MSG_NO              "No"

SCO_MAIL_ADMIN_VACATION_MSG_AKA_ADD_TITLE   "Add Other Address"
SCO_MAIL_ADMIN_VACATION_MSG_AKA_ADD_ADDRESS "Enter another mail address to be recognised as \"%1$s\":"
SCO_MAIL_ADMIN_VACATION_MSG_AKA_ADD_ADD     "Add"

SCO_MAIL_ADMIN_VACATION_MSG_TRIPNOTE	    "\tThis is an automatic reply to email you recently sent\nto %1$s.  Additional mail to %1$s will not result in\nfurther replies.  This mail indicates that the user is not\nresponding to your message for the following reason:\n\n\tNo pre-recorded message file was left by the user.\nHowever, this feature of the mail system is normally used\nduring vacations and other extended absences.\n\n\tThe Mail System\n" 

SCO_MAIL_ADMIN_VACATION_MSG_TRIPSUBJECT	    "Absence (Automatic reply)"

SCO_MAIL_ADMIN_VACATION_ERR_SUBJECT	    "You must enter a Notification Subject"
SCO_MAIL_ADMIN_VACATION_ERR_MESSAGE	    "You must enter a Notification Message"
SCO_MAIL_ADMIN_VACATION_ERR_SPACE	    "An address may not contain a space"
SCO_MAIL_ADMIN_VACATION_ERR_CANT_READ       "Can't read file \"%1$s\"."
SCO_MAIL_ADMIN_VACATION_ERR_CANT_WRITE      "Can't write file \"%1$s\"."
SCO_MAIL_ADMIN_VACATION_ERR_CANT_REMOVE     "Can't remove file \"%1$s\"."
SCO_MAIL_ADMIN_VACATION_ERR_CANT_LOCK       "Vacation preferences are already being edited."

$ Point Help messages:
SCO_MAIL_ADMIN_VACATION_MSG_HELP_AKA_ADD    "Press to add mail address(es)"
SCO_MAIL_ADMIN_VACATION_MSG_HELP_AKA_REMOVE "Press to remove the selected mail address"
SCO_MAIL_ADMIN_VACATION_MSG_HELP_AKA_LIST   "The list of addresses which will be considered \"you\""

SCO_MAIL_ADMIN_VACATION_MSG_HELP_NOTICE_CLEAR  "Press this to erase the names from this list"
SCO_MAIL_ADMIN_VACATION_MSG_HELP_NOTICE_LIST   "The list of addresses which were sent a notice"
SCO_MAIL_ADMIN_VACATION_MSG_HELP_MESSAGE    "A message to send with each notification"
SCO_MAIL_ADMIN_VACATION_MSG_HELP_MESSAGE_DEFAULT  "Press this to use the default notification message"
SCO_MAIL_ADMIN_VACATION_MSG_HELP_SUBJECT    "The subject for each notification"
SCO_MAIL_ADMIN_VACATION_MSG_HELP_SUBJECT_DEFAULT  "Press this to use the default notification subject"
