$
$	ident @(#) mag.msg 11.2 97/11/11 
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
$ message catalog for mail admin gui.
$
$ Modification History
$
$ M000, 08-Sep-97, andrean
$	- us97-24559, us97-24560: Fixed typos in two messages:
$		SCO_MAIL_ADMIN_ERR_CF_INVALID, SCO_MAIL_ADMIN_ERR_FQDN
$	- New copyright.
$		

$moduleset SCO_MAIL_ADMIN Tcl=mag

$ mail helpbook
SCO_MAIL_ADMIN_MSG_HELPBOOK	"mailadmin"

$ error messages, can be multiple lines of text 75 chars max per line.
SCO_MAIL_ADMIN_MSG_TITLE	"Mail Settings on %1$s"
SCO_MAIL_ADMIN_MSG_APP_TITLE	"Mail Manager"
$ argument is the name of the host being managed
SCO_MAIL_ADMIN_MSG_WIN_TITLE	"Mail Manager on %1$s"
SCO_MAIL_ADMIN_ERR_CF_COPY_OUT	"Unable to fetch %1$s\nIs it ok to restore the factory version?"
SCO_MAIL_ADMIN_ERR_CF_FACTORY_RESTORE	"Unable to restore %1$s factory defaults.\nCheck /etc/mail/sendmail.factory."
SCO_MAIL_ADMIN_ERR_CF_INVALID		"%1$s appears to have been hand edited\nDo you wish to continue?"
SCO_MAIL_ADMIN_ERR_CF_OPEN_FAIL		"Unable to open file: %1$s"
SCO_MAIL_ADMIN_ERR_CF_PARSE		"Unable to parse %1$s\nIs it ok to restore the factory version?"
SCO_MAIL_ADMIN_ERR_MS1_PARSE		"%1$s has parse errors.\nBad lines will be dropped.\nAlternatively you can restore the factory defaults file.\nDo you wish to restore the factory defaults file?"

SCO_MAIL_ADMIN_ERR_CF_WRITE		"Unable to write %1$s."
SCO_MAIL_ADMIN_ERR_COPY_BACK		"Unable to copy back to %1$s."
SCO_MAIL_ADMIN_ERR_FQDN			"Must specify a fully qualified domain name."
SCO_MAIL_ADMIN_ERR_INTEGER		"Invalid numeric value (%1$s)."
SCO_MAIL_ADMIN_ERR_INVALID		"Invalid entry (%1$s)."
SCO_MAIL_ADMIN_ERR_INVALID_TABLE	"Invalid table type (%1$s)."
SCO_MAIL_ADMIN_ERR_INVALID_NAME		"Invalid channel name (%1$s)."
SCO_MAIL_ADMIN_ERR_NULL			"Entry cannot be empty."
SCO_MAIL_ADMIN_ERR_AFILE_ONLY		"Can only edit alias files."
SCO_MAIL_ADMIN_ERR_USER_FIRST		"Must set non-default user first."
SCO_MAIL_ADMIN_ERR_CONFLICT		"Name conflict or restricted name."
SCO_MAIL_ADMIN_ERR_CH_NOT_LAST		"Channel is not last."
SCO_MAIL_ADMIN_ERR_BADUSER_ONLY		"There can only be one baduser channel."
SCO_MAIL_ADMIN_ERR_NOHOST		"Host (%1$s) not found."
SCO_MAIL_ADMIN_ERR_BADHOST		"Host (%1$s) not responding.\nPlease check the following:\n   * your network configuration\n   * your network connection\n   * rhost permissions on the remote site\n   * that SCOadmin is installed at the remote site"
SCO_MAIL_ADMIN_ERR_INVALID_CLASS	"Classname (%1$s) is invalid, unable to edit."
SCO_MAIL_ADMIN_ERR_EXEC			"Unable to exec (%1$s)."
SCO_MAIL_ADMIN_ERR_NO_MAP		"No map created for %1$s, file contents may be invalid."
SCO_MAIL_ADMIN_ERR_FILE_ONLY		"This item is only used for File Lookups."
SCO_MAIL_ADMIN_ERR_TFADMIN		"Failure in tfadmin authorization check"
SCO_MAIL_ADMIN_ERR_SENDMAIL_RESTART	"Failed to restart sendmail"

$ these can be multiline but each line is limited to 75 characters
SCO_MAIL_ADMIN_MSG_DIRTY		"Do you wish to save your changes?"
SCO_MAIL_ADMIN_MSG_CONT_DEFAULT		"This will reset all items in the folder to their default values\nDo you wish to continue?"
SCO_MAIL_ADMIN_MSG_ALIAS1		"Alias File or NIS MAP"
SCO_MAIL_ADMIN_MSG_ALIAS2		"Examples: btree:/etc/mail/aliases, nis:mail.aliases@domain"
SCO_MAIL_ADMIN_MSG_NOEXIST		"%1$s does not exist."
SCO_MAIL_ADMIN_MSG_NOEXIST_IGNORE	"%1$s does not exist.\nContinue?"
SCO_MAIL_ADMIN_MSG_EDIT_DTABLE		"Do you wish to edit the domain table?"
SCO_MAIL_ADMIN_MSG_GO_BACK		"Do you wish to return to host (%1$s)?"
SCO_MAIL_ADMIN_MSG_UUCP_UPDATE		"The UUCP Systems file map (%1$s) is out of date.\nDo you wish to rebuild it?"
SCO_MAIL_ADMIN_MSG_SAVING		"Saving configuration..."

$ button labels
SCO_MAIL_ADMIN_MSG_STR_YES		"Yes"
SCO_MAIL_ADMIN_MSG_STR_NO		"No"
SCO_MAIL_ADMIN_MSG_STR_CANCEL		"Cancel"
SCO_MAIL_ADMIN_MSG_STR_OK		"Ok"
SCO_MAIL_ADMIN_MSG_STR_SELECT		"Select..."
SCO_MAIL_ADMIN_MSG_STR_NONE		"None"
SCO_MAIL_ADMIN_MSG_STR_NONE_DELAY	"None/Delay"
SCO_MAIL_ADMIN_MSG_STR_EDIT_ALIAS	"Edit Alias File..."
SCO_MAIL_ADMIN_MSG_STR_EDIT_CTABLE	"Edit Channel Table File..."

$ Menu bar entries
SCO_MAIL_ADMIN_MSG_MENU_HOST		"Host"
SCO_MAIL_ADMIN_MSG_MENU_HOST_MN		"t"
SCO_MAIL_ADMIN_MSG_MENU_OPEN		"Open Host..."
SCO_MAIL_ADMIN_MSG_TITLE_OPEN		"Open Host"
SCO_MAIL_ADMIN_MSG_MENU_OPEN_MN		"O"
SCO_MAIL_ADMIN_MSG_MENU_EXIT		"Exit"
SCO_MAIL_ADMIN_MSG_MENU_EXIT_MN		"x"
SCO_MAIL_ADMIN_MSG_MENU_EDIT		"Edit"
SCO_MAIL_ADMIN_MSG_MENU_EDIT_MN		"E"
SCO_MAIL_ADMIN_MSG_MENU_ADD		"Add..."
SCO_MAIL_ADMIN_MSG_MENU_ADD_MN		"A"
SCO_MAIL_ADMIN_MSG_MENU_DEL		"Delete"
SCO_MAIL_ADMIN_MSG_MENU_DEL_MN		"D"
SCO_MAIL_ADMIN_MSG_MENU_MOD		"Modify..."
SCO_MAIL_ADMIN_MSG_TITLE_MOD		"Modify"
SCO_MAIL_ADMIN_MSG_MENU_MOD_MN		"M"
SCO_MAIL_ADMIN_MSG_MENU_UP		"Move Up"
SCO_MAIL_ADMIN_MSG_MENU_UP_MN		"U"
SCO_MAIL_ADMIN_MSG_MENU_DN		"Move Down"
SCO_MAIL_ADMIN_MSG_MENU_DN_MN		"o"
SCO_MAIL_ADMIN_MSG_MENU_SET		"Settings"
SCO_MAIL_ADMIN_MSG_MENU_SET_MN		"S"
SCO_MAIL_ADMIN_MSG_MENU_DEF		"Defaults"
SCO_MAIL_ADMIN_MSG_MENU_DEF_MN		"D"
SCO_MAIL_ADMIN_MSG_MENU_BADHOST		"Bad Host Forwarding/Delay"
SCO_MAIL_ADMIN_MSG_MENU_BADHOST_MN	"H"
SCO_MAIL_ADMIN_MSG_MENU_BADUSER		"Bad User Enable/Disable"
SCO_MAIL_ADMIN_MSG_MENU_BADUSER_MN	"U"
SCO_MAIL_ADMIN_MSG_MENU_MHOME		"Multihome Enable/Disable"
SCO_MAIL_ADMIN_MSG_MENU_MHOME_MN	"M"
SCO_MAIL_ADMIN_MSG_MENU_UUCP		"UUCP Enable/Disable"
SCO_MAIL_ADMIN_MSG_MENU_UUCP_MN		"C"

$ Short help entries (75 chars max)
SCO_MAIL_ADMIN_MSG_SHORT_OPEN		"Manage mail on another host"
SCO_MAIL_ADMIN_MSG_SHORT_EXIT		"Exit Mail Manager"
SCO_MAIL_ADMIN_MSG_SHORT_ADD		"Add item to selected list"
SCO_MAIL_ADMIN_MSG_SHORT_DEL		"Delete selected list-item"
SCO_MAIL_ADMIN_MSG_SHORT_MOD		"Modify selected item"
SCO_MAIL_ADMIN_MSG_SHORT_UP		"Move selected list-item up"
SCO_MAIL_ADMIN_MSG_SHORT_DN		"Most selected list-item down"
SCO_MAIL_ADMIN_MSG_SHORT_DEF		"Restore factory Defaults to selected item"
SCO_MAIL_ADMIN_MSG_SHORT_BADHOST	"Forward mail from unknown hosts"
SCO_MAIL_ADMIN_MSG_SHORT_BADUSER	"Forward mail from unknown users"
SCO_MAIL_ADMIN_MSG_SHORT_UUCP		"Enable or disable the UUCP channel"
SCO_MAIL_ADMIN_MSG_SHORT_MHOME		"Enable or disable the multihome channel"
SCO_MAIL_ADMIN_MSG_SHORT_FOLDER_DIR	"If null, users' home directories are used"
SCO_MAIL_ADMIN_MSG_SHORT_FOLDER_NAME	"If null, users' login names are used"

$ Note to translators:
$ the strings in the following section are limited to 38 characters.
$ If the following lines don't wrap in an 80 column editor in this file,
$ you will be ok.
SCO_MAIL_ADMIN_MSG_HOST			"Host Name"
SCO_MAIL_ADMIN_MSG_FROM			"Mail Comes From"
SCO_MAIL_ADMIN_MSG_DTABLE		"Domain Table Enabled"
SCO_MAIL_ADMIN_MSG_DTABLE_FILE		"Domain Table File"

SCO_MAIL_ADMIN_MSG_FOLDER_LOCATION	"Users' INBOX Location"
SCO_MAIL_ADMIN_MSG_FOLDER_SYSTEM	"System Spool Directory"
SCO_MAIL_ADMIN_MSG_FOLDER_HOME		"User's Home Directory"
SCO_MAIL_ADMIN_MSG_FOLDER_CUSTOM	"Custom Location"
SCO_MAIL_ADMIN_MSG_FOLDER_DIR		"Folder Directory"
SCO_MAIL_ADMIN_MSG_FOLDER_NAME		"Folder Name"
SCO_MAIL_ADMIN_MSG_FOLDER_FORMAT	"Default Folder Format"
SCO_MAIL_ADMIN_MSG_FOLDER_SENDMAIL	"Sendmail"
SCO_MAIL_ADMIN_MSG_FOLDER_MMDF		"MMDF"
SCO_MAIL_ADMIN_MSG_FOLDER_FSYNC		"Folder Synchronize to Disk"
SCO_MAIL_ADMIN_MSG_FOLDER_CCHECK	"Folder Consistency Checking"
SCO_MAIL_ADMIN_MSG_FOLDER_NORMAL	"Normal"
SCO_MAIL_ADMIN_MSG_FOLDER_EXT		"Extended"
SCO_MAIL_ADMIN_MSG_FOLDER_INCORE	"Folders Cached in Memory"
SCO_MAIL_ADMIN_MSG_FOLDER_THRESHOLD	"Folder Garbage Collection Threshold"
SCO_MAIL_ADMIN_MSG_FOLDER_TIMEOUT	"Folder Lock Timeout"
SCO_MAIL_ADMIN_MSG_FOLDER_FILELOCK	"File Based Locking Enabled"
SCO_MAIL_ADMIN_MSG_FOLDER_UMASK		"File/Directory Creation Umask"

$ don't translate the (X=) parts.
SCO_MAIL_ADMIN_MSG_CH_NAME		"Channel Name"
SCO_MAIL_ADMIN_MSG_CH_PROGRAM		"Channel Program (P=)"
SCO_MAIL_ADMIN_MSG_CH_ARGS		"Channel Program Arguments (A=)"
SCO_MAIL_ADMIN_MSG_CH_PROGDIR		"Channel Program Run Directory (D=)"
SCO_MAIL_ADMIN_MSG_CH_TABLE		"Channel Table Type"
SCO_MAIL_ADMIN_MSG_CH_FILE		"Channel Table File"
SCO_MAIL_ADMIN_MSG_CH_HOST		"Forward all Channel Mail to"
SCO_MAIL_ADMIN_MSG_CH_FLAGS		"Channel Flags (F=)"
SCO_MAIL_ADMIN_MSG_CH_EOL		"Channel End of Line Character (E=)"
SCO_MAIL_ADMIN_MSG_CH_MAXMSG		"Channel Maximum Message Size (M=)"
SCO_MAIL_ADMIN_MSG_CH_MAXLINE		"Channel Maximum Line Size (L=)"
SCO_MAIL_ADMIN_MSG_CH_NICE		"Channel Program Nice Increment (N=)"
SCO_MAIL_ADMIN_MSG_CH_USER		"Channel Program Runs as User (U=)"
SCO_MAIL_ADMIN_MSG_CH_GROUP		"Channel Program Runs as Group (U=)"
SCO_MAIL_ADMIN_MSG_CH_RECIP		"Recipient Envelope Ruleset (R=)"
SCO_MAIL_ADMIN_MSG_CH_HRECIP		"Recipient Header Ruleset (R=)"
SCO_MAIL_ADMIN_MSG_CH_SENDER		"Sender Envelope Ruleset (S=)"
SCO_MAIL_ADMIN_MSG_CH_HSENDER		"Sender Header Ruleset (S=)"

$ map as in mapping of one set of values to another (NIS Maps)
SCO_MAIL_ADMIN_MSG_MAP			"NIS Map"
SCO_MAIL_ADMIN_MSG_FILE			"File"
SCO_MAIL_ADMIN_MSG_ALTERNATE		"Alternate Name"

$ Note to translators:
$ the strings in the following section are limited to 75 characters.
SCO_MAIL_ADMIN_MSG_BASIC	"Basic Configuration"
SCO_MAIL_ADMIN_MSG_FOLDER	"Folder Configuration"
SCO_MAIL_ADMIN_MSG_ALIAS	"Alias Files and Maps"
SCO_MAIL_ADMIN_MSG_ALTNAME	"Alternate Host Names"
SCO_MAIL_ADMIN_MSG_CHANNEL	"Mail Delivery Channels"
SCO_MAIL_ADMIN_MSG_LOADING	"Loading..."
SCO_MAIL_ADMIN_MSG_NEW_CHANNEL	"New Channel"

$ misc entries 38 chars max
SCO_MAIL_ADMIN_MSG_NONE		"<None>"
SCO_MAIL_ADMIN_MSG_DEFAULT	"<Default>"
SCO_MAIL_ADMIN_MSG_RULEDEF	"<Same as Envelope>"
SCO_MAIL_ADMIN_MSG_LOCAL	"Local"
SCO_MAIL_ADMIN_MSG_NEW		"New"

$ These are channel names, they must be one word with no special characters.
$ if they don't translate then the english ones can remain.
SCO_MAIL_ADMIN_MSG_CLOCAL	"Local"
SCO_MAIL_ADMIN_MSG_CBADHOST	"Badhost"
SCO_MAIL_ADMIN_MSG_CBADUSER	"Baduser"
SCO_MAIL_ADMIN_MSG_CMHOME	"Multihome"

$ table types 38 chars max
SCO_MAIL_ADMIN_MSG_TTBADUSER	"Match Unknown Users"
SCO_MAIL_ADMIN_MSG_TTDNS	"Look in Name Service"
SCO_MAIL_ADMIN_MSG_TTUUCP	"Match UUCP Configuration (Systems file)"
SCO_MAIL_ADMIN_MSG_TTREMOTE	"Match Non-Local Users"
SCO_MAIL_ADMIN_MSG_TTLOCAL	"Match Local Users"
SCO_MAIL_ADMIN_MSG_TTFILE	"File Lookup"
SCO_MAIL_ADMIN_MSG_FLAGS_LIST1	"Flags Not Set"
SCO_MAIL_ADMIN_MSG_FLAGS_LIST2	"Flags Set"

$ channel flags single line, unlimited length (we have scroll bars here).
$ words in 'quotes' cannot be translated as they are part of the protocol
$ being documented here.
SCO_MAIL_ADMIN_MSG_F_UNKNOWN	"Unknown User Specified Flag"
SCO_MAIL_ADMIN_MSG_F_a	"Try ESMTP always instead of auto-sensing"
SCO_MAIL_ADMIN_MSG_F_A	"Look up user in the alias database, usually local mailers only"
SCO_MAIL_ADMIN_MSG_F_b	"Force blank line on end of a message"
SCO_MAIL_ADMIN_MSG_F_c	"Do not include comments in addresses"
SCO_MAIL_ADMIN_MSG_F_C	"Add @domain part for recipients that do not have it, the @domain part is taken from the sender address, it doesn't work reliably"
SCO_MAIL_ADMIN_MSG_F_d	"Do not include angle brackets around route-syntax addresses, useful for shell script mailers"
SCO_MAIL_ADMIN_MSG_F_D	"This mailer wants a date 'Date:' header line"
SCO_MAIL_ADMIN_MSG_F_e	"Expensive mailer to connect to, connect only on queue runs"
SCO_MAIL_ADMIN_MSG_F_E	"Escape lines beginning with 'From' in the message with a '>' sign"
SCO_MAIL_ADMIN_MSG_F_f	"Pass -f from flag to mailer"
SCO_MAIL_ADMIN_MSG_F_F	"This mailer wants a 'From:' header line"
SCO_MAIL_ADMIN_MSG_F_g	"Internally generated messages are from 'MAILER_DAEMON' ($u), instead of null"
SCO_MAIL_ADMIN_MSG_F_h	"Upper case should be preserved in host names for this mailer"
SCO_MAIL_ADMIN_MSG_F_I	"This mailer will be speaking SMTP to another sendmail - use special protocol features of sendmail"
SCO_MAIL_ADMIN_MSG_F_j	"Do User Database rewriting on recipients as well as senders"
SCO_MAIL_ADMIN_MSG_F_k	"Disable SMTP loopback check"
SCO_MAIL_ADMIN_MSG_F_K	"Currently unimplemented, reserved for chunking"
SCO_MAIL_ADMIN_MSG_F_l	"This mailer is local, final delivery will be performed"
SCO_MAIL_ADMIN_MSG_F_L	"Limit line lengths as specified in RFC821, L= should be used instead of this flag, this flag also sets the 7 flag"
SCO_MAIL_ADMIN_MSG_F_m	"Mailer can handle multiple users, $u will be expanded as needed"
SCO_MAIL_ADMIN_MSG_F_M	"This mailer wants a 'Message-Id:' header line"
SCO_MAIL_ADMIN_MSG_F_n	"Do not insert a UNIX-style 'From' line on the front of the message"
SCO_MAIL_ADMIN_MSG_F_o	"Always run as the user of the recipient mailbox, ignored if S is set"
SCO_MAIL_ADMIN_MSG_F_p	"Use route-addr style reverese-path in the SMTP 'MAIL FROM:' command rather than just the return address"
SCO_MAIL_ADMIN_MSG_F_P	"This mailer wants a 'Return-Path:' header line"
SCO_MAIL_ADMIN_MSG_F_q	"Addresses for this mailer are verified as local by the SMTP verify command, i.e. 250 responses are generated instead of 252 responses"
SCO_MAIL_ADMIN_MSG_F_r	"Same as f, but send -r flag"
SCO_MAIL_ADMIN_MSG_F_s	"Strip quote characters (\" and \\) off of the address before calling the mailer"
SCO_MAIL_ADMIN_MSG_F_S	"Don't reset the userid before calling the mailer, if U= is also specified, then that user and group is used"
SCO_MAIL_ADMIN_MSG_F_u	"Upper case should be preserved in user names for this mailer"
SCO_MAIL_ADMIN_MSG_F_U	"This mailer wants UUCP-style 'From' lines with the 'remote from host' on the end"
SCO_MAIL_ADMIN_MSG_F_w	"The user must have a valid account on this machine or the mail will bounce, this is required to enable .forward capability"
SCO_MAIL_ADMIN_MSG_F_x	"This mailer wants a 'Full-Name:' header line"
SCO_MAIL_ADMIN_MSG_F_X	"This mailer wants to use the hidden dot algorithm described in RFC821"
SCO_MAIL_ADMIN_MSG_F_0	"Don't look up MX records for hosts send via SMTP"
SCO_MAIL_ADMIN_MSG_F_3	"When encoding quoted printable, include those characters that don't map cleanly between ASCII and EBCDIC"
SCO_MAIL_ADMIN_MSG_F_5	"Enable ruleset 5 processing for this mailer, aliasing must be enabled (A flag)"
SCO_MAIL_ADMIN_MSG_F_7	"Strip all output to seven bits, L flag sets this by default"
SCO_MAIL_ADMIN_MSG_F_8	"Strip all output to seven bits, done after any 8 to 7 bit conversion"
SCO_MAIL_ADMIN_MSG_F_9	"Do limited 7 to 8 bit MIME conversion, text/plain messages only"
SCO_MAIL_ADMIN_MSG_F_COLON	"Check addresses to see if they begin with ':include:', if they do convert them to the '*include*' mailer"
SCO_MAIL_ADMIN_MSG_F_PIPE	"Check addresses to see if they begin with a '|', if they do convert them to the 'prog' mailer"
SCO_MAIL_ADMIN_MSG_F_SLASH	"Check addresses to see if they begin with a '/', if they do convert them to the '*file*' mailer"
SCO_MAIL_ADMIN_MSG_F_AT		"Look up addresses in the user database"

