$
$	ident @(#) mfck.msg 11.1 97/10/30 
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
$ Message definitions for mfck.
$ 
$ Modification History
$
$

$quote "
$set MS_MFCK

$ do not translate "MMDF"
SCO_MFCK_MSG_CONV_MMDF		"Convert to MMDF format.\n"
$ do not translate "Sendmail"
SCO_MFCK_MSG_CONV_SENDMAIL	"Convert to Sendmail format.\n"
SCO_MFCK_MSG_NO_CONV		"No conversion necessary.\n"
SCO_MFCK_MSG_CONV_SUCCESS	"Conversion successful.\n"
$ for the next three, the arguments will be numbers
SCO_MFCK_MSG_DETECTED		"%d messages detected.\n"
SCO_MFCK_MSG_ERRORS		"%d errors were found.\n"
SCO_MFCK_MSG_FIXED		"%d fixes were made.\n"
SCO_MFCK_MSG_COMMIT		"Commit Changes?"
SCO_MFCK_MSG_UNEXPECTED_CTRLA	"Unexpected Ctrl-A delimiter, remove it?"
$ do not translate "From" -- the word "From" in this message
$ refers to the From message header
SCO_MFCK_MSG_FROM		"Start of message (From) without Ctrl-A delimiters.  Fix?"
SCO_MFCK_MSG_MISSING_CTRLA	"Ctrl-A delimiter missing, restore it?"
SCO_MFCK_MSG_CTRLA_BEGIN	"Unexpected Ctrl-A delimiter at beginning of folder, remove it?"
SCO_MFCK_MSG_GARBLED		"Start of first message garbled, fix it?"
SCO_MFCK_MSG_MISSING_END	"Missing end of header, insert it?"
SCO_MFCK_MSG_EXTRA_FROM		"Unpadded extra From line in header, pad it?"
SCO_MFCK_MSG_MISSING_CTRLA_LINE	"Missing Ctrl-A line, insert it?"
SCO_MFCK_MSG_CONTENT_LENGTH	"Missing/Bad Content-Length header, fix?"
$ the argument is a filename
SCO_MFCK_MSG_LOCKED		"%s is locked\n"
SCO_MFCK_MSG_MMDF_DETECTED	"MMDF Folder Detected.\n"
SCO_MFCK_MSG_SENDMAIL_DETECTED	"Sendmail Folder Detected.\n"
$ argument is a line number
SCO_MFCK_MSG_LINE		"line %d: "
SCO_MFCK_MSG_YES		"yes"
SCO_MFCK_MSG_NO			"no"

$ Error messages
SCO_MFCK_ERR_CANT_OPEN_MAILBOX	"Conversion failed, unable to open mailbox.\n"
SCO_MFCK_ERR_INVALID_MAILBOX	"Conversion failed, invalid mailbox format.\n"
SCO_MFCK_ERR_LOCKED_MAILBOX	"Conversion failed, mailbox is locked.\n"
$ the argument is a filename
SCO_MFCK_ERR_CREATE		"Unable to create %s\n"
$ the argument is a filename
SCO_MFCK_ERR_REOPEN		"Unable to reopen %s\n"
$ the argument is a filename
SCO_MFCK_ERR_OPEN		"Unable to open %s\n"
SCO_MFCK_ERR_MALLOC		"Malloc error\n"
$ in the following usage messages (actually one message),
$	- do not translate "mfck", "Sendmail", or "MMDF"
$	- all arguments are command line flags
SCO_MFCK_ERR_USAGE_1		"Usage for mfck (Mail Folder Check):\n"
SCO_MFCK_ERR_USAGE_2		"mfck %s Sendmail|MMDF folders...\n"
SCO_MFCK_ERR_USAGE_3		"   convert mail folders from one format to another\n"
SCO_MFCK_ERR_USAGE_4		"mfck [%s | %s] folders...\n"
SCO_MFCK_ERR_USAGE_5		"   check and repair damaged mail folders\n"
SCO_MFCK_ERR_USAGE_6		"   %s answer yes to all repair prompts\n"
SCO_MFCK_ERR_USAGE_7		"   %s answer no to all repair prompts\n"
