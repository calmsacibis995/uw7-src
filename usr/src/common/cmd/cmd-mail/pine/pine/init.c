#if !defined(lint) && !defined(DOS)
static char rcsid[] = "$Id$";
#endif
/*----------------------------------------------------------------------

            T H E    P I N E    M A I L   S Y S T E M

   Laurence Lundblade and Mike Seibel
   Networks and Distributed Computing
   Computing and Communications
   University of Washington
   Administration Builiding, AG-44
   Seattle, Washington, 98195, USA
   Internet: lgl@CAC.Washington.EDU
             mikes@CAC.Washington.EDU

   Please address all bugs and comments to "pine-bugs@cac.washington.edu"


   Pine and Pico are registered trademarks of the University of Washington.
   No commercial use of these trademarks may be made without prior written
   permission of the University of Washington.

   Pine, Pico, and Pilot software and its included text are Copyright
   1989-1996 by the University of Washington.

   The full text of our legal notices is contained in the file called
   CPYRIGHT, included with this distribution.


   Pine is in part based on The Elm Mail System:
    ***********************************************************************
    *  The Elm Mail System  -  Revision: 2.13                             *
    *                                                                     *
    * 			Copyright (c) 1986, 1987 Dave Taylor              *
    * 			Copyright (c) 1988, 1989 USENET Community Trust   *
    ***********************************************************************
 

  ----------------------------------------------------------------------*/

/*======================================================================
     init.c
     Routines for pine start up and initialization
       init_vars
       free_vars
       init_username
       init_hostname
       read_pinerc
       write_pinerc
       init_mail_dir

       sent-mail expiration
 
       open debug files 

      - open and set up the debug files
      - fetch username, password, and home directory
      - get host and domain name
      - read and write the users .pinerc config file
      - create the "mail" subdirectory
      - expire old sent-mail

  ====*/


#include "headers.h"


typedef enum {ParseLocal, ParseGlobal, ParseFixed} ParsePinerc;
typedef enum {Sapling, Seedling, Seasoned} FeatureLevel;


/*
 * Internal prototypes
 */
void init_error PROTO((struct pine *, char *));
void read_pinerc PROTO((char *, struct variable *, ParsePinerc));
int  compare_sm_files PROTO((const QSType *, const QSType *));
/* AIX gives warning here 'cause it can't quite cope with enums */
void process_init_cmds PROTO((struct pine *, char **));
void process_feature_list PROTO((struct pine *, char **, int , int, int));
int  decode_sort PROTO((struct pine *, char *));
void display_init_err PROTO((char *, int));


#if	defined(DOS_EXTRA) && !(defined(WIN32) || defined (_WINDOWS))
#define	CONF_TXT_T	char __based(__segname("_CNFT"))
#else
#define	CONF_TXT_T	char
#endif

/*------------------------------------
Some definitions to keep the static "variable" array below 
a bit more readable...
  ----*/
/* be careful changing these PINERC_COMMENTs */
CONF_TXT_T cf_text_comment1[] = "# Updated by Pine(tm) ";
CONF_TXT_T cf_text_comment2[] = ", copyright 1989-1996 University of Washington.\n";
CONF_TXT_T cf_text_comment3[] = "#\n# Pine configuration file -- customize as needed.\n#\n# This file sets the configuration options used by Pine and PC-Pine.  If you\n# are using Pine on a Unix system, there may be a system-wide configuration\n# file which sets the defaults for these variables.  There are comments in\n# this file to explain each variable, but if you have questions about\n# specific\
 settings see the section on configuration options in the Pine\n# notes.  On Unix, run pine -conf to see how system defaults have been set.\n# For variables that accept multiple values, list elements are separated\n# by commas.  A line beginning with a space or tab is considered to be a\n# continuation of the previous line.  For a variable to be unset its value\n# must be blank.  To set a variable to the empty string its value should\n# \
be \"\".  You can override system defaults by setting a variable to the\n# empty string.  Switch variables are set to either \"yes\" or \"no\", and\n# default to \"no\".\n# Lines beginning with \"#\" are comments, and ignored by Pine.\n";


CONF_TXT_T cf_text_personal_name[] =	"Over-rides your full name from Unix password file. Required for PC-Pine.";

CONF_TXT_T cf_text_user_id[] =		"Your login/e-mail user name";

CONF_TXT_T cf_text_user_domain[] =		"Sets domain part of From: and local addresses in outgoing mail.";

CONF_TXT_T cf_text_smtp_server[] =		"List of SMTP servers for sending mail. If blank: Unix Pine uses sendmail.";

CONF_TXT_T cf_text_nntp_server[] =		"NNTP server for posting news. Also sets news-collections for news reading.";

CONF_TXT_T cf_text_inbox_path[] =		"Path of (local or remote) INBOX, e.g. ={mail.somewhere.edu}inbox\n# Normal Unix default is the local INBOX (usually /usr/spool/mail/$USER).";

CONF_TXT_T cf_text_incoming_folders[] =	"List of incoming msg folders besides INBOX, e.g. ={host2}inbox, {host3}inbox\n# Syntax: optnl-label {optnl-imap-host-name}folder-path";

CONF_TXT_T cf_text_folder_collections[] =	"List of directories where saved-message folders may be. First one is\n# the default for Saves. Example: Main {host1}mail/[], Desktop mail\\[]\n# Syntax: optnl-label {optnl-imap-hostname}optnl-directory-path[]";

CONF_TXT_T cf_text_news_collections[] =	"List, only needed if nntp-server not set, or news is on a different host\n# than used for NNTP posting. Examples: News *[] or News *{host3/nntp}[]\n# Syntax: optnl-label *{news-host/protocol}[]";

CONF_TXT_T cf_text_pruned_folders[] =	"List of context and folder pairs, delimited by a space, to be offered for\n# pruning each month.  For example: {host1}mail/[] mumble";

CONF_TXT_T cf_text_default_fcc[] =		"Over-rides default path for sent-mail folder, e.g. =old-mail (using first\n# folder collection dir) or ={host2}sent-mail or =\"\" (to suppress saving).\n# Default: sent-mail (Unix) or SENTMAIL.MTX (PC) in default folder collection.";

CONF_TXT_T cf_text_default_saved[] =	"Over-rides default path for saved-msg folder, e.g. =saved-messages (using first\n# folder collection dir) or ={host2}saved-mail or =\"\" (to suppress saving).\n# Default: saved-messages (Unix) or SAVEMAIL.MTX (PC) in default folder collection.";

CONF_TXT_T cf_text_postponed_folder[] =	"Over-rides default path for postponed messages folder, e.g. =pm (which uses\n# first folder collection dir) or ={host4}pm (using home dir on host4).\n# Default: postponed-msgs (Unix) or POSTPOND.MTX (PC) in default fldr coltn.";

CONF_TXT_T cf_text_mail_directory[] =	"Pine compares this value with the first folder collection directory.\n# If they match (or no folder collections are defined), and the directory\n# does not exist, Pine will create and use it. Default: ~/mail";

CONF_TXT_T cf_text_read_message_folder[] =	"If set, specifies where already-read messages will be moved upon quitting.";

CONF_TXT_T cf_text_signature_file[] =	"Over-rides default path for signature file. Default is ~/.signature";

CONF_TXT_T cf_text_global_address_book[] =	"List of file or path names for global/shared addressbook(s).\n# Default: none\n# Syntax: optnl-label path-name";

CONF_TXT_T cf_text_address_book[] =	"List of file or path names for personal addressbook(s).\n# Default: ~/.addressbook (Unix) or \\PINE\\ADDRBOOK (PC)\n# Syntax: optnl-label path-name";

CONF_TXT_T cf_text_feature_list[] =	"List of features; see Pine's Setup/options menu for the current set.\n# e.g. feature-list= select-without-confirm, signature-at-bottom\n# Default condition for all of the features is no-.";

CONF_TXT_T cf_text_initial_keystroke_list[] =	"Pine executes these keys upon startup (e.g. to view msg 13: i,j,1,3,CR,v)";

CONF_TXT_T cf_text_default_composer_hdrs[] =	"Only show these headers (by default) when composing messages";

CONF_TXT_T cf_text_customized_hdrs[] =	"Add these customized headers (and possible default values) when composing";

CONF_TXT_T cf_text_view_headers[] =	"When viewing messages, include this list of headers";

CONF_TXT_T cf_text_save_msg_name_rule[] =	"Determines default folder name for Saves...\n# Choices: default-folder, by-sender, by-from, by-recipient, last-folder-used.\n# Default: \"default-folder\", i.e. \"saved-messages\" (Unix) or \"SAVEMAIL\" (PC).";

CONF_TXT_T cf_text_fcc_name_rule[] =	"Determines default name for Fcc...\n# Choices: default-fcc, by-recipient, last-fcc-used.\n# Default: \"default-fcc\" (see also \"default-fcc=\" variable.)";

CONF_TXT_T cf_text_sort_key[] =		"Sets presentation order of messages in Index. Choices:\n# subject, from, arrival, date, size. Default: \"arrival\".";

CONF_TXT_T cf_text_addrbook_sort_rule[] =	"Sets presentation order of address book entries. Choices: dont-sort,\n# fullname-with-lists-last, fullname, nickname-with-lists-last, nickname\n# Default: \"fullname-with-lists-last\".";

CONF_TXT_T cf_text_character_set[] =	"Reflects capabilities of the display you have. Default: US-ASCII.\n# Typical alternatives include ISO-8859-x, (x is a number between 1 and 9).";

CONF_TXT_T cf_text_editor[] =		"Specifies the program invoked by ^_ in the Composer,\n# or the \"enable-alternate-editor-implicitly\" feature.";

CONF_TXT_T cf_text_speller[] =		"Specifies the program invoked by ^T in the Composer.";

CONF_TXT_T cf_text_fillcol[] =		"Specifies the column of the screen where the composer should wrap.";

CONF_TXT_T cf_text_replystr[] =		"Specifies the string to insert when replying to  message.";

CONF_TXT_T cf_text_emptyhdr[] =		"Specifies the string to use when sending a  message with no to or cc.";

CONF_TXT_T cf_text_image_viewer[] =	"Program to view images (e.g. GIF or TIFF attachments).";

CONF_TXT_T cf_text_use_only_domain_name[] = "If \"user-domain\" not set, strips hostname in FROM address. (Unix only)";

CONF_TXT_T cf_text_printer[] =		"Your default printer selection";

CONF_TXT_T cf_text_personal_print_command[] =	"List of special print commands";

CONF_TXT_T cf_text_personal_print_cat[] =	"Which category default print command is in";

CONF_TXT_T cf_text_standard_printer[] =	"The system wide standard printers";

CONF_TXT_T cf_text_last_time_prune_quest[] =	"Set by Pine; controls beginning-of-month sent-mail pruning.";

CONF_TXT_T cf_text_last_version_used[] =	"Set by Pine; controls display of \"new version\" message.";

CONF_TXT_T cf_text_bugs_fullname[] =	"Full name for bug report address used by \"Report Bug\" command.\n# Default: Pine Developers";

CONF_TXT_T cf_text_bugs_address[] =	"Email address used to send bug reports.\n# Default: pine-bugs@cac.washington.edu";

CONF_TXT_T cf_text_bugs_extras[] =		"Program/Script used by \"Report Bug\" command. No default.";

CONF_TXT_T cf_text_suggest_fullname[] =	"Full name for suggestion address used by \"Report Bug\" command.\n# Default: Pine Developers";

CONF_TXT_T cf_text_suggest_address[] =	"Email address used to send suggestions.\n# Default: pine-suggestions@cac.washington.edu";

CONF_TXT_T cf_text_local_fullname[] =	"Full name for \"local support\" address used by \"Report Bug\" command.\n# Default: Local Support";

CONF_TXT_T cf_text_local_address[] =	"Email address used to send to \"local support\".\n# Default: postmaster";

CONF_TXT_T cf_text_forced_abook[] =	"Force these address book entries into all writable personal address books.\n# Syntax is   forced-abook-entry=nickname|fullname|address\n# This is a comma-separated list of entries, each with syntax above.\n# Existing entries with same nickname are not replaced.\n# Example: help|Help Desk|help@ourdomain.com";

CONF_TXT_T cf_text_kblock_passwd[] =	"This is a number between 1 and 5.  It is the number of times a user will\n# have to enter a password when they run the keyboard lock command in the\n# main menu.  Default is 1.";

CONF_TXT_T cf_text_sendmail_path[] =	"This names the path to an alternative program, and any necessary arguments,\n# to be used in posting mail messages.  Example:\n#                    /usr/lib/sendmail -oem -t -oi\n# or,\n#                    /usr/local/bin/sendit.sh\n# The latter a script found in Pine distribution's contrib/util directory.\n# NOTE: The program MUST read the message to be posted on standard input,\n#       AND operate in the style of sendmail's \"-t\" option.";

CONF_TXT_T cf_text_oper_dir[] =	"This names the root of the tree to which the user is restricted when reading\n# and writing folders and files.  For example, on Unix ~/work confines the\n# user to the subtree beginning with their work subdirectory.\n# (Note: this alone is not sufficient for preventing access.  You will also\n# need to restrict shell access and so on, see Pine Technical Notes.)\n# Default: not set (so no restriction)";

CONF_TXT_T cf_text_in_fltr[] = 		"This variable takes a list of programs that message text is piped into\n# after MIME decoding, prior to display.";

CONF_TXT_T cf_text_out_fltr[] =		"This defines a program that message text is piped into before MIME\n# encoding, prior to sending";

CONF_TXT_T cf_text_alt_addrs[] =		"A list of alternate addresses the user is known by";

CONF_TXT_T cf_text_abook_formats[] =	"This is a list of formats for address books.  Each entry in the list is made\n# up of space-delimited tokens telling which fields are displayed and in\n# which order.  See help text";

CONF_TXT_T cf_text_index_format[] =	"This gives a format for displaying the index.  It is made\n# up of space-delimited tokens telling which fields are displayed and in\n# which order.  See help text";

CONF_TXT_T cf_text_overlap[] =		"The number of lines of overlap when scrolling through message text";

CONF_TXT_T cf_text_margin[] =		"Number of lines from top and bottom of screen where single\n# line scrolling occurs.";

CONF_TXT_T cf_text_stat_msg_delay[] =	"The number of seconds to sleep after writing a status message";

CONF_TXT_T cf_text_mailcheck[] =		"The approximate number of seconds between checks for new mail";

CONF_TXT_T cf_text_news_active[] =		"Path and filename of news configation's active file.\n# The default is typically \"/usr/lib/news/active\".";

CONF_TXT_T cf_text_news_spooldir[] =	"Directory containing system's news data.\n# The default is typically \"/usr/spool/news\"";

CONF_TXT_T cf_text_upload_cmd[] =		"Path and filename of the program used to upload text from your terminal\n# emulator's into Pine's composer.";

CONF_TXT_T cf_text_upload_prefix[] =	"Text sent to terminal emulator prior to invoking the program defined by\n# the upload-command variable.\n# Note: _FILE_ will be replaced with the temporary file used in the upload.";

CONF_TXT_T cf_text_download_cmd[] =	"Path and filename of the program used to download text via your terminal\n# emulator from Pine's export and save commands.";

CONF_TXT_T cf_text_download_prefix[] =	"Text sent to terminal emulator prior to invoking the program defined by\n# the download-command variable.\n# Note: _FILE_ will be replaced with the temporary file used in the downlaod.";

CONF_TXT_T cf_text_goto_default[] =	"Sets the default folder and collectionoffered at the Goto Command's prompt.";

CONF_TXT_T cf_text_mailcap_path[] =	"Sets the search path for the mailcap cofiguration file.\n# NOTE: colon delimited under UNIX, semi-colon delimited under DOS/Windows/OS2.";

CONF_TXT_T cf_text_mimetype_path[] =	"Sets the search path for the mimetypes cofiguration file.\n# NOTE: colon delimited under UNIX, semi-colon delimited under DOS/Windows/OS2.";

CONF_TXT_T cf_text_tcp_open_timeo[] =	"Sets the time in seconds that Pine will attempt to open a network\n# connection.  The default is 30, the minimum is 5, and the maximum is\n# system defined (typically 75).";

CONF_TXT_T cf_text_rsh_open_timeo[] =	"Sets the time in seconds that Pine will attempt to open a UNIX remote\n# shell connection.  The default is 15, min is 5, and max is unlimited.\n# Zero disables rsh altogether.";

CONF_TXT_T cf_text_version_threshold[] = "Sets the version number Pine will use as a threshold for offering\n# its new version message on startup.";

CONF_TXT_T cf_text_archived_folders[] =	"List of folder pairs; the first indicates a folder to archive, and the\n# second indicates the folder read messages in the first should\n# be moved to.";

CONF_TXT_T cf_text_elm_style_save[] =	"Elm-style-save is obsolete, use saved-msg-name-rule";

CONF_TXT_T cf_text_header_in_reply[] =	"Header-in-reply is obsolete, use include-header-in-reply in feature-list";

CONF_TXT_T cf_text_feature_level[] =	"Feature-level is obsolete, use feature-list";

CONF_TXT_T cf_text_old_style_reply[] =	"Old-style-reply is obsolete, use signature-at-bottom in feature-list";

CONF_TXT_T cf_text_compose_mime[] =	"Compose-mime is obsolete";

CONF_TXT_T cf_text_show_all_characters[] =	"Show-all-characters is obsolete";

CONF_TXT_T cf_text_save_by_sender[] =	"Save-by-sender is obsolete, use saved-msg-name-rule";

CONF_TXT_T cf_text_nntp_new_group_time[] =	"Last time NNTP server was checked for newly created news groups";

CONF_TXT_T cf_text_folder_extension[] =	"Extension used for local folder names (\".MTX\" by default).";

CONF_TXT_T cf_text_normal_foreground_color[] =	"Choose: black,blue,green,cyan,red,magenta,yellow,or white (CAPS=BLINKING).";

CONF_TXT_T cf_text_window_position[] =	"Window position in the format: CxR+X+Y\n# Where C and R are the window size in characters and X and Y are the\n# screen position of the top left corner of the window.";

CONF_TXT_T cf_text_newsrc_path[] =		"Full path and name of NEWSRC file";


/* these sort of divide up the pinerc file into categories */
char	cf_before_personal_name[] =	"########################### Essential Parameters ###########################";
char	cf_before_incoming_folders[] =	"###################### Collections, Folders, and Files #####################";
char	cf_before_feature_list[] =	"############################### Preferences ################################";
char	cf_before_printer[] =	"########## Set within or by Pine: No need to edit below this line ##########";


/* these are used to report folder directory creation problems */
CONF_TXT_T init_md_exists[] =	"The \"%s\" subdirectory already exists, but it is not writable by Pine so Pine cannot run.  Please correct the permissions and restart Pine.";

CONF_TXT_T init_md_file[] =	"Pine requires a directory called \"%s\" and usualy creates it.  You already have a regular file by that name which means Pine cannot create the directory.  Please move or remove it and start Pine again.";

CONF_TXT_T init_md_create[] =	"Creating subdirectory \"%s\" where Pine will store its mail folders.";


static int	copyright_line_is_there,
		trademark_lines_are_there;


/*----------------------------------------------------------------------
These are the variables that control a number of pine functions.  They
come out of the .pinerc and the /usr/local/lib/pine.conf files.  Some can
be set by the user while in Pine.  Eventually all the local ones should
be so and maybe the global ones too.

Each variable can have a command-line, user, global, and current value.
All of these values are malloc'd.  The user value is the one read out of
the user's .pinerc, the global value is the one from the system pine
configuration file.  There are often defaults for the global values, set
at the start of init_vars().  Perhaps someday there will be group values.
The current value is the one that is actually in use.
  ----*/
/* name::is_obsolete::is_used::been_written::is_user::is_global::is_list::
   is_fixed::description */
static struct variable variables[] = {
{"personal-name",              0, 1, 0, 1, 1, 0, 0, cf_text_personal_name},
#if defined(DOS) || defined(OS2)
                        /* Have to have this on DOS, PC's, Macs, etc... */
{"user-id",                    0, 1, 0, 1, 0, 0, 0,
#else			/* Don't allow on UNIX machines for some security */
{"user-id",                    0, 0, 0, 1, 0, 0, 0,
#endif
    cf_text_user_id},
{"user-domain",                0, 1, 0, 1, 1, 0, 0, cf_text_user_domain},
{"smtp-server",                0, 1, 0, 1, 1, 1, 0, cf_text_smtp_server},
{"nntp-server",                0, 1, 0, 1, 1, 1, 0, cf_text_nntp_server},
{"inbox-path",                 0, 1, 0, 1, 1, 0, 0, cf_text_inbox_path},
{"incoming-folders",           0, 1, 0, 1, 1, 1, 0, cf_text_incoming_folders},
{"folder-collections",         0, 1, 0, 1, 1, 1, 0,
						   cf_text_folder_collections},
{"news-collections",           0, 1, 0, 1, 1, 1, 0, cf_text_news_collections},
{"incoming-archive-folders",   0, 1, 0, 1, 1, 1, 0, cf_text_archived_folders},
{"pruned-folders",	       0, 1, 0, 1, 1, 1, 0, cf_text_pruned_folders},
{"default-fcc",                0, 1, 0, 1, 1, 0, 0, cf_text_default_fcc},
{"default-saved-msg-folder",   0, 1, 0, 1, 1, 0, 0, cf_text_default_saved},
{"postponed-folder",           0, 1, 0, 1, 1, 0, 0, cf_text_postponed_folder},
{"mail-directory",             0, 1, 0, 0, 1, 0, 0, cf_text_mail_directory},
{"read-message-folder",        0, 1, 0, 1, 1, 0, 0,
						  cf_text_read_message_folder},
{"signature-file",             0, 1, 0, 1, 1, 0, 0, cf_text_signature_file},
{"global-address-book",        0, 1, 0, 1, 1, 1, 0,
						  cf_text_global_address_book},
{"address-book",               0, 1, 0, 1, 1, 1, 0, cf_text_address_book},
{"feature-list",               0, 1, 0, 1, 1, 1, 0, cf_text_feature_list},
{"initial-keystroke-list",     0, 1, 0, 1, 1, 1, 0,
					       cf_text_initial_keystroke_list},
{"default-composer-hdrs",      0, 1, 0, 1, 1, 1, 0,
					        cf_text_default_composer_hdrs},
{"customized-hdrs",            0, 1, 0, 1, 1, 1, 0, cf_text_customized_hdrs},
{"viewer-hdrs",                0, 1, 0, 1, 1, 1, 0, cf_text_view_headers},
{"saved-msg-name-rule",        0, 1, 0, 1, 1, 0, 0,
						   cf_text_save_msg_name_rule},
{"fcc-name-rule",              0, 1, 0, 1, 1, 0, 0, cf_text_fcc_name_rule},
{"sort-key",                   0, 1, 0, 1, 1, 0, 0, cf_text_sort_key},
{"addrbook-sort-rule",         0, 1, 0, 1, 1, 0, 0,
						   cf_text_addrbook_sort_rule},
{"goto-default-rule",	       0, 1, 0, 1, 1, 0, 0, cf_text_goto_default},
{"character-set",              0, 1, 0, 1, 1, 0, 0, cf_text_character_set},
{"editor",                     0, 1, 0, 1, 1, 0, 0, cf_text_editor},
{"speller",                    0, 1, 0, 1, 1, 0, 0, cf_text_speller},
{"composer-wrap-column",       0, 1, 0, 1, 1, 0, 0, cf_text_fillcol},
{"reply-indent-string",        0, 1, 0, 1, 1, 0, 0, cf_text_replystr},
{"empty-header-message",       0, 1, 0, 1, 1, 0, 0, cf_text_emptyhdr},
{"image-viewer",               0, 1, 0, 1, 1, 0, 0, cf_text_image_viewer},
{"use-only-domain-name",       0, 1, 0, 1, 1, 0, 0,
						 cf_text_use_only_domain_name},
{"printer",                    0, 1, 0, 1, 1, 0, 0, cf_text_printer},
{"personal-print-command",     0, 1, 0, 1, 1, 1, 0,
					       cf_text_personal_print_command},
{"personal-print-category",    0, 1, 0, 1, 0, 0, 0,
					       cf_text_personal_print_cat},
{"standard-printer",           0, 1, 0, 0, 1, 1, 0, cf_text_standard_printer},
{"last-time-prune-questioned" ,0, 1, 0, 1, 0, 0, 0,
					   cf_text_last_time_prune_quest},
{"last-version-used",          0, 1, 0, 1, 0, 0, 0, cf_text_last_version_used},
{"bugs-fullname",              0, 1, 0, 0, 1, 0, 0, cf_text_bugs_fullname},
{"bugs-address",               0, 1, 0, 0, 1, 0, 0, cf_text_bugs_address},
{"bugs-additional-data",       0, 1, 0, 0, 1, 0, 0, cf_text_bugs_extras},
{"suggest-fullname",           0, 1, 0, 0, 1, 0, 0, cf_text_suggest_fullname},
{"suggest-address",            0, 1, 0, 0, 1, 0, 0, cf_text_suggest_address},
{"local-fullname",             0, 1, 0, 0, 1, 0, 0, cf_text_local_fullname},
{"local-address",              0, 1, 0, 0, 1, 0, 0, cf_text_local_address},
{"forced-abook-entry",         0, 1, 0, 0, 1, 1, 0, cf_text_forced_abook},
{"kblock-passwd-count",        0, 1, 0, 0, 1, 0, 0, cf_text_kblock_passwd},
{"sendmail-path",	       0, 1, 0, 1, 1, 0, 0, cf_text_sendmail_path},
{"operating-dir",	       0, 1, 0, 1, 1, 0, 0, cf_text_oper_dir},
{"display-filters",	       0, 1, 0, 1, 1, 1, 0, cf_text_in_fltr},
{"sending-filters",	       0, 1, 0, 1, 1, 1, 0, cf_text_out_fltr},
{"alt-addresses",              0, 1, 0, 1, 1, 1, 0, cf_text_alt_addrs},
{"addressbook-formats",        0, 1, 0, 1, 1, 1, 0, cf_text_abook_formats},
{"index-format",               0, 1, 0, 1, 1, 0, 0, cf_text_index_format},
{"viewer-overlap",             0, 1, 0, 1, 1, 0, 0, cf_text_overlap},
{"scroll-margin",              0, 1, 0, 1, 1, 0, 0, cf_text_margin},
{"status-message-delay",       0, 1, 0, 1, 1, 0, 0, cf_text_stat_msg_delay},
{"mail-check-interval",        0, 1, 0, 1, 1, 0, 0, cf_text_mailcheck},
{"newsrc-path",		       0, 1, 0, 1, 1, 0, 0, cf_text_newsrc_path},
{"news-active-file-path",      0, 1, 0, 1, 1, 0, 0, cf_text_news_active},
{"news-spool-directory",       0, 1, 0, 1, 1, 0, 0, cf_text_news_spooldir},
{"upload-command",	       0, 1, 0, 1, 1, 0, 0, cf_text_upload_cmd},
{"upload-command-prefix",      0, 1, 0, 1, 1, 0, 0, cf_text_upload_prefix},
{"download-command",	       0, 1, 0, 1, 1, 0, 0, cf_text_download_cmd},
{"download-command-prefix",    0, 1, 0, 1, 1, 0, 0, cf_text_download_prefix},
{"mailcap-search-path",	       0, 1, 0, 1, 1, 0, 0, cf_text_mailcap_path},
{"mimetype-search-path",       0, 1, 0, 1, 1, 0, 0, cf_text_mimetype_path},
{"tcp-open-timeout",	       0, 1, 0, 1, 1, 0, 0, cf_text_tcp_open_timeo},
{"rsh-open-timeout",	       0, 1, 0, 1, 1, 0, 0, cf_text_rsh_open_timeo},
{"new-version-threshold",      0, 1, 0, 1, 1, 0, 0, cf_text_version_threshold},

#ifdef NEWBB
{"nntp-new-group-time",        0, 1, 0, 1, 0, 0, 0,
						  cf_text_nntp_new_group_time},
#endif
/* OBSOLETE VARS */
{"elm-style-save",             1, 1, 0, 1, 1, 0, 0, cf_text_elm_style_save},
{"header-in-reply",            1, 1, 0, 1, 1, 0, 0, cf_text_header_in_reply},
{"feature-level",              1, 1, 0, 1, 1, 0, 0, cf_text_feature_level},
{"old-style-reply",            1, 1, 0, 1, 1, 0, 0, cf_text_old_style_reply},
{"compose-mime",               1, 1, 0, 0, 1, 0, 0, cf_text_compose_mime},
{"show-all-characters",        1, 1, 0, 1, 1, 0, 0,
						  cf_text_show_all_characters},
{"save-by-sender",             1, 1, 0, 1, 1, 0, 0, cf_text_save_by_sender},
#if defined(DOS) || defined(OS2)
{"folder-extension",	       0, 1, 0, 1, 1, 0, 0, cf_text_folder_extension},
{"normal-foreground-color",    0, 1, 0, 1, 1, 0, 0,
						cf_text_normal_foreground_color},
{"normal-background-color",    0, 1, 0, 1, 1, 0, 0, NULL},
{"reverse-foreground-color",   0, 1, 0, 1, 1, 0, 0, NULL},
{"reverse-background-color",   0, 1, 0, 1, 1, 0, 0, NULL},
#ifdef _WINDOWS
{"font-name",                  0, 1, 0, 1, 1, 0, 0, "Name and size of font."},
{"font-size",                  0, 1, 0, 1, 1, 0, 0, NULL},
{"font-style",                 0, 1, 0, 1, 1, 0, 0, NULL},
{"print-font-name",            0, 1, 0, 1, 1, 0, 0,
					     "Name and size of printer font."},
{"print-font-size",            0, 1, 0, 1, 1, 0, 0, NULL},
{"print-font-style",           0, 1, 0, 1, 1, 0, 0, NULL},
{"window-position",            0, 1, 0, 1, 1, 0, 0, cf_text_window_position},
#endif	/* _WINDOWS */
#endif	/* DOS */
{NULL,                         0, 0, 0, 0, 0, 0, 0, NULL},
};


#if	defined(DOS) || defined(OS2)
/*
 * Table containing Code Page value to external charset value mappings
 */
unsigned char *xlate_to_codepage   = NULL;
unsigned char *xlate_from_codepage = NULL;
#endif


static struct pinerc_line {
  char *line;
  struct variable *var;
  unsigned int  is_var:1;
  unsigned int  is_quoted:1;
  unsigned int  obsolete_var:1;
} *pinerc_lines = NULL;


#ifdef	DEBUG
/*
 * Debug level and output file defined here, referenced globally.
 * The debug file is opened and initialized below...
 */
int   debug	= DEFAULT_DEBUG;
FILE *debugfile = NULL;
#endif


init_init_vars(ps)
     struct pine *ps;
{
    ps->vars = variables;
}

    
/*----------------------------------------------------------------------
     Initialize the variables

 Args:   ps   -- The usual pine structure

 Result: 

  This reads the system pine configuration file and the user's pine
configuration file ".pinerc" and places the results in the variables 
structure.  It sorts out what was read and sets a few other variables 
based on the contents.
  ----*/
void 
init_vars(ps)
     struct pine *ps;
{
    char	 buf[MAXPATH +1], *p, *q, **s;
    register struct variable *vars = ps->vars;
    int		 obs_header_in_reply,     /* the obs_ variables are to       */
		 obs_old_style_reply,     /* support backwards compatibility */
		 obs_save_by_sender, i;
    FeatureLevel obs_feature_level;

    /*--- The defaults here are defined in os-xxx.h so they can vary
          per machine ---*/

    GLO_PRINTER			= cpystr(DF_DEFAULT_PRINTER);
    GLO_ELM_STYLE_SAVE		= cpystr(DF_ELM_STYLE_SAVE);
    GLO_SAVE_BY_SENDER		= cpystr(DF_SAVE_BY_SENDER);
    GLO_HEADER_IN_REPLY		= cpystr(DF_HEADER_IN_REPLY);
    GLO_INBOX_PATH		= cpystr("inbox");
    GLO_DEFAULT_FCC		= cpystr(DF_DEFAULT_FCC);
    GLO_DEFAULT_SAVE_FOLDER	= cpystr(DEFAULT_SAVE);
    GLO_POSTPONED_FOLDER	= cpystr(POSTPONED_MSGS);
    GLO_USE_ONLY_DOMAIN_NAME	= cpystr(DF_USE_ONLY_DOMAIN_NAME);
    GLO_FEATURE_LEVEL		= cpystr(DF_FEATURE_LEVEL);
    GLO_OLD_STYLE_REPLY		= cpystr(DF_OLD_STYLE_REPLY);
    GLO_SORT_KEY		= cpystr(DF_SORT_KEY);
    GLO_SAVED_MSG_NAME_RULE	= cpystr(DF_SAVED_MSG_NAME_RULE);
    GLO_FCC_RULE		= cpystr(DF_FCC_RULE);
    GLO_AB_SORT_RULE		= cpystr(DF_AB_SORT_RULE);
    GLO_SIGNATURE_FILE		= cpystr(DF_SIGNATURE_FILE);
    GLO_MAIL_DIRECTORY		= cpystr(DF_MAIL_DIRECTORY);
    GLO_BUGS_FULLNAME		= cpystr(DF_BUGS_FULLNAME);
    GLO_BUGS_ADDRESS		= cpystr(DF_BUGS_ADDRESS);
    GLO_SUGGEST_FULLNAME	= cpystr(DF_SUGGEST_FULLNAME);
    GLO_SUGGEST_ADDRESS		= cpystr(DF_SUGGEST_ADDRESS);
    GLO_LOCAL_FULLNAME		= cpystr(DF_LOCAL_FULLNAME);
    GLO_LOCAL_ADDRESS		= cpystr(DF_LOCAL_ADDRESS);
    GLO_OVERLAP			= cpystr(DF_OVERLAP);
    GLO_MARGIN			= cpystr(DF_MARGIN);
    GLO_FILLCOL			= cpystr(DF_FILLCOL);
    GLO_REPLY_STRING		= cpystr("> ");
    GLO_EMPTY_HDR_MSG		= cpystr("Undisclosed recipients");
    GLO_STATUS_MSG_DELAY	= cpystr("0");
    GLO_MAILCHECK		= cpystr(DF_MAILCHECK);
    GLO_KBLOCK_PASSWD_COUNT	= cpystr(DF_KBLOCK_PASSWD_COUNT);
#ifdef	DF_FOLDER_EXTENSION
    GLO_FOLDER_EXTENSION	= cpystr(DF_FOLDER_EXTENSION);
#endif
#ifdef	DF_SMTP_SERVER
    GLO_SMTP_SERVER		= parse_list(DF_SMTP_SERVER, 1, NULL);
#endif

    /*
     * Default first value for addrbook list if none set.
     * We also want to be sure to set global_val to the default
     * if is_fixed, so that address-book= will cause the default to happen.
     */
    if(!GLO_ADDRESSBOOK && !FIX_ADDRESSBOOK)
      GLO_ADDRESSBOOK = parse_list(DF_ADDRESSBOOK, 1, NULL);

    /*
     * Default first value if none set.
     */
    if(!GLO_STANDARD_PRINTER && !FIX_STANDARD_PRINTER)
      GLO_STANDARD_PRINTER = parse_list(DF_STANDARD_PRINTER, 1, NULL);

#if defined(DOS) || defined(OS2)
    /*
     * Rules for the config/support file locations under DOS are:
     *
     * 1) The location of the PINERC is searched for in the following
     *    order of precedence:
     *	     - File pointed to by '-p' command line option
     *       - File pointed to by PINERC environment variable
     *       - $HOME\pine
     *       - same dir as argv[0]
     *
     * 2) The HOME environment variable, if not set, defaults to 
     *    root of the current working drive (see pine.c)
     * 
     * 3) The default for external files (PINE.SIG and ADDRBOOK) is the
     *    same directory as the pinerc
     *
     * 4) The support files (PINE.HLP and PINE.NDX) are expected to be in
     *    the same directory as PINE.EXE.
     */
    if(!ps_global->pinerc){
	if(!(p = getenv("PINERC"))){
	    char buf2[MAXPATH];

	    p = buf;		/* ultimately holds the answer */
	    build_path(buf2, ps_global->home_dir, DF_PINEDIR);
	    if(is_writable_dir(buf2) == 0){
		/* $HOME/PINE/ exists!, see if $HOME/PINE/PINERC does too */
		build_path(buf, buf2, SYSTEM_PINERC);
		if(can_access(buf, ACCESS_EXISTS) != 0){
		    /*
		     * no $HOME/PINE/PINERC, make sure
		     * one isn't already in same dir as PINE.EXE
		     */
		    build_path(buf2, ps_global->pine_dir, SYSTEM_PINERC);
		    if(can_access(buf, ACCESS_EXISTS) == 0)
		      strcpy(buf, buf2);
		    /* else just create $HOME/PINEDIR/PINERC */
		}
		/* else just create $HOME/PINEDIR/PINERC */
	    }
	    else
	      /* no $HOME/pine dir, put PINERC next to PINE.EXE */
	      build_path(buf, ps_global->pine_dir, SYSTEM_PINERC);
	}

	ps_global->pinerc = cpystr(p);
    }

    /*
     * Now that we know the default for the PINERC, build NEWSRC default.
     * Backward compatibility makes this kind of funky.  If what the
     * c-client thinks the NEWSRC should be exists *AND* it doesn't
     * already exist in the PINERC's dir, use c-client's default, otherwise
     * use the one next to the PINERC...
     */
    p = last_cmpnt(ps_global->pinerc);
    buf[0] = '\0';
    if(p != NULL){
	strncpy(buf, ps_global->pinerc, p - ps_global->pinerc);
	buf[p - ps_global->pinerc] = '\0';
    }

    strcat(buf, "NEWSRC");

    if(!(p = (void *) mail_parameters(NULL, GET_NEWSRC, (void *)NULL))
       || can_access(p, ACCESS_EXISTS) < 0
       || can_access(buf, ACCESS_EXISTS) == 0){
	mail_parameters(NULL, SET_NEWSRC, (void *)buf);
	GLO_NEWSRC_PATH = cpystr(buf);
    }
    else
      GLO_NEWSRC_PATH = cpystr(p);

    /*
     * Now that we know where to look for the pinerc, see if a 
     * pointer to the global pine.conf has been set.  If so,
     * try loading it...
     */
    if(!ps_global->pine_conf && (p = getenv("PINECONF")))
      ps_global->pine_conf = cpystr(p);

    if(ps_global->pine_conf)
      read_pinerc(ps_global->pine_conf, vars, ParseGlobal);

    read_pinerc(ps_global->pinerc, vars, ParseLocal);
#else
    read_pinerc(ps_global->pine_conf ? ps_global->pine_conf
				     : SYSTEM_PINERC,
		vars,
		ParseGlobal);

    if(!ps_global->pinerc){
      build_path(buf, ps->home_dir, ".pinerc");
      ps_global->pinerc = cpystr(buf);
    }

    read_pinerc(ps_global->pinerc, vars, ParseLocal);
    read_pinerc(SYSTEM_PINERC_FIXED, vars, ParseFixed);
#endif

    set_current_val(&vars[V_INBOX_PATH], TRUE, TRUE);

    set_current_val(&vars[V_USER_DOMAIN], TRUE, TRUE);
    if(VAR_USER_DOMAIN
       && VAR_USER_DOMAIN[0]
       && (p = strrindex(VAR_USER_DOMAIN, '@'))){
	if(*(++p)){
	    char *q;

	    sprintf(tmp_20k_buf,
		    "User-domain (%s) cannot contain \"@\", using \"%s\"",
		    VAR_USER_DOMAIN, p);
	    init_error(ps, tmp_20k_buf);
	    q = VAR_USER_DOMAIN;
	    while((*q++ = *p++) != '\0')
	      ;/* do nothing */
	}
	else{
	    sprintf(tmp_20k_buf,
		    "User-domain (%s) cannot contain \"@\", deleting",
		    VAR_USER_DOMAIN);
	    init_error(ps, tmp_20k_buf);
	    fs_give((void **)&USR_USER_DOMAIN);
	    set_current_val(&vars[V_USER_DOMAIN], TRUE, TRUE);
	}
    }

    set_current_val(&vars[V_USE_ONLY_DOMAIN_NAME], TRUE, TRUE);
    set_current_val(&vars[V_REPLY_STRING], TRUE, TRUE);
    set_current_val(&vars[V_EMPTY_HDR_MSG], TRUE, TRUE);

    /* obsolete, backwards compatibility */
    set_current_val(&vars[V_HEADER_IN_REPLY], TRUE, TRUE);
    obs_header_in_reply=!strucmp(VAR_HEADER_IN_REPLY, "yes");

    set_current_val(&vars[V_PRINTER], TRUE, TRUE);
    set_current_val(&vars[V_PERSONAL_PRINT_COMMAND], TRUE, TRUE);
    set_current_val(&vars[V_STANDARD_PRINTER], TRUE, TRUE);

    set_current_val(&vars[V_LAST_TIME_PRUNE_QUESTION], TRUE, TRUE);
    if(VAR_LAST_TIME_PRUNE_QUESTION != NULL){
        /* The month value in the file runs from 1-12, the variable here
           runs from 0-11; the value in the file used to be 0-11, but we're 
           fixing it in January */
        ps->last_expire_year  = atoi(VAR_LAST_TIME_PRUNE_QUESTION);
        ps->last_expire_month =
			atoi(strindex(VAR_LAST_TIME_PRUNE_QUESTION, '.') + 1);
        if(ps->last_expire_month == 0){
            /* Fix for 0 because of old bug */
            char buf[10];
            sprintf(buf, "%d.%d", ps_global->last_expire_year,
              ps_global->last_expire_month + 1);
            set_variable(V_LAST_TIME_PRUNE_QUESTION, buf, 1);
        }else{
            ps->last_expire_month--; 
        } 
    }else{
        ps->last_expire_year  = -1;
        ps->last_expire_month = -1;
    }

    set_current_val(&vars[V_BUGS_FULLNAME], TRUE, TRUE);
    set_current_val(&vars[V_BUGS_ADDRESS], TRUE, TRUE);
    set_current_val(&vars[V_SUGGEST_FULLNAME], TRUE, TRUE);
    set_current_val(&vars[V_SUGGEST_ADDRESS], TRUE, TRUE);
    set_current_val(&vars[V_LOCAL_FULLNAME], TRUE, TRUE);
    set_current_val(&vars[V_LOCAL_ADDRESS], TRUE, TRUE);
    set_current_val(&vars[V_BUGS_EXTRAS], TRUE, TRUE);
    set_current_val(&vars[V_KBLOCK_PASSWD_COUNT], TRUE, TRUE);
    set_current_val(&vars[V_DEFAULT_FCC], TRUE, TRUE);
    set_current_val(&vars[V_POSTPONED_FOLDER], TRUE, TRUE);
    set_current_val(&vars[V_READ_MESSAGE_FOLDER], TRUE, TRUE);
    set_current_val(&vars[V_EDITOR], TRUE, TRUE);
    set_current_val(&vars[V_SPELLER], TRUE, TRUE);
    set_current_val(&vars[V_IMAGE_VIEWER], TRUE, TRUE);
    set_current_val(&vars[V_SMTP_SERVER], TRUE, TRUE);
    set_current_val(&vars[V_COMP_HDRS], TRUE, TRUE);
    set_current_val(&vars[V_CUSTOM_HDRS], TRUE, TRUE);
    set_current_val(&vars[V_SENDMAIL_PATH], TRUE, TRUE);
    set_current_val(&vars[V_DISPLAY_FILTERS], TRUE, TRUE);
    set_current_val(&vars[V_SEND_FILTER], TRUE, TRUE);
    set_current_val(&vars[V_ALT_ADDRS], TRUE, TRUE);
    set_current_val(&vars[V_ABOOK_FORMATS], TRUE, TRUE);

    set_current_val(&vars[V_OPER_DIR], TRUE, TRUE);
    if(VAR_OPER_DIR && !VAR_OPER_DIR[0]){
	init_error(ps,
 "Setting operating-dir to the empty string is not allowed.  Will be ignored!");
	fs_give((void **)&VAR_OPER_DIR);
	if(FIX_OPER_DIR)
	  fs_give((void **)&FIX_OPER_DIR);
	if(GLO_OPER_DIR)
	  fs_give((void **)&GLO_OPER_DIR);
	if(COM_OPER_DIR)
	  fs_give((void **)&COM_OPER_DIR);
	if(USR_OPER_DIR)
	  fs_give((void **)&USR_OPER_DIR);
    }

    set_current_val(&vars[V_INDEX_FORMAT], TRUE, TRUE);
    init_index_format(VAR_INDEX_FORMAT, &ps->index_disp_format);

    set_current_val(&vars[V_PERSONAL_PRINT_CATEGORY], TRUE, TRUE);
    ps->printer_category = -1;
    if(VAR_PERSONAL_PRINT_CATEGORY != NULL){
	ps->printer_category = atoi(VAR_PERSONAL_PRINT_CATEGORY);
	if(ps->printer_category < 1 || ps->printer_category > 3){
	    char **tt;
	    char aname[100];

	    strcat(strcpy(aname, ANSI_PRINTER), "-no-formfeed");
	    if(strucmp(VAR_PRINTER, ANSI_PRINTER) == 0
	      || strucmp(VAR_PRINTER, aname) == 0)
	      ps->printer_category = 1;
	    else if(VAR_STANDARD_PRINTER && VAR_STANDARD_PRINTER[0]){
		for(tt = VAR_STANDARD_PRINTER; *tt; tt++)
		  if(strucmp(VAR_PRINTER, *tt) == 0)
		    break;
		
		if(*tt)
		  ps->printer_category = 2;
	    }

	    /* didn't find it yet */
	    if(ps->printer_category < 1 || ps->printer_category > 3){
		if(VAR_PERSONAL_PRINT_COMMAND && VAR_PERSONAL_PRINT_COMMAND[0]){
		    for(tt = VAR_PERSONAL_PRINT_COMMAND; *tt; tt++)
		      if(strucmp(VAR_PRINTER, *tt) == 0)
			break;
		    
		    if(*tt)
		      ps->printer_category = 3;
		}
	    }
	}
    }

    set_current_val(&vars[V_OVERLAP], TRUE, TRUE);
    ps->viewer_overlap = i = atoi(DF_OVERLAP);
    if(SVAR_OVERLAP(ps, i, tmp_20k_buf))
      init_error(ps, tmp_20k_buf);
    else
      ps->viewer_overlap = i;

    set_current_val(&vars[V_MARGIN], TRUE, TRUE);
    ps->scroll_margin = i = atoi(DF_MARGIN);
    if(SVAR_MARGIN(ps, i, tmp_20k_buf))
      init_error(ps, tmp_20k_buf);
    else
      ps->scroll_margin = i;

    set_current_val(&vars[V_FILLCOL], TRUE, TRUE);
    ps->composer_fillcol = i = atoi(DF_FILLCOL);
    if(SVAR_FILLCOL(ps, i, tmp_20k_buf))
      init_error(ps, tmp_20k_buf);
    else
      ps->composer_fillcol = i;
    
    set_current_val(&vars[V_STATUS_MSG_DELAY], TRUE, TRUE);
    ps->status_msg_delay = i = 0;
    if(SVAR_MSGDLAY(ps, i, tmp_20k_buf))
      init_error(ps, tmp_20k_buf);
    else
      ps->status_msg_delay = i;

    /* timeout is a regular extern int because it is referenced in pico */
    set_current_val(&vars[V_MAILCHECK], TRUE, TRUE);
    timeout = i = 15;
    if(SVAR_MAILCHK(ps, i, tmp_20k_buf))
      init_error(ps, tmp_20k_buf);
    else
      timeout = i;

    set_current_val(&vars[V_NEWSRC_PATH], TRUE, TRUE);
    if(ps_global->VAR_NEWSRC_PATH && ps_global->VAR_NEWSRC_PATH[0])
      mail_parameters(NULL, SET_NEWSRC, (void *)ps_global->VAR_NEWSRC_PATH);

    set_current_val(&vars[V_NEWS_ACTIVE_PATH], TRUE, TRUE);
    if(ps_global->VAR_NEWS_ACTIVE_PATH)
      mail_parameters(NULL, SET_NEWSACTIVE,
		      (void *)ps_global->VAR_NEWS_ACTIVE_PATH);

    set_current_val(&vars[V_NEWS_SPOOL_DIR], TRUE, TRUE);
    if(ps_global->VAR_NEWS_SPOOL_DIR)
      mail_parameters(NULL, SET_NEWSSPOOL,
		      (void *)ps_global->VAR_NEWS_SPOOL_DIR);

    /* guarantee a save default */
    set_current_val(&vars[V_DEFAULT_SAVE_FOLDER], TRUE, TRUE);
    if(!VAR_DEFAULT_SAVE_FOLDER || !VAR_DEFAULT_SAVE_FOLDER[0])
      set_variable(V_DEFAULT_SAVE_FOLDER,
		   (GLO_DEFAULT_SAVE_FOLDER && GLO_DEFAULT_SAVE_FOLDER[0])
		     ? GLO_DEFAULT_SAVE_FOLDER
		     : DEFAULT_SAVE, 0);

    /* obsolete, backwards compatibility */
    set_current_val(&vars[V_FEATURE_LEVEL], TRUE, TRUE);
    if(strucmp(VAR_FEATURE_LEVEL, "seedling") == 0)
      obs_feature_level = Seedling;
    else if(strucmp(VAR_FEATURE_LEVEL, "old-growth") == 0)
      obs_feature_level = Seasoned;
    else
      obs_feature_level = Sapling;

    /* obsolete, backwards compatibility */
    set_current_val(&vars[V_OLD_STYLE_REPLY], TRUE, TRUE);
    obs_old_style_reply = !strucmp(VAR_OLD_STYLE_REPLY, "yes");

    set_current_val(&vars[V_SIGNATURE_FILE], TRUE, TRUE);
    set_current_val(&vars[V_CHAR_SET], TRUE, TRUE);
    set_current_val(&vars[V_GLOB_ADDRBOOK], TRUE, TRUE);
    set_current_val(&vars[V_ADDRESSBOOK], TRUE, TRUE);
    set_current_val(&vars[V_FORCED_ABOOK_ENTRY], TRUE, TRUE);
    set_current_val(&vars[V_NNTP_SERVER], TRUE, TRUE);
    for(i = 0; VAR_NNTP_SERVER && VAR_NNTP_SERVER[i]; i++)
      removing_quotes(VAR_NNTP_SERVER[i]);


    set_current_val(&vars[V_VIEW_HEADERS], TRUE, TRUE);
    /* strip spaces and colons */
    if(ps->VAR_VIEW_HEADERS){
	for(s = ps->VAR_VIEW_HEADERS; (q = *s) != NULL; s++){
	    if(q[0]){
		removing_leading_white_space(q);
		/* look for colon or space or end */
		for(p = q; *p && !isspace((unsigned char)*p) && *p != ':'; p++)
		  ;/* do nothing */
		
		*p = '\0';
		if(strucmp(q, ALL_EXCEPT) == 0)
		  ps->view_all_except = 1;
	    }
	}
    }

    set_current_val(&vars[V_UPLOAD_CMD], TRUE, TRUE);
    set_current_val(&vars[V_UPLOAD_CMD_PREFIX], TRUE, TRUE);
    set_current_val(&vars[V_DOWNLOAD_CMD], TRUE, TRUE);
    set_current_val(&vars[V_DOWNLOAD_CMD_PREFIX], TRUE, TRUE);
    set_current_val(&vars[V_MAILCAP_PATH], TRUE, TRUE);
    set_current_val(&vars[V_MIMETYPE_PATH], TRUE, TRUE);
    set_current_val(&vars[V_TCPOPENTIMEO], TRUE, TRUE);
    set_current_val(&vars[V_RSHOPENTIMEO], TRUE, TRUE);

#ifdef NEWBB
    set_current_val(&vars[V_NNTP_NEW_GROUP_TIME], TRUE, TRUE);
    if(ps_global->VAR_NNTP_NEW_GROUP_TIME == NULL ||
       strlen(ps_global->VAR_NNTP_NEW_GROUP_TIME) <= 0){
        /* If not set, set to two weeks ago */
        long now  = time(0) - 24 * 3600 * 14; /* Two weeks ago */
        set_variable(V_NNTP_NEW_GROUP_TIME, ctime(&now), 0);
    }
#endif

#if	defined(DOS) || defined(OS2)
    /*
     * Handle setting up page table IF running DOS 3.30 or greater
     */
    if(strucmp(ps_global->VAR_CHAR_SET, "us-ascii") != 0){
#ifdef	DOS
	unsigned long ver;
        extern unsigned int dos_version();
        extern          int dos_codepage();
#endif
	extern unsigned char *read_xtable();
#ifndef	_WINDOWS
	extern unsigned char  cp437L1[], cp850L1[], cp860L1[], cp863L1[],
			      cp865L1[], cp866L5[], cp852L2[], cp895L2[];
	extern unsigned char  L1cp437[], L1cp850[], L1cp860[], L1cp863[],
			      L1cp865[], L5cp866[], L2cp852[], L2cp895[];
#endif

	/* suck in the translation table */
#if	defined(DOS) && !defined(_WINDOWS)
	if(((ver = dos_version()) & 0x00ff) > 3 
	   || ((ver & 0x00ff) == 3 && (ver >> 8) >= 30))
#endif
	{
	    char *in_table  = getenv("ISO_TO_CP"),
	         *out_table = getenv("CP_TO_ISO");

	    if(out_table)
	      xlate_from_codepage = read_xtable(out_table);

	    if(in_table)
	      xlate_to_codepage = read_xtable(in_table);

#ifndef	_WINDOWS
	    /*
	     * if tables not already set, do the best we can...
	     */
	    switch(dos_codepage()){
	      case 437: /* latin-1 */
		if(strucmp(ps_global->VAR_CHAR_SET, "iso-8859-1") == 0){
		    if(!xlate_from_codepage)
		      xlate_from_codepage = cp437L1;

		    if(!xlate_to_codepage)
		      xlate_to_codepage   = L1cp437;
		}

		break;
	      case 850: /* latin-1 */
		if(strucmp(ps_global->VAR_CHAR_SET, "iso-8859-1") == 0){
		    if(!xlate_from_codepage)
		      xlate_from_codepage = cp850L1;

		    if(!xlate_to_codepage)
		      xlate_to_codepage = L1cp850;
		}

		break;
	      case 860: /* latin-1 */
		if(strucmp(ps_global->VAR_CHAR_SET, "iso-8859-1") == 0){
		    if(!xlate_from_codepage)
		      xlate_from_codepage = cp860L1;

		    if(!xlate_to_codepage)
		      xlate_to_codepage   = L1cp860;
		}

		break;
	      case 863: /* latin-1 */
		if(strucmp(ps_global->VAR_CHAR_SET, "iso-8859-1") == 0){
		    if(!xlate_from_codepage)
		      xlate_from_codepage = cp863L1;

		    if(!xlate_to_codepage)
		      xlate_to_codepage   = L1cp863;
		}

		break;
	      case 865: /* latin-1 */
		if(strucmp(ps_global->VAR_CHAR_SET, "iso-8859-1") == 0){
		    if(!xlate_from_codepage)
		      xlate_from_codepage = cp865L1;

		    if(!xlate_to_codepage)
		      xlate_to_codepage   = L1cp865;
		}

		break;
	      case 866: /* latin-5 */
		if(strucmp(ps_global->VAR_CHAR_SET, "iso-8859-5") == 0){
		    if(!xlate_from_codepage)
		      xlate_from_codepage = cp866L5;

		    if(!xlate_to_codepage)
		      xlate_to_codepage   = L5cp866;
		}
 
		break;
	      case 852: /* latin-2 */
		if(strucmp(ps_global->VAR_CHAR_SET, "iso-8859-2") == 0){
		    if(!xlate_from_codepage)
		      xlate_from_codepage = cp852L2;
 
		    if(!xlate_to_codepage)
		      xlate_to_codepage = L2cp852;
		}
 
		break;
	      case 895: /* latin-2 */
		if(strucmp(ps_global->VAR_CHAR_SET, "iso-8859-2") == 0){
		    if(!xlate_from_codepage)
		      xlate_from_codepage = cp895L2;
 
		    if(!xlate_to_codepage)
		      xlate_to_codepage = L2cp895;
		}

		break;
	      default:
		break;
	    }
#endif	/* _WINDOWS */
	}
    }

    set_current_val(&vars[V_FOLDER_EXTENSION], TRUE, TRUE);
    if(ps_global->VAR_FOLDER_EXTENSION)
      mail_parameters(NULL, SET_EXTENSION, 
		      (void *)ps_global->VAR_FOLDER_EXTENSION);

    set_current_val(&vars[V_NORM_FORE_COLOR], TRUE, TRUE);
    if(ps_global->VAR_NORM_FORE_COLOR)
      pico_nfcolor(ps_global->VAR_NORM_FORE_COLOR);

    set_current_val(&vars[V_NORM_BACK_COLOR], TRUE, TRUE);
    if(ps_global->VAR_NORM_BACK_COLOR)
      pico_nbcolor(ps_global->VAR_NORM_BACK_COLOR);

    set_current_val(&vars[V_REV_FORE_COLOR], TRUE, TRUE);
    if(ps_global->VAR_REV_FORE_COLOR)
      pico_rfcolor(ps_global->VAR_REV_FORE_COLOR);

    set_current_val(&vars[V_REV_BACK_COLOR], TRUE, TRUE);
    if(ps_global->VAR_REV_BACK_COLOR)
      pico_rbcolor(ps_global->VAR_REV_BACK_COLOR);

#ifdef	_WINDOWS
    set_current_val(&vars[V_FONT_NAME], TRUE, TRUE);
    set_current_val(&vars[V_FONT_SIZE], TRUE, TRUE);
    set_current_val(&vars[V_FONT_STYLE], TRUE, TRUE);
    set_current_val(&vars[V_WINDOW_POSITION], TRUE, TRUE);

    mswin_setwindow (ps_global->VAR_FONT_NAME, ps_global->VAR_FONT_SIZE, 
		    ps_global->VAR_FONT_STYLE, ps_global->VAR_WINDOW_POSITION);
    set_current_val(&vars[V_PRINT_FONT_NAME], TRUE, TRUE);
    set_current_val(&vars[V_PRINT_FONT_SIZE], TRUE, TRUE);
    set_current_val(&vars[V_PRINT_FONT_STYLE], TRUE, TRUE);
    mswin_setprintfont (ps_global->VAR_PRINT_FONT_NAME,
			ps_global->VAR_PRINT_FONT_SIZE,
			ps_global->VAR_PRINT_FONT_STYLE);

    {
	char **help_text = get_help_text (h_pine_for_windows, NULL);

	if (help_text != NULL)
	  mswin_sethelptext ("PC-Pine For Windows", NULL, 0, help_text);
    }

    mswin_setclosetext ("Use the \"Q\" command to exit Pine.");
#endif	/* _WINDOWS */
#endif	/* DOS */

    set_current_val(&vars[V_LAST_VERS_USED], TRUE, TRUE);
    /* Check for special cases first */
    if(VAR_LAST_VERS_USED
          /* Special Case #1: 3.92 use is effectively the same as 3.92 */
       && (strcmp(VAR_LAST_VERS_USED, "3.92") == 0
	   /*
	    * Special Case #2:  We're not really a new version if our
	    * version number looks like: <number><dot><number><number><alpha>
	    * The <alpha> on the end is key meaning its just a bug-fix patch.
	    */
	   || (isdigit((unsigned char)PINE_VERSION[0])
	       && PINE_VERSION[1] == '.'
	       && isdigit((unsigned char)PINE_VERSION[2])
	       && isdigit((unsigned char)PINE_VERSION[3])
	       && isalpha((unsigned char)PINE_VERSION[4])
	       && strncmp(VAR_LAST_VERS_USED, PINE_VERSION, 4) >= 0))){
	ps->show_new_version = 0;
	set_variable(V_LAST_VERS_USED, pine_version, 1);
    }
    /* Otherwise just do lexicographic comparision... */
    else if(VAR_LAST_VERS_USED
	    && strcmp(VAR_LAST_VERS_USED, PINE_VERSION) >= 0){
	ps->show_new_version = 0;
    }
    else{
        ps->pre390 = !(VAR_LAST_VERS_USED
		       && strcmp(VAR_LAST_VERS_USED, "3.90") >= 0);

	/*
	 * Don't offer the new version message if we're told not to.
	 */
	set_current_val(&vars[V_NEW_VER_QUELL], TRUE, TRUE);
	ps->show_new_version = !(VAR_NEW_VER_QUELL
			         && strcmp(pine_version,
					   VAR_NEW_VER_QUELL) < 0);

	set_variable(V_LAST_VERS_USED, pine_version, 1);
    }

    /* Obsolete, backwards compatibility */
    set_current_val(&vars[V_ELM_STYLE_SAVE], TRUE, TRUE);
    /* Also obsolete */
    set_current_val(&vars[V_SAVE_BY_SENDER], TRUE, TRUE);
    if(!strucmp(VAR_ELM_STYLE_SAVE, "yes"))
      set_variable(V_SAVE_BY_SENDER, "yes", 1);
    obs_save_by_sender = !strucmp(VAR_SAVE_BY_SENDER, "yes");

    /*
     * mail-directory variable is obsolete, put its value in
     * folder-collection list if that list is blank...
     */
    set_current_val(&vars[V_MAIL_DIRECTORY], TRUE, TRUE);
    set_current_val(&vars[V_PRUNED_FOLDERS], TRUE, TRUE);
    set_current_val(&vars[V_ARCHIVED_FOLDERS], TRUE, TRUE);
    set_current_val(&vars[V_INCOMING_FOLDERS], TRUE, TRUE);
    set_current_val(&vars[V_NEWS_SPEC], TRUE, TRUE);
    set_current_val(&vars[V_FOLDER_SPEC], TRUE, TRUE);
    /*
     * If there's no spec'd folder collection somewhere, set the current
     * value to the default, built from the mail directory...
     */
    if(!VAR_FOLDER_SPEC){
	build_path(tmp_20k_buf, VAR_MAIL_DIRECTORY, "[]");
	VAR_FOLDER_SPEC = parse_list(tmp_20k_buf, 1, NULL);
    }

    set_current_val(&vars[V_SORT_KEY], TRUE, TRUE);
    if(decode_sort(ps, VAR_SORT_KEY) == -1){
        if(!struncmp(VAR_SORT_KEY, "to", 2) ||
		!struncmp(VAR_SORT_KEY, "cc", 2)){
	   sprintf(tmp_20k_buf, "Sort type \"%s\" is not implemented yet\n",
		   VAR_SORT_KEY);
	   init_error(ps, tmp_20k_buf);
	}else{
	   fprintf(stderr, "Sort type \"%s\" is invalid\n", VAR_SORT_KEY);
	   exit(-1);
	}
    }

    set_current_val(&vars[V_SAVED_MSG_NAME_RULE], TRUE, TRUE);
    {int        i;
     NAMEVAL_S *v;
     for(i = 0; v = save_msg_rules(i); i++)
       if(!strucmp(VAR_SAVED_MSG_NAME_RULE, v->name)){
	   if((ps_global->save_msg_rule = v->value) == MSG_RULE_DEFLT){
	       if(!strucmp(USR_SAVED_MSG_NAME_RULE,
			   v->name))
		 obs_save_by_sender = 0;  /* don't overwrite */
	   }

	   break;
       }
    }

    set_current_val(&vars[V_FCC_RULE], TRUE, TRUE);
    {int        i;
     NAMEVAL_S *v;
     for(i = 0; v = fcc_rules(i); i++)
       if(!strucmp(VAR_FCC_RULE, v->name)){
	   ps_global->fcc_rule = v->value;
	   break;
       }
    }

    set_current_val(&vars[V_AB_SORT_RULE], TRUE, TRUE);
    {int        i;
     NAMEVAL_S *v;
     for(i = 0; v = ab_sort_rules(i); i++)
       if(!strucmp(VAR_AB_SORT_RULE, v->name)){
	   ps_global->ab_sort_rule = v->value;
	   break;
       }
    }

    set_current_val(&vars[V_GOTO_DEFAULT_RULE], TRUE, TRUE);
    {int        i;
     NAMEVAL_S *v;
     for(i = 0; v = goto_rules(i); i++)
       if(!strucmp(VAR_GOTO_DEFAULT_RULE, v->name)){
	   ps_global->goto_default_rule = v->value;
	   break;
       }
    }

    /* backwards compatibility */
    if(obs_save_by_sender){
        ps->save_msg_rule = MSG_RULE_FROM;
	set_variable(V_SAVED_MSG_NAME_RULE, "by-from", 1);
    }

    set_feature_list_current_val(&vars[V_FEATURE_LIST]);
    process_feature_list(ps, VAR_FEATURE_LIST,
           (obs_feature_level == Seasoned) ? 1 : 0,
	   obs_header_in_reply, obs_old_style_reply);

    /* this should come after process_feature_list because of use_fkeys */
    if(!ps->start_in_index)
        set_current_val(&vars[V_INIT_CMD_LIST], FALSE, TRUE);
    if(VAR_INIT_CMD_LIST)
        process_init_cmds(ps, VAR_INIT_CMD_LIST);

#ifdef	_WINDOWS
    mswin_set_quit_confirm (F_OFF(F_QUIT_WO_CONFIRM, ps_global));
#endif	/* _WINDOWS */

#ifdef DEBUG
    if(debugfile){
	gf_io_t pc;

	gf_set_writec(&pc, debugfile, 0L, FileStar);
	if(do_debug(debugfile))
	  dump_config(ps, pc);
    }
#endif /* DEBUG */
}


/*
 * free_vars -- give back resources acquired when we defined the
 *		variables list
 */
void
free_vars(ps)
    struct pine *ps;
{
    register int i;

    for(i = 0; ps && i <= V_LAST_VAR; i++)
      if(ps->vars[i].is_list){
	  char **l;

	  if(l = ps->vars[i].current_val.l){
	      for( ; *l; l++)
		fs_give((void **)l);

	      fs_give((void **)&ps->vars[i].current_val.l);
	  }

	  if(l = ps->vars[i].user_val.l){
	      for( ; *l; l++)
		fs_give((void **)l);

	      fs_give((void **)&ps->vars[i].user_val.l);
	  }

	  if(l = ps->vars[i].global_val.l){
	      for( ; *l; l++)
		fs_give((void **)l);

	      fs_give((void **)&ps->vars[i].global_val.l);
	  }

	  if(l = ps->vars[i].fixed_val.l){
	      for( ; *l; l++)
		fs_give((void **)l);

	      fs_give((void **)&ps->vars[i].fixed_val.l);
	  }

	  if(l = ps->vars[i].cmdline_val.l){
	      for( ; *l; l++)
		fs_give((void **)l);

	      fs_give((void **)&ps->vars[i].cmdline_val.l);
	  }
      }
      else{
	  if(ps->vars[i].current_val.p)
	    fs_give((void **)&ps->vars[i].current_val.p);

	  if(ps->vars[i].user_val.p)
	    fs_give((void **)&ps->vars[i].user_val.p);

	  if(ps->vars[i].global_val.p)
	    fs_give((void **)&ps->vars[i].global_val.p);

	  if(ps->vars[i].fixed_val.p)
	    fs_give((void **)&ps->vars[i].fixed_val.p);

	  if(ps->vars[i].cmdline_val.p)
	    fs_give((void **)&ps->vars[i].cmdline_val.p);
      }
}


/*
 * Standard way to get at feature list members...
 */
NAMEVAL_S *
feature_list(index)
    int index;
{
    /*
     * This list is alphabatized by feature string, but the 
     * macro values need not be ordered.
     */
    static NAMEVAL_S feat_list[] = {
	{"old-growth",				F_OLD_GROWTH},
#if !defined(DOS) && !defined(OS2)
	{"allow-talk",				F_ALLOW_TALK},
#endif
	{"assume-slow-link",			F_FORCE_LOW_SPEED},
	{"auto-move-read-msgs",			F_AUTO_READ_MSGS},
	{"auto-open-next-unread",		F_AUTO_OPEN_NEXT_UNREAD},
	{"auto-zoom-after-select",		F_AUTO_ZOOM},
	{"auto-unzoom-after-apply",		F_AUTO_UNZOOM},
	{"compose-cut-from-cursor",		F_DEL_FROM_DOT},
	{"compose-maps-delete-key-to-ctrl-d",	F_COMPOSE_MAPS_DEL},
	{"compose-rejects-unqualified-addrs",	F_COMPOSE_REJECTS_UNQUAL},
	{"compose-send-offers-first-filter",	F_FIRST_SEND_FILTER_DFLT},
	{"compose-sets-newsgroup-without-confirm", F_COMPOSE_TO_NEWSGRP},
	{"delete-skips-deleted",		F_DEL_SKIPS_DEL},
	{"disable-config-cmd",			F_DISABLE_CONFIG_SCREEN},
	{"disable-default-in-bug-report",	F_DISABLE_DFLT_IN_BUG_RPT},
	{"disable-busy-alarm",			F_DISABLE_ALARM},
	{"disable-keyboard-lock-cmd",		F_DISABLE_KBLOCK_CMD},
	{"disable-keymenu",			F_BLANK_KEYMENU},
	{"disable-password-cmd",		F_DISABLE_PASSWORD_CMD},
	{"disable-update-cmd",			F_DISABLE_UPDATE_CMD},
	{"disable-signature-edit-cmd",		F_DISABLE_SIGEDIT_CMD},
	{"enable-8bit-esmtp-negotiation",	F_ENABLE_8BIT},
	{"enable-8bit-nntp-posting",		F_ENABLE_8BIT_NNTP},
	{"enable-aggregate-command-set",	F_ENABLE_AGG_OPS},
	{"enable-alternate-editor-cmd",		F_ENABLE_ALT_ED},
	{"enable-alternate-editor-implicitly",	F_ALT_ED_NOW},
#ifdef	BACKGROUND_POST
	{"enable-background-sending",		F_BACKGROUND_POST},
#endif
	{"enable-bounce-cmd",			F_ENABLE_BOUNCE},
	{"enable-cruise-mode",			F_ENABLE_SPACE_AS_TAB},
	{"enable-cruise-mode-delete",		F_ENABLE_TAB_DELETES},
	{"enable-dot-files",			F_ENABLE_DOT_FILES},
	{"enable-dot-folders",			F_ENABLE_DOT_FOLDERS},
	{"enable-flag-cmd",			F_ENABLE_FLAG},
	{"enable-flag-screen-implicitly",	F_FLAG_SCREEN_DFLT},
	{"enable-full-header-cmd",		F_ENABLE_FULL_HDR},
	{"enable-goto-in-file-browser",		F_ALLOW_GOTO},
	{"enable-incoming-folders",		F_ENABLE_INCOMING},
	{"enable-jump-shortcut",		F_ENABLE_JUMP},
	{"enable-mail-check-cue",		F_SHOW_DELAY_CUE},
	{"enable-mouse-in-xterm",		F_ENABLE_MOUSE},
	{"enable-newmail-in-xterm-icon",	F_ENABLE_XTERM_NEWMAIL},
	{"enable-suspend",			F_CAN_SUSPEND},
	{"enable-tab-completion",		F_ENABLE_TAB_COMPLETE},
	{"enable-unix-pipe-cmd",		F_ENABLE_PIPE},
	{"enable-verbose-smtp-posting",		F_VERBOSE_POST},
	{"expanded-view-of-addressbooks",	F_EXPANDED_ADDRBOOKS},
	{"expanded-view-of-distribution-lists",	F_EXPANDED_DISTLISTS},
	{"expanded-view-of-folders",		F_EXPANDED_FOLDERS},
	{"expunge-without-confirm",		F_AUTO_EXPUNGE},
	{"fcc-on-bounce",			F_FCC_ON_BOUNCE},
	{"include-attachments-in-reply",	F_ATTACHMENTS_IN_REPLY},
	{"include-header-in-reply",		F_INCLUDE_HEADER},
	{"include-text-in-reply",		F_AUTO_INCLUDE_IN_REPLY},
	{"news-approximates-new-status",	F_FAKE_NEW_IN_NEWS},
	{"news-post-without-validation",	F_NO_NEWS_VALIDATION},
	{"news-read-in-newsrc-order",		F_READ_IN_NEWSRC_ORDER},
	{"pass-control-characters-as-is",	F_PASS_CONTROL_CHARS},
	{"preserve-start-stop-characters",	F_PRESERVE_START_STOP},
	{"print-offers-custom-cmd-prompt",	F_CUSTOM_PRINT},
	{"print-includes-from-line",		F_FROM_DELIM_IN_PRINT},
	{"print-index-enabled",			F_PRINT_INDEX},
	{"print-formfeed-between-messages",	F_AGG_PRINT_FF},
	{"quell-dead-letter-on-cancel",		F_QUELL_DEAD_LETTER},
	{"quell-lock-failure-warnings",		F_QUELL_LOCK_FAILURE_MSGS},
	{"quell-status-message-beeping",	F_QUELL_BEEPS},
	{"quell-user-lookup-in-passwd-file",	F_QUELL_LOCAL_LOOKUP},
	{"quit-without-confirm",		F_QUIT_WO_CONFIRM},
	{"reply-always-uses-reply-to",		F_AUTO_REPLY_TO},
	{"save-aggregates-copy-sequence",	F_AGG_SEQ_COPY},
	{"save-will-quote-leading-froms",	F_QUOTE_ALL_FROMS},
	{"save-will-not-delete",		F_SAVE_WONT_DELETE},
	{"save-will-advance",			F_SAVE_ADVANCES},
	{"select-without-confirm",		F_SELECT_WO_CONFIRM},
	{"show-cursor",				F_SHOW_CURSOR},
	{"show-selected-in-boldface",		F_SELECTED_SHOWN_BOLD},
	{"signature-at-bottom",			F_SIG_AT_BOTTOM},
	{"single-column-folder-list",		F_VERT_FOLDER_LIST},
	{"tab-visits-next-new-message-only",	F_TAB_TO_NEW},
	{"use-current-dir",			F_USE_CURRENT_DIR},
	{"use-function-keys",			F_USE_FK},
	{"use-sender-not-x-sender",		F_USE_SENDER_NOT_X},
	{"use-subshell-for-suspend",		F_SUSPEND_SPAWNS}
    };

    return((index >= 0 && index < (sizeof(feat_list)/sizeof(feat_list[0])))
	   ? &feat_list[index] : NULL);
}


/*
 * All the arguments past "list" are the backwards compatibility hacks.
 */
void
process_feature_list(ps, list, old_growth, hir, osr)
    struct pine *ps;
    char **list;
    int old_growth, hir, osr;
{
    register struct variable *vars = ps->vars;
    register char            *q;
    char                    **p,
                             *lvalue[LARGEST_BITMAP];
    int                       i,
                              yorn;
    NAMEVAL_S		     *feat;


    /* backwards compatibility */
    if(hir)
	F_TURN_ON(F_INCLUDE_HEADER, ps);

    /* ditto */
    if(osr)
	F_TURN_ON(F_SIG_AT_BOTTOM, ps);

    /* ditto */
    if(old_growth)
        set_old_growth_bits(ps, 0);

    /* now run through the list (global, user, and cmd_line lists are here) */
    if(list){
      for(p = list; (q = *p) != NULL; p++){
	if(struncmp(q, "no-", 3) == 0){
	  yorn = 0;
	  q += 3;
	}else{
	  yorn = 1;
	}

	for(i = 0; (feat = feature_list(i)) != NULL; i++){
	  if(strucmp(q, feat->name) == 0){
	    if(feat->value == F_OLD_GROWTH){
	      set_old_growth_bits(ps, yorn);
	    }else{
	      F_SET(feat->value, ps, yorn);
	    }
	    break;
	  }
	}
	/* if it wasn't in that list */
	if(feat == NULL)
          dprint(1, (debugfile,"Unrecognized feature in feature-list (%s%s)\n",
		     (yorn ? "" : "no-"), q));
      }
    }

    /*
     * Turn on gratuitous '>From ' quoting, if requested...
     */
    mail_parameters(NULL, SET_FROMWIDGET,
		    (void  *)(F_ON(F_QUOTE_ALL_FROMS, ps) ? 1 : 0));

    /*
     * Turn off .lock creation complaints...
     */
    if(F_ON(F_QUELL_LOCK_FAILURE_MSGS, ps))
      mail_parameters(NULL, SET_LOCKEACCESERROR, (void *) 0);

    if(F_ON(F_USE_FK, ps))
      ps->orig_use_fkeys = 1;

    /* Will we have to build a new list? */
    if(!(old_growth || hir || osr))
	return;

    /*
     * Build a new list for feature-list.  The only reason we ever need to
     * do this is if one of the obsolete options is being converted
     * into a feature-list item, and it isn't already included in the user's
     * feature-list.
     */
    i = 0;
    for(p = USR_FEATURE_LIST; p && (q = *p); p++){
      /* already have it or cancelled it, don't need to add later */
      if(hir && (strucmp(q, "include-header-in-reply") == 0 ||
                             strucmp(q, "no-include-header-in-reply") == 0)){
	hir = 0;
      }else if(osr && (strucmp(q, "signature-at-bottom") == 0 ||
                             strucmp(q, "no-signature-at-bottom") == 0)){
	osr = 0;
      }else if(old_growth && (strucmp(q, "old-growth") == 0 ||
                             strucmp(q, "no-old-growth") == 0)){
	old_growth = 0;
      }
      lvalue[i++] = cpystr(q);
    }

    /* check to see if we still need to build a new list */
    if(!(old_growth || hir || osr))
	return;

    if(hir)
      lvalue[i++] = "include-header-in-reply";
    if(osr)
      lvalue[i++] = "signature-at-bottom";
    if(old_growth)
      lvalue[i++] = "old-growth";
    lvalue[i] = NULL;
    set_variable_list(V_FEATURE_LIST, lvalue);
}



/*
 * set_old_growth_bits - Command used to set or unset old growth set
 *			 of features
 */
void
set_old_growth_bits(ps, val)
    struct pine *ps;
    int          val;
{
    int i;

    for(i = 1; i <= F_LAST_FEATURE; i++)
      if(test_old_growth_bits(ps, i))
	F_SET(i, ps, val);
}



/*
 * test_old_growth_bits - Test to see if all the old growth bits are on,
 *			  *or* if a particular feature index is in the old
 *			  growth set.
 *
 * WEIRD ALERT: if index == F_OLD_GROWTH bit values are tested
 *              otherwise a bits existence in the set is tested!!!
 *
 * BUG: this will break if an old growth feature number is ever >= 32.
 */
int
test_old_growth_bits(ps, index)
    struct pine *ps;
    int          index;
{
    /*
     * this list defines F_OLD_GROWTH set
     */
    static unsigned long old_growth_bits = ((1 << F_ENABLE_FULL_HDR)     |
					    (1 << F_ENABLE_PIPE)         |
					    (1 << F_ENABLE_TAB_COMPLETE) |
					    (1 << F_QUIT_WO_CONFIRM)     |
					    (1 << F_ENABLE_JUMP)         |
					    (1 << F_ENABLE_ALT_ED)       |
					    (1 << F_ENABLE_BOUNCE)       |
					    (1 << F_ENABLE_AGG_OPS)	 |
					    (1 << F_ENABLE_FLAG)         |
					    (1 << F_CAN_SUSPEND));
    if(index >= 32)
	return(0);

    if(index == F_OLD_GROWTH){
	for(index = 1; index <= F_LAST_FEATURE; index++)
	  if(((1 << index) & old_growth_bits) && F_OFF(index, ps))
	    return(0);

	return(1);
    }
    else
      return((1 << index) & old_growth_bits);
}



/*
 * Standard way to get at save message rules...
 */
NAMEVAL_S *
save_msg_rules(index)
    int index;
{
    static NAMEVAL_S save_rules[] = {
	{"by-from",				MSG_RULE_FROM},
	{"by-nick-of-from",			MSG_RULE_NICK_FROM_DEF},
	{"by-nick-of-from-then-from",		MSG_RULE_NICK_FROM},
	{"by-fcc-of-from",			MSG_RULE_FCC_FROM_DEF},
	{"by-fcc-of-from-then-from",		MSG_RULE_FCC_FROM},
	{"by-sender",				MSG_RULE_SENDER},
	{"by-nick-of-sender",			MSG_RULE_NICK_SENDER_DEF},
	{"by-nick-of-sender-then-sender",	MSG_RULE_NICK_SENDER},
	{"by-fcc-of-sender",			MSG_RULE_FCC_SENDER_DEF},
	{"by-fcc-of-sender-then-sender",	MSG_RULE_FCC_SENDER},
	{"by-recipient",			MSG_RULE_RECIP},
	{"by-nick-of-recip",			MSG_RULE_NICK_RECIP_DEF},
	{"by-nick-of-recip-then-recip",		MSG_RULE_NICK_RECIP},
	{"by-fcc-of-recip",			MSG_RULE_FCC_RECIP_DEF},
	{"by-fcc-of-recip-then-recip",		MSG_RULE_FCC_RECIP},
	{"last-folder-used",			MSG_RULE_LAST}, 
	{"default-folder",			MSG_RULE_DEFLT}
    };

    return((index >= 0 && index < (sizeof(save_rules)/sizeof(save_rules[0])))
	   ? &save_rules[index] : NULL);
}


/*
 * Standard way to get at fcc rules...
 */
NAMEVAL_S *
fcc_rules(index)
    int index;
{
    static NAMEVAL_S f_rules[] = {
	{"default-fcc",        FCC_RULE_DEFLT}, 
	{"last-fcc-used",      FCC_RULE_LAST}, 
	{"by-recipient",       FCC_RULE_RECIP},
	{"by-nickname",        FCC_RULE_NICK},
	{"by-nick-then-recip", FCC_RULE_NICK_RECIP},
	{"current-folder",     FCC_RULE_CURRENT}
    };

    return((index >= 0 && index < (sizeof(f_rules)/sizeof(f_rules[0])))
	   ? &f_rules[index] : NULL);
}


/*
 * Standard way to get at addrbook sort rules...
 */
NAMEVAL_S *
ab_sort_rules(index)
    int index;
{
    static NAMEVAL_S ab_rules[] = {
	{"fullname-with-lists-last",  AB_SORT_RULE_FULL_LISTS},
	{"fullname",                  AB_SORT_RULE_FULL}, 
	{"nickname-with-lists-last",  AB_SORT_RULE_NICK_LISTS},
	{"nickname",                  AB_SORT_RULE_NICK},
	{"dont-sort",                 AB_SORT_RULE_NONE}
    };

    return((index >= 0 && index < (sizeof(ab_rules)/sizeof(ab_rules[0])))
	   ? &ab_rules[index] : NULL);
}


/*
 * Standard way to get at goto default rules...
 */
NAMEVAL_S *
goto_rules(index)
    int index;
{
    static NAMEVAL_S g_rules[] = {
	{"inbox-or-folder-in-recent-collection", GOTO_INBOX_RECENT_CLCTN},
	{"inbox-or-folder-in-first-collection",	 GOTO_INBOX_FIRST_CLCTN},
	{"most-recent-folder",			 GOTO_LAST_FLDR}
    };

    return((index >= 0 && index < (sizeof(g_rules)/sizeof(g_rules[0])))
	   ? &g_rules[index] : NULL);
}


/*
 * The argument is an argument from the command line.  We check to see
 * if it is specifying an alternate value for one of the options that is
 * normally set in pinerc.  If so, we return 1 and set the appropriate
 * values in the variables array.
 */
int
pinerc_cmdline_opt(arg)
    char *arg;
{
    struct variable *v;
    char            *p,
                    *p1,
                    *value,
                   **lvalue;
    int              i;

    if(!arg || !arg[0])
      return 0;

    for(v = variables; v->name != NULL; v++){
      if(v->is_used && strncmp(v->name, arg, strlen(v->name)) == 0)
        break;
    }

    /* no match */
    if(v->name == NULL)
      return 0;

    if(v->is_obsolete || !v->is_user){
      fprintf(stderr, "Option \"%s\" is %s\n", v->name, v->is_obsolete ?
				       "obsolete" :
				       "not user settable");
      exit(-1);
    }

    /*----- Skip to '=' -----*/
    p1 = arg + strlen(v->name);
    while(*p1 && (*p1 == '\t' || *p1 == ' '))
      p1++;

    if(*p1 != '='){
      fprintf(stderr, "Missing \"=\" after -%s\n", v->name);
      exit(-1);
    }

    /* free mem */
    if(v->is_list){
      if(v->cmdline_val.l){
        char **p;
        for(p = v->cmdline_val.l; *p ; p++)
          fs_give((void **)p);
        fs_give((void **)&(v->cmdline_val.l));
      }
    }else{
      if(v->cmdline_val.p)
        fs_give((void **) &(v->cmdline_val.p));
    }

    p1++;

    /*----- Matched a variable, get its value ----*/
    while(*p1 == ' ' || *p1 == '\t')
      p1++;
    value = p1;

    if(*value == '\0'){
      if(v->is_list){
        v->cmdline_val.l = (char **)fs_get(2 * sizeof(char *));
	/*
	 * we let people leave off the quotes on command line so that
	 * they don't have to mess with shell quoting
	 */
        v->cmdline_val.l[0] = cpystr("");
        v->cmdline_val.l[1] = NULL;
      }else{
        v->cmdline_val.p = cpystr("");
      }
      return 1;
    }

    /*--value is non-empty--*/
    if(*value == '"' && !v->is_list){
      value++;
      for(p1 = value; *p1 && *p1 != '"'; p1++);
        if(*p1 == '"')
          *p1 = '\0';
        else
          removing_trailing_white_space(value);
    }else{
      removing_trailing_white_space(value);
    }

    if(v->is_list){
      int   was_quoted = 0,
            count      = 1;
      char *error      = NULL;

      for(p1=value; *p1; p1++){		/* generous count of list elements */
        if(*p1 == '"')			/* ignore ',' if quoted   */
          was_quoted = (was_quoted) ? 0 : 1;

        if(*p1 == ',' && !was_quoted)
          count++;
      }

      lvalue = parse_list(value, count, &error);
      if(error){
	fprintf(stderr, "%s in %s = \"%s\"\n", error, v->name, value);
	exit(-1);
      }
      /*
       * Special case: turn "" strings into empty strings.
       * This allows users to turn off default lists.  For example,
       * if smtp-server is set then a user could override smtp-server
       * with smtp-server="".
       */
      for(i = 0; lvalue[i]; i++)
	if(lvalue[i][0] == '"' &&
	   lvalue[i][1] == '"' &&
	   lvalue[i][2] == '\0')
	     lvalue[i][0] = '\0';
    }

    if(v->is_list)
      v->cmdline_val.l = lvalue;
    else
      v->cmdline_val.p = cpystr(value);

    return 1;
}


/*
 * Process the command list, changing function key notation into
 * lexical equivalents.
 */
void
process_init_cmds(ps, list)
    struct pine *ps;
    char **list;
{
    char **p;
    int i = 0;
    int j;
#define MAX_INIT_CMDS 500
    /* this is just a temporary stack array, the real one is allocated below */
    int i_cmds[MAX_INIT_CMDS];
    int fkeys = 0;
    int not_fkeys = 0;
  
    if(list){
      for(p = list; *p && i < MAX_INIT_CMDS; p++){

	/* regular character commands */
	if(strlen(*p) == 1){

	  i_cmds[i++] = **p;
	  not_fkeys++;

	}else if(strucmp(*p, "SPACE") == 0){
	    i_cmds[i++] = ' ';
	    not_fkeys++;

	}else if(strucmp(*p, "CR") == 0){
	    i_cmds[i++] = '\n';
	    not_fkeys++;

	}else if(strucmp(*p, "TAB") == 0){
	    i_cmds[i++] = '\t';
	    not_fkeys++;

	/* control chars */
	}else if(strlen(*p) == 2 && **p == '^'){

	    i_cmds[i++] = ctrl(*((*p)+1));

	/* function keys */
	}else{

	  fkeys++;

	  if(**p == 'F' || **p == 'f'){
	    int v;

	    v = atoi((*p)+1);
	    if(v >= 1 && v <= 12)
	      i_cmds[i++] = PF1 + v - 1;
	    else
	      i_cmds[i++] = KEY_JUNK;

	  }else if(strucmp(*p, "UP") == 0){
	    i_cmds[i++] = KEY_UP;
	  }else if(strucmp(*p, "DOWN") == 0){
	    i_cmds[i++] = KEY_DOWN;
	  }else if(strucmp(*p, "LEFT") == 0){
	    i_cmds[i++] = KEY_LEFT;
	  }else if(strucmp(*p, "RIGHT") == 0){
	    i_cmds[i++] = KEY_RIGHT;
	  }else{
	    fkeys--;
	    sprintf(tmp_20k_buf,
		    "Bad initial keystroke \"%s\" (missing comma?)\n", *p);
	    init_error(ps, tmp_20k_buf);
	  }
	}
      }
    }

    /*
     * We don't handle the case where function keys are used to specify the
     * commands but some non-function key input is also required.  For example,
     * you might want to jump to a specific message number and view it
     * on start up.  To do that, you need to use character commands instead
     * of function key commands in the initial-keystroke-list.
     */
    if(fkeys && not_fkeys){
	init_error(ps,
"Mixed characters and function keys in \"initial-keystroke-list\", skipping!");
	i = 0;
    }

    if(fkeys && !not_fkeys)
      F_TURN_ON(F_USE_FK,ps);
    if(!fkeys && not_fkeys)
      F_TURN_OFF(F_USE_FK,ps);

    ps->initial_cmds = (int *)fs_get((i+1) * sizeof(int));
    ps->free_initial_cmds = ps->initial_cmds;
    for(j = 0; j < i; j++)
	ps->initial_cmds[j] = i_cmds[j];
    ps->initial_cmds[i] = 0;
    if(i)
      ps->in_init_seq = ps->save_in_init_seq = 1;
}


/*
 * Choose from the global default, command line args, pinerc values to set
 * the actual value of the variable that we will use.  Start at the top
 * and work down from higher to lower precedence.
 */
void    
set_current_val(var, expand, cmdline)
    struct variable *var;
    int              expand, cmdline;
{
    int    is_set[4];  /* to see if we should warn user about fixed var */
    char **tmp;

    dprint(9, (debugfile,
	       "set_current_val(var num=%d, expand=%d, cmdline=%d)\n",
	       var - ps_global->vars, expand, cmdline));

    if(var->is_list){  /* variable is a list */
	char **list = NULL;
	int    i, j;

	dprint(9, (debugfile, "is_list: name=%s\n", var->name));

	/*
	 * It's possible for non-null list elements to expand to
	 * nothing, so we have to be a little more intelligent about
	 * pre-expanding the lists to make sure such lists are
	 * treated like empty lists...
	 */
	for(j = 0; j < 4; j++){
	    char **t;

	    /*
	     * The first one that's set wins.
	     */
	    t = j==0 ? var->fixed_val.l :
		j==1 ? (cmdline) ? var->cmdline_val.l : NULL :
		j==2 ? var->user_val.l :
		       var->global_val.l;

	    is_set[j] = 0;

	    if(t){
		if(!expand){
		    is_set[j]++;
		    if(!list)
		      list = t;
		}
		else{
		    for(i = 0; t[i]; i++){
			if(expand_variables(tmp_20k_buf, t[i])){
			    /* successful expand */
			    is_set[j]++;
			    if(!list)
			      list = t;

			    break;
			}
		    }
		}
	    }
	}

	/* Admin wants default, which is global_val. */
	if(var->is_fixed && var->fixed_val.l == NULL)
	  list = var->global_val.l;
		    
	if(var->current_val.l){	 /* clean up any old values */
	    for(tmp = var->current_val.l; *tmp ; tmp++)
	      fs_give((void **)tmp);

	    fs_give((void **)&var->current_val.l);
	}

	if(list){
	    int cnt = 0;

	    for(; list[cnt]; cnt++)	/* item count */
	      ;/* do nothing */

	    dprint(9, (debugfile, "counted %d items\n", cnt));

	    var->current_val.l = (char **)fs_get((cnt+1)*sizeof(char *));
	    tmp = var->current_val.l;

	    for(i = 0; list[i]; i++){
		if(!expand)
		  *tmp++ = cpystr(list[i]);
		else if(expand_variables(tmp_20k_buf, list[i]))
		  *tmp++ = cpystr(tmp_20k_buf);
	    }

	    *tmp = NULL;
	}
	else
	  var->current_val.l = NULL;

    }

    else{  /* variable is not a list */
	char *strvar = NULL;
	char *t;
	int   j;

	for(j = 0; j < 4; j++){

	    t = j==0 ? var->fixed_val.p :
		j==1 ? (cmdline) ? var->cmdline_val.p : NULL :
		j==2 ? var->user_val.p :
		       var->global_val.p;

	    is_set[j] = 0;

	    if(t){
		if(!expand){
		    is_set[j]++;
		    if(!strvar)
			strvar = t;
		}
		else if(expand_variables(tmp_20k_buf, t)){
		    is_set[j]++;
		    if(!strvar)
			strvar = t;
		}
	    }
	}

	/* Admin wants default, which is global_val. */
	if(var->is_fixed && var->fixed_val.p == NULL)
	  strvar = var->global_val.p;

	if(var->current_val.p)		/* free previous value */
	  fs_give((void **)&var->current_val.p);

	if(strvar){
	    if(!expand)
	      var->current_val.p = cpystr(strvar);
	    else{
		expand_variables(tmp_20k_buf, strvar);
		var->current_val.p = cpystr(tmp_20k_buf);
	    }
	}
	else
	  var->current_val.p = NULL;
    }

    if(var->is_fixed){
	char **list;
	int i, j, fixed_len, user_len;

	/*
	 * sys mgr fixed this variable and user is trying to change it
	 */
	if(is_set[2]){
	    if(var->is_list){
		/* If same length and same contents, don't warn. */
		for(list=var->fixed_val.l; list && *list; list++)
		  ;/* just counting */

		fixed_len = var->fixed_val.l ? (list - var->fixed_val.l) : 0;
		for(list=var->user_val.l; list && *list; list++)
		  ;/* just counting */

		user_len = var->user_val.l ? (list - var->user_val.l) : 0;
		if(user_len == fixed_len){
		  for(i=0; i < user_len; i++){
		    for(j=0; j < user_len; j++)
		      if(!strucmp(var->user_val.l[i], var->fixed_val.l[j]))
			break;
		      
		    if(j == user_len){
		      ps_global->give_fixed_warning = 1;
		      ps_global->fix_fixed_warning = 1;
		      break;
		    }
		  }
		}
		else{
		    ps_global->give_fixed_warning = 1;
		    ps_global->fix_fixed_warning = 1;
		}
	    }
	    else if(var->fixed_val.p && !var->user_val.p
		    || !var->fixed_val.p && var->user_val.p
		    || (var->fixed_val.p && var->user_val.p
		        && strucmp(var->fixed_val.p, var->user_val.p))){
		ps_global->give_fixed_warning = 1;
		ps_global->fix_fixed_warning = 1;
	    }
	}

	if(is_set[1]){
	    if(var->is_list){
		/* If same length and same contents, don't warn. */
		for(list=var->fixed_val.l; list && *list; list++)
		  ;/* just counting */

		fixed_len = var->fixed_val.l ? (list - var->fixed_val.l) : 0;
		for(list=var->cmdline_val.l; list && *list; list++)
		  ;/* just counting */

		user_len = var->cmdline_val.l ? (list - var->cmdline_val.l) : 0;
		if(user_len == fixed_len){
		  for(i=0; i < user_len; i++){
		    for(j=0; j < user_len; j++)
		      if(!strucmp(var->cmdline_val.l[i], var->fixed_val.l[j]))
			break;
		      
		    if(j == user_len){
		      ps_global->give_fixed_warning = 1;
		      break;
		    }
		  }
		}
		else{
		    ps_global->give_fixed_warning = 1;
		}
	    }
	    else if(var->fixed_val.p && !var->cmdline_val.p
		    || !var->fixed_val.p && var->cmdline_val.p
		    || (var->fixed_val.p && var->cmdline_val.p
		        && strucmp(var->fixed_val.p, var->cmdline_val.p))){
		ps_global->give_fixed_warning = 1;
	    }
	}
    }
}


/*
 * Feature-list has to be handled separately from the other variables
 * because it is additive.  The other variables choose one of command line,
 * or pine.conf, or pinerc.  Feature list adds them.  This could easily be
 * converted to a general purpose routine if we add more additive variables.
 *
 * This works by replacing earlier values with later ones.  That is, command
 * line settings have higher precedence than global settings and that is
 * accomplished by putting the command line features after the global
 * features in the list.  When they are processed, the last one wins.
 *
 * Feature-list also has a backwards compatibility hack.
 */
void    
set_feature_list_current_val(var)
  struct variable *var;
{
    char **list;
    char **list_fixed;
    int    i, j, k,
	   elems = 0;

    /* count the lists so I can allocate */
    if(list=var->global_val.l)
      for(i = 0; list[i]; i++)
        elems++;
    if(list=var->user_val.l)
      for(i = 0; list[i]; i++)
        elems++;
    if(list=var->cmdline_val.l)
      for(i = 0; list[i]; i++)
        elems++;
    if(list=ps_global->feat_list_back_compat)
      for(i = 0; list[i]; i++)
        elems++;
    if(list=var->fixed_val.l)
      for(i = 0; list[i]; i++)
        elems++;

    list_fixed = var->fixed_val.l;

    var->current_val.l = (char **)fs_get((elems+1) * sizeof(char *));

    j = 0;
    if(list=var->global_val.l)
      for(i = 0; list[i]; i++)
        var->current_val.l[j++] = cpystr(list[i]);
    /*
     * We need to warn the user if the sys mgr has restricted him or her
     * from changing a feature that he or she is trying to change.
     *
     * I'm not catching the old-growth macro since I'm just comparing
     * strings.  That is, it works correctly, but the user won't be warned
     * if the user old-growth and the mgr says no-quit-without-confirm.
     */
    if(list=var->user_val.l)
      for(i = 0; list[i]; i++){
        var->current_val.l[j++] = cpystr(list[i]);
        for(k = 0; list_fixed && list_fixed[k]; k++){
	  char *p, *q;
	  p = list[i];
	  q = list_fixed[k];
	  if(!struncmp(p, "no-", 3))
	    p += 3;
	  if(!struncmp(q, "no-", 3))
	    q += 3;
	  if(!strucmp(q, p) && strucmp(list[i], list_fixed[k])){
	      ps_global->give_fixed_warning = 1;
	      ps_global->fix_fixed_warning = 1;
	  }
	}
      }
    if(list=var->cmdline_val.l)
      for(i = 0; list[i]; i++){
        var->current_val.l[j++] = cpystr(list[i]);
        for(k = 0; list_fixed && list_fixed[k]; k++){
	  char *p, *q;
	  p = list[i];
	  q = list_fixed[k];
	  if(!struncmp(p, "no-", 3))
	    p += 3;
	  if(!struncmp(q, "no-", 3))
	    q += 3;
	  if(!strucmp(q, p) && strucmp(list[i], list_fixed[k]))
	    ps_global->give_fixed_warning = 1;
	}
      }
    if(list=var->fixed_val.l)
      for(i = 0; list[i]; i++)
        var->current_val.l[j++] = cpystr(list[i]);
    if(list=ps_global->feat_list_back_compat)
      for(i = 0; list[i]; i++)
        var->current_val.l[j++] = cpystr(list[i]);

    var->current_val.l[j] = NULL;
}
                                                     


/*----------------------------------------------------------------------

	Expand Metacharacters/variables in file-names

   Read input line and expand shell-variables/meta-characters

	<input>		<replaced by>
	${variable}	getenv("variable")
	$variable	getenv("variable")
	~		getenv("HOME")
	\c		c
	<others>	<just copied>

NOTE handling of braces in ${name} doesn't check much or do error recovery
	
  ----*/

char *
expand_variables(lineout, linein)
char *linein, *lineout;
{
    char *src = linein, *dest = lineout, *p;
    int  envexpand = 0;

    if(!linein)
      return(NULL);

    while( *src ){			/* something in input string */
#if defined(DOS) || defined(OS2)
        if(*src == '$' && *(src+1) == '$'){
	    /*
	     * backslash to escape chars we're interested in, else
	     * it's up to the user of the variable to handle the 
	     * backslash...
	     */
            *dest++ = *++src;		/* copy next as is */
        }else
#else
        if(*src == '\\' && *(src+1) == '$'){
	    /*
	     * backslash to escape chars we're interested in, else
	     * it's up to the user of the variable to handle the 
	     * backslash...
	     */
            *dest++ = *++src;		/* copy next as is */
        }else if(*src == '~' && src == linein){
	    char buf[MAXPATH];
	    int  i;

	    for(i = 0; src[i] && src[i] != '/'; i++)
	      buf[i] = src[i];

	    src    += i;		/* advance src pointer */
	    buf[i]  = '\0';		/* tie off buf string */
	    fnexpand(buf, MAXPATH);	/* expand the path */

	    for(p = buf; *dest = *p; p++, dest++)
	      ;

	    continue;
        }else
#endif
	if(*src == '$'){		/* shell variable */
	    char word[128];
	    int  found_brace = 0;

	    envexpand++;		/* signal that we've expanded a var */
	    src++;			/* skip dollar */
	    p = word;
	    if(*src == '{'){		/* starts with brace? */
		src++;        
		found_brace = 1;
	    }

	    while(*src && !isspace((unsigned char)*src)
		  && (!found_brace || *src != '}'))
	      *p++ = *src++;		/* copy to word */

	    if(found_brace){	/* look for closing  brace */
		while(*src && *src != '}')
		  src++;		/* skip until brace */

		if(*src == '}')	/* skip brace */
		  src++;
	    }

	    *p = '\0';			/* tie off word */

	    if(p = getenv(word)) 	/* check for word in environment */
	      while(*p)
		*dest++ = *p++;

	    continue;
	}else{				/* other cases: just copy */
	    *dest++ = *src;
	}

        if(*src)			/* next character (if any) */
	  src++;
    }

    *dest = '\0';
    return((envexpand && lineout[0] == '\0') ? NULL : lineout);
}


/*----------------------------------------------------------------------
    Sets  login, full_username and home_dir

   Args: ps -- The Pine structure to put the user name, etc in

  Result: sets the fullname, login and home_dir field of the pine structure
          returns 0 on success, -1 if not.
  ----*/
#define	MAX_INIT_ERRS	5
void
init_error(ps, s)
    struct pine *ps;
    char	*s;
{
    int    i;

    if(!ps->init_errs){
	ps->init_errs = (char **)fs_get((MAX_INIT_ERRS + 1) * sizeof(char *));
	memset(ps->init_errs, 0, (MAX_INIT_ERRS + 1) * sizeof(char *));
    }

    for(i = 0; i < MAX_INIT_ERRS; i++)
      if(!ps->init_errs[i]){
	  ps->init_errs[i] = cpystr(s);
	  dprint(2, (debugfile, "%s\n", s));
	  break;
      }
}


/*----------------------------------------------------------------------
    Sets  login, full_username and home_dir

   Args: ps -- The Pine structure to put the user name, etc in

  Result: sets the fullname, login and home_dir field of the pine structure
          returns 0 on success, -1 if not.
  ----*/

init_username(ps)
     struct pine *ps;
{
    char fld_dir[MAXPATH+1], *expanded;
    int  rv;

    rv       = 0;
    expanded = NULL;
#if defined(DOS) || defined(OS2)
    if(ps->COM_USER_ID)
      expanded = expand_variables(tmp_20k_buf,
				  ps->COM_USER_ID);
    
    if(!expanded && ps->USR_USER_ID)
      expanded = expand_variables(tmp_20k_buf, ps->USR_USER_ID);

    if(!expanded)
      ps->blank_user_id = 1;

    ps->VAR_USER_ID = cpystr(expanded ? expanded : "");
#else
    ps->VAR_USER_ID = cpystr(ps->ui.login);
    if(!ps->VAR_USER_ID[0]){
        fprintf(stderr, "Who are you? (Unable to look up login name)\n");
        rv = -1;
    }
#endif

    expanded = NULL;
    if(ps->vars[V_PERSONAL_NAME].is_fixed){
	if(ps->FIX_PERSONAL_NAME){
            expanded = expand_variables(tmp_20k_buf, ps->FIX_PERSONAL_NAME);
	}
	if(ps->USR_PERSONAL_NAME){
	    ps_global->give_fixed_warning = 1;
	    ps_global->fix_fixed_warning = 1;
	}
	else if(ps->COM_PERSONAL_NAME)
	  ps_global->give_fixed_warning = 1;
    }
    else{
	if(ps->COM_PERSONAL_NAME)
	  expanded = expand_variables(tmp_20k_buf,
				ps->COM_PERSONAL_NAME);

	if(!expanded && ps->USR_PERSONAL_NAME)
	  expanded = expand_variables(tmp_20k_buf,
				      ps->USR_PERSONAL_NAME);
    }

    if(!expanded){
	expanded = ps->ui.fullname;
#if defined(DOS) || defined(OS2)
	ps->blank_personal_name = 1;
#endif
    }

    ps->VAR_PERSONAL_NAME = cpystr(expanded ? expanded : "");

    if(strlen(ps->home_dir) + strlen(ps->VAR_MAIL_DIRECTORY)+2 > MAXPATH){
        printf("Folders directory name is longer than %d\n", MAXPATH);
        printf("Directory name: \"%s/%s\"\n",ps->home_dir,
               ps->VAR_MAIL_DIRECTORY);
        return(-1);
    }
#if defined(DOS) || defined(OS2)
    if(ps->VAR_MAIL_DIRECTORY[1] == ':')
      strcpy(fld_dir, ps->VAR_MAIL_DIRECTORY);
    else
#endif
    build_path(fld_dir, ps->home_dir, ps->VAR_MAIL_DIRECTORY);
    ps->folders_dir = cpystr(fld_dir);

    dprint(1, (debugfile, "Userid: %s\nFullname: \"%s\"\n",
               ps->VAR_USER_ID, ps->VAR_PERSONAL_NAME));
    return(rv);
}


/*----------------------------------------------------------------------
        Fetch the hostname of the current system and put it in pine struct

   Args: ps -- The pine structure to put the hostname, etc in

  Result: hostname, localdomain, userdomain and maildomain are set


** Pine uses the following set of names:
  hostname -    The fully-qualified hostname.  Obtained with
		gethostbyname() which reads /etc/hosts or does a DNS
		lookup.  This may be blank.
  localdomain - The domain name without the host.  Obtained from the
		above hostname if it has a "." in it.  Removes first
		segment.  If hostname has no "." in it then the hostname
		is used.  This may be blank.
  userdomain -  Explicitly configured domainname.  This is read out of the
		global pine.conf or user's .pinerc.  The user's entry in the
		.pinerc overrides.

** Pine has the following uses for such names:

  1. On outgoing messages in the From: line
	Uses userdomain if there is one.  If not uses, uses
	hostname unless Pine has been configured to use localdomain.

  2. When expanding/fully-qualifying unqualified addresses during
     composition
	(same as 1)

  3. When expanding/fully-qualifying unqualified addresses during
     composition when a local entry in the password file exists for
     name.
        If no userdomain is given, then this lookup is always done
	and the hostname is used unless Pine's been configured to 
	use the localdomain.  If userdomain is defined, it is used,
	but no local lookup is done.  We can't assume users on the
	local host are valid in the given domain (and, for simplicity,
	have chosen to ignore the cases userdomain matches localdomain
	or localhost).  Setting user-lookup-even-if-domain-mismatch
	feature will tell pine to override this behavior and perform
	the local lookup anyway.  The problem of a global "even-if"
	set and a .pinerc-defined user-domain of something odd causing
	the local lookup, but this will only effect the personal name, 
	and is not judged to be a significant problem.

  4. In determining if an address is that of the current pine user for
     formatting index and filtering addresses when replying
	If a userdomain is specified the address must match the
	userdomain exactly.  If a userdomain is not specified or the
	userdomain is the same as the hostname or domainname, then
	an address will be considered the users if it matches either
	the domainname or the hostname.  Of course, the userid must
	match too. 

  5. In Message ID's
	The fully-qualified hostname is always users here.


** Setting the domain names
  To set the domain name for all Pine users on the system to be
different from what Pine figures out from DNS, set the domain name in
the "user-domain" variable in pine.conf.  To set the domain name for an
individual user, set the "user-domain" variable in his .pinerc.
The .pinerc setting overrides any other setting.
 ----*/
init_hostname(ps)
     struct pine *ps;
{
    char hostname[MAX_ADDRESS+1], domainname[MAX_ADDRESS+1];

    getdomainnames(hostname, MAX_ADDRESS, domainname, MAX_ADDRESS);

    if(ps->hostname)
      fs_give((void **)&ps->hostname);

    ps->hostname = cpystr(hostname);

    if(ps->localdomain)
      fs_give((void **)&ps->localdomain);

    ps->localdomain = cpystr(domainname);
    ps->userdomain  = NULL;

    if(ps->VAR_USER_DOMAIN && ps->VAR_USER_DOMAIN[0]){
        ps->maildomain = ps->userdomain = ps->VAR_USER_DOMAIN;
    }else{
#if defined(DOS) || defined(OS2)
	if(ps->VAR_USER_DOMAIN)
	  ps->blank_user_domain = 1;	/* user domain set to null string! */

        ps->maildomain = ps->localdomain[0] ? ps->localdomain : ps->hostname;
#else
        ps->maildomain = strucmp(ps->VAR_USE_ONLY_DOMAIN_NAME, "yes")
			  ? ps->hostname : ps->localdomain;
#endif
    }

    /*
     * Tell c-client what domain to use when completing unqualified
     * addresses it finds in local mailboxes.  Remember, it won't 
     * affect what's to the right of '@' for unqualified addresses in
     * remote folders...
     */
    mail_parameters(NULL, SET_LOCALHOST, (void *) ps->maildomain);
    if(!strchr(ps->maildomain, '.')){
	sprintf(tmp_20k_buf, "Incomplete maildomain \"%s\".",
		ps->maildomain);
	init_error(ps, tmp_20k_buf);
	strcpy(tmp_20k_buf,
	       "Return address in mail you send may be incorrect.");
	init_error(ps, tmp_20k_buf);
    }

    dprint(1, (debugfile,"User domain name being used \"%s\"\n",
               ps->userdomain == NULL ? "" : ps->userdomain));
    dprint(1, (debugfile,"Local Domain name being used \"%s\"\n",
               ps->localdomain));
    dprint(1, (debugfile,"Host name being used \"%s\"\n",
               ps->hostname));
    dprint(1, (debugfile,
	       "Mail Domain name being used (by c-client too)\"%s\"\n",
               ps->maildomain));

    if(!ps->maildomain || !ps->maildomain[0]){
#if defined(DOS) || defined(OS2)
	if(ps->blank_user_domain)
	  return(0);		/* prompt for this in send.c:dos_valid_from */
#endif
        fprintf(stderr, "No host name or domain name set\n");
        return(-1);
    }
    else
      return(0);
}


/*----------------------------------------------------------------------
         Read and parse a pinerc file
  
   Args:  Filename   -- name of the .pinerc file to open and read
          vars       -- The vars structure to store values in
          which_vars -- Whether the local or global values are being read

   Result: 

 This may be the local file or the global file.  The values found are
merged with the values currently in vars.  All values are strings and
are malloced; and existing values will be freed before the assignment.
Those that are <unset> will be left unset; their values will be NULL.
  ----*/
void
read_pinerc(filename, vars, which_vars)
     char *filename;
     ParsePinerc which_vars;
     struct variable *vars;
{
    char               *file, *value, **lvalue, *line, *error;
    register char      *p, *p1;
    struct variable    *v;
    struct pinerc_line *pline;
    int                 line_count, was_quoted;
    int			i;

    dprint(1, (debugfile, "reading_pinerc \"%s\"\n", filename));

    file = read_file(filename);
    if(file == NULL){
        dprint(1, (debugfile, "Open failed: %s\n", error_description(errno)));
	if(which_vars == ParseLocal)
	    ps_global->first_time_user = 1;
        return;
    }
    else{
	/*
	 * fixup unix newlines?  note: this isn't a problem under dos
	 * since the file's read in text mode.
	 */
	for(p = file; *p && *p != '\012'; p++)
	  ;

	if(p > file && *p && *(p-1) == '\015')	/* cvt crlf to lf */
	  for(p1 = p - 1; *p1 = *p; p++)
	    if(!(*p == '\015' && *(p+1) == '\012'))
	      p1++;
    }

    dprint(1, (debugfile, "Read %d characters:\n", strlen(file)));

    if(which_vars == ParseLocal){
      /*--- Count up lines and allocate structures */
      for(line_count = 0, p = file; *p != '\0'; p++)
        if(*p == '\n')
	  line_count++;
      pinerc_lines = (struct pinerc_line *)
               fs_get((3 + line_count) * sizeof(struct pinerc_line));
      memset((void *)pinerc_lines, 0,
			    (3 + line_count) * sizeof(struct pinerc_line));
      pline = pinerc_lines;

      /* copyright stuff, sorry, magic numbers galore */
      p = file;
      if(which_vars == ParseLocal){
        if(strncmp(p, cf_text_comment1, strlen(cf_text_comment1)) == 0){
	  /* find the comma after the version number */
	  p = strchr(p+strlen(cf_text_comment1), ',');
          if(p && strncmp(p, cf_text_comment2, 11) == 0){
            p += strlen(cf_text_comment2) - 26;
            if(p &&
	     strncmp(p, cf_text_comment2+strlen(cf_text_comment2)-26,26) == 0){
	       copyright_line_is_there = 1;
	    }
	  }
        }
        p = strchr(p, '\n');
        if(p && *(p+1) && strncmp(p+1, cf_text_comment3, 27) == 0)
          trademark_lines_are_there = 1;
      }
    }

    for(p = file, line = file; *p != '\0';){
        /*----- Grab the line ----*/
        line = p;
        while(*p && *p != '\n')
          p++;
        if(*p == '\n'){
            *p++ = '\0';
        }

        /*----- Comment Line -----*/
        if(*line == '#'){
            if(which_vars == ParseLocal){
                pline->is_var = 0;
                pline->line = cpystr(line);
                pline++;
            }
            continue;
        }

	if(*line == '\0' || *line == '\t' || *line == ' '){
            p1 = line;
            while(*p1 == '\t' || *p1 == ' ')
               p1++;
            if(which_vars == ParseLocal){
	        if(*p1 != '\0') /* contin. line from some future version? */
                    pline->line = cpystr(line);
	        else
                    pline->line = cpystr("");
               pline->is_var = 0;
               pline++;
            }
            continue;
	}

        /*----- look up matching 'v' and leave "value" after '=' ----*/
        for(v = vars; *line && v->name; v++)
	  if((i = strlen(v->name)) < strlen(line) && !strncmp(v->name,line,i)){
	      int j;

	      for(j = i; line[j] == ' ' || line[j] == '\t'; j++)
		;

	      if(line[j] == '='){	/* bingo! */
		  for(value = &line[j+1];
		      *value == ' ' || *value == '\t';
		      value++)
		    ;

		  break;
	      }
	      /* else either unrecognized var or bogus line */
	  }

        /*----- Didn't match any variable or bogus format -----*/
        if(!v->name){
            if(which_vars == ParseLocal){
                pline->is_var = 0;
                pline->line = cpystr(line);
                pline++;
            }
            continue;
        }

        /*----- Obsolete variable, read it anyway below, might use it -----*/
        if(v->is_obsolete){
            if(which_vars == ParseLocal){
                pline->obsolete_var = 1;
                pline->line = cpystr(line);
                pline->var = v;
            }
        }

        /*----- Variable is in the list but unused for some reason -----*/
        if(!v->is_used){
            if(which_vars == ParseLocal){
                pline->is_var = 0;
                pline->line = cpystr(line);
                pline++;
            }
            continue;
        }

        /*--- Var is not user controlled, leave it alone for back compat ---*/
        if(!v->is_user && which_vars == ParseLocal){
	    pline->is_var = 0;
	    pline->line = cpystr(line);
	    pline++;
	    continue;
        }

	if(which_vars == ParseFixed)
	    v->is_fixed = 1;

        /*---- variable is unset, or it global but expands to nothing ----*/
        if(!*value
	   || (which_vars == ParseGlobal
	       && !expand_variables(tmp_20k_buf, value))){
            if(v->is_user && which_vars == ParseLocal){
                pline->is_var   = 1;
                pline->var = v;
                pline++;
            }
            continue;
        }

        /*--value is non-empty, store it handling quotes and trailing space--*/
        if(*value == '"' && !v->is_list){
            was_quoted = 1;
            value++;
            for(p1 = value; *p1 && *p1 != '"'; p1++);
            if(*p1 == '"')
              *p1 = '\0';
            else
              removing_trailing_white_space(value);
        }else{
            removing_trailing_white_space(value);
            was_quoted = 0;
        }

	/*
	 * List Entry Parsing
	 *
	 * The idea is to parse a comma separated list of 
	 * elements, preserving quotes, and understanding
	 * continuation lines (that is ',' == "\n ").
	 * Quotes must be balanced within elements.  Space 
	 * within elements is preserved, but leading and trailing 
	 * space is trimmed.  This is a generic function, and it's 
	 * left to the the functions that use the lists to make sure
	 * they contain valid data...
	 */
	if(v->is_list){

	    was_quoted = 0;
	    line_count = 0;
	    p1         = value;
	    while(1){			/* generous count of list elements */
		if(*p1 == '"')		/* ignore ',' if quoted   */
		  was_quoted = (was_quoted) ? 0 : 1 ;

		if((*p1 == ',' && !was_quoted) || *p1 == '\n' || *p1 == '\0')
		  line_count++;		/* count this element */

		if(*p1 == '\0' || *p1 == '\n'){	/* deal with EOL */
		    if(p1 < p || *p1 == '\n'){
			*p1++ = ','; 	/* fix null or newline */

			if(*p1 != '\t' && *p1 != ' '){
			    *(p1-1) = '\0'; /* tie off list */
			    p       = p1;   /* reset p */
			    break;
			}
		    }else{
			p = p1;		/* end of pinerc */
			break;
		    }
		}else
		  p1++;
	    }

	    error  = NULL;
	    lvalue = parse_list(value, line_count, &error);
	    if(error){
		dprint(1, (debugfile,
		       "read_pinerc: ERROR: %s in %s = \"%s\"\n", 
			   error, v->name, value));
	    }
	    /*
	     * Special case: turn "" strings into empty strings.
	     * This allows users to turn off default lists.  For example,
	     * if smtp-server is set then a user could override smtp-server
	     * with smtp-server="".
	     */
	    for(i = 0; lvalue[i]; i++)
		if(lvalue[i][0] == '"' &&
		   lvalue[i][1] == '"' &&
		   lvalue[i][2] == '\0')
		     lvalue[i][0] = '\0';
	}

        if(which_vars == ParseLocal){
            if(v->is_user){
		if(v->is_list){
		    if(v->user_val.l){
			char **p;
			for(p = v->user_val.l; *p ; p++)
			  fs_give((void **)p);

			fs_give((void **)&(v->user_val.l));
		    }

		    v->user_val.l = lvalue;
		}else{
		    if(v->user_val.p != NULL)
		      fs_give((void **) &(v->user_val.p));

		    v->user_val.p = cpystr(value);

		}

		pline->is_var    = 1;
		pline->var  = v;
		pline->is_quoted = was_quoted;
		pline++;
            }
        }else if(which_vars == ParseGlobal){
            if(v->is_global){
		if(v->is_list){
		    if(v->global_val.l){
			char **p;
			for(p = v->global_val.l; *p ; p++)
			  fs_give((void **)p);

			fs_give((void **)&(v->global_val.l));
		    }

		    v->global_val.l = lvalue;
		}else{
		    if(v->global_val.p != NULL)
		      fs_give((void **) &(v->global_val.p));
		    v->global_val.p = cpystr(value);
		}
            }
        }else{  /* which_vars == ParseFixed */
            if(v->is_user || v->is_global){
		if(v->is_list){
		    if(v->fixed_val.l){
			char **p;
			for(p = v->fixed_val.l; *p ; p++)
			  fs_give((void **)p);

			fs_give((void **)&(v->fixed_val.l));
		    }

		    v->fixed_val.l = lvalue;
		}else{
		    if(v->fixed_val.p != NULL)
		      fs_give((void **) &(v->fixed_val.p));
		    v->fixed_val.p = cpystr(value);
		}
	    }
	}

#ifdef DEBUG
	if(v->is_list){
	    char **t;
	    t =   which_vars == ParseLocal  ? v->user_val.l
	        : which_vars == ParseGlobal ? v->global_val.l
		:                             v->fixed_val.l;
	    if(t && *t && **t){
                dprint(3,(debugfile, " %20.20s : %s\n", v->name, *t));
	        while(++t && *t && **t)
                    dprint(3,(debugfile, " %20.20s : %s\n", "", *t));
	    }
	}else{
	    char *t;
	    t =   which_vars == ParseLocal  ? v->user_val.p
	        : which_vars == ParseGlobal ? v->global_val.p
		:                             v->fixed_val.p;
	    if(t && *t)
                dprint(3,(debugfile, " %20.20s : %s\n", v->name, t));
	}
#endif /* DEBUG */
    }
    if(which_vars == ParseLocal){
        pline->line = NULL;
        pline->is_var = 0;
    }
    fs_give((void **)&file);
}


/*
 * parse_list - takes a comma delimited list of "count" elements and 
 *              returns an array of pointers to each element neatly
 *              malloc'd in its own array.  Any errors are returned
 *              in the string pointed to by "error"
 *
 *  NOTE: only recognizes escaped quotes
 */
char **
parse_list(list, count, error)
    char *list, **error;
    int   count;
{
    char **lvalue, *p2, *p3, *p4;
    int    was_quoted = 0;

    lvalue = (char **)fs_get((count+1)*sizeof(char *));
    count  = 0;
    while(*list){			/* pick elements from list */
	p2 = list;		/* find end of element */
	while(1){
	    if(*p2 == '"')	/* ignore ',' if quoted   */
	      was_quoted = (was_quoted) ? 0 : 1 ;

	    if(*p2 == '\\' && *(p2+1) == '"')
	      p2++;		/* preserve escaped quotes, too */

	    if((*p2 == ',' && !was_quoted) || *p2 == '\0')
	      break;

	    p2++;
	}

	if(was_quoted){		/* unbalanced parens! */
	    if(error)
	      *error = "Unbalanced parentheses";

	    break;
	}

	/*
	 * if element found, eliminate trailing 
	 * white space and tie into variable list
	 */
	if(p2 != list){
	    for(p3 = p2 - 1; isspace((unsigned char)*p3) && list < p3; p3--)
	      ;

	    p4 = fs_get(((p3 - list) + 2) * sizeof(char));
	    lvalue[count++] = p4;
	    while(list <= p3)
	      *p4++ = *list++;

	    *p4 = '\0';
	}

	if(*(list = p2) != '\0'){	/* move to beginning of next val */
	    while(*list == ',' || isspace((unsigned char)*list))
	      list++;
	}
    }

    lvalue[count] = NULL;		/* tie off pointer list */
    return(lvalue);
}


static char quotes[3] = {'"', '"', '\0'};
/*----------------------------------------------------------------------
    Write out the .pinerc state information

   Args: ps -- The pine structure to take state to be written from

  This writes to a temporary file first, and then renames that to 
 be the new .pinerc file to protect against disk error.  This has the 
 problem of possibly messing up file protections, ownership and links.
  ----*/
write_pinerc(ps)
    struct pine *ps;
{
    char                buf[MAXPATH+1], copyright[90], *p, *dir, *tmp;
    int			write_trademark = 0;
    FILE               *f;
    struct pinerc_line *pline;
    struct variable    *var;

    dprint(2,(debugfile,"---- write_pinerc ----\n"));

    /* don't write if pinerc is read-only */
    if(ps->readonly_pinerc ||
         (can_access(ps->pinerc, ACCESS_EXISTS) == 0 &&
          can_access(ps->pinerc, EDIT_ACCESS) != 0)){
	ps->readonly_pinerc = 1;
	dprint(2,
	    (debugfile,"write_pinerc: fail because can't access pinerc\n"));
	return(-1);
    }

    dir = ".";
    if(p = last_cmpnt(ps->pinerc)){
	*--p = '\0';
	dir = ps->pinerc;
    }

#if	defined(DOS) || defined(OS2)
    if(!(isalpha((unsigned char)dir[0]) && dir[1] == ':' && dir[2] == '\0')
       && (can_access(dir, EDIT_ACCESS) < 0 &&
#ifdef	DOS
	   mkdir(dir) < 0)){
#else
	   mkdir(dir,0700) < 0)) {
#endif
	q_status_message2(SM_ORDER | SM_DING, 3, 5,
			  "Error creating \"%s\" : %s", dir,
			  error_description(errno));
	return(-1);
    }

    tmp = temp_nam(dir, "rc");
    if(p)
      *p = '\\';

    if(tmp == NULL)
      goto io_err;

    strcpy(buf, tmp);
    free(tmp);
    f = fopen(buf, "wt");
#else  /* !DOS */
    tmp = temp_nam((*dir) ? dir : "/", "pinerc");
    if(p)
      *p = '/';

    if(tmp == NULL)
      goto io_err;

    strcpy(buf, tmp);
    free(tmp);
    f = fopen(buf, "w");
#endif  /* !DOS */

    if(f == NULL) 
      goto io_err;

    for(var = ps->vars; var->name != NULL; var++) 
      var->been_written = 0;

    /* copyright stuff starts here */
    pline = pinerc_lines;
    if(ps->first_time_user ||
       (ps->show_new_version && !trademark_lines_are_there))
      write_trademark = 1;

    /* if it was already there, remove old one */
    if(copyright_line_is_there)
      pline++;

    if(fprintf(f, "%s%s%s", cf_text_comment1, pine_version,
	       cf_text_comment2) == EOF)
      goto io_err;
    if(write_trademark && fprintf(f, "%s%s", cf_text_comment3,
				  ps->first_time_user ? "" : "\n") == EOF)
      goto io_err;
    /* end of copyright stuff */

    /* Write out what was in the .pinerc */
    if(pline != NULL){
	for(; pline->is_var || pline->line != NULL; pline++){
	    if(pline->is_var && !pline->obsolete_var){
		var = pline->var;
		if((var->is_list && (!var->user_val.l || !var->user_val.l[0]))
		   || (!var->is_list && !var->user_val.p)){
		    if(fprintf(f, "%s=\n", pline->var->name) == EOF)
		      goto io_err;
		}
		else if((var->is_list && *var->user_val.l[0] == '\0')
			 || (!var->is_list && *var->user_val.p == '\0')){
		    if(fprintf(f, "%s=%s\n", pline->var->name, quotes) == EOF)
		      goto io_err;
		}
		else{
		    if(var->is_list){
			int i = 0;

			for(i = 0; var->user_val.l[i]; i++)
			  if(fprintf(f, "%s%s%s%s\n",
				     (i) ? "\t" : var->name,
				     (i) ? "" : "=",
				     var->user_val.l[i][0] 
				       ? var->user_val.l[i] : quotes,
				     var->user_val.l[i+1] ? ",":"") == EOF)
			    goto io_err;
		    }
		    else{
			if(fprintf(f, "%s=%s%s%s\n", pline->var->name,
				   (pline->is_quoted
				    && *pline->var->user_val.p != '\"')
				     ? "\"" : "",
				   pline->var->user_val.p,
				   (pline->is_quoted
				    && *pline->var->user_val.p != '\"')
				     ? "\"" : "") == EOF)
			  goto io_err;
		    }
		}

		pline->var->been_written = 1;
	    }else{
		/*
		 * The description text should be changed into a message
		 * about the variable being obsolete when a variable is
		 * moved to obsolete status.  We add that message before
		 * the variable unless it is already there.  However, we
		 * leave the variable itself in case the user runs an old
		 * version of pine again.  Note that we have read in the
		 * value of the variable in read_pinerc and translated it
		 * into a new variable if appropriate.
		 */
		if(pline->obsolete_var){
		    if(pline <= pinerc_lines || (pline-1)->line == NULL ||
		       strlen((pline-1)->line) < 3 ||
		       strucmp((pline-1)->line+2, pline->var->descrip) != 0)
		      if(fprintf(f, "# %s\n", pline->var->descrip) == EOF)
			goto io_err;
		}

		if(fprintf(f, "%s\n", pline->line) == EOF)
		  goto io_err;
	    }
	}
    }

    /* Now write out all the variables not in the .pinerc */
    for(var = ps->vars; var->name != NULL; var++){
	if(!var->is_user || var->been_written || !var->is_used
	   || var->is_obsolete)
	  continue;

	dprint(5,(debugfile,"write_pinerc: %s = %s\n", var->name,
		  var->user_val.p ? var->user_val.p : "<not set>"));

	/* Add extra comments about categories of variables */
	if(ps->first_time_user){
	    if(var == &variables[V_PERSONAL_NAME]){
		if(fprintf(f, "\n%s\n", cf_before_personal_name) == EOF)
		  goto io_err;
	    }
	    else if(var == &variables[V_INCOMING_FOLDERS]){
		if(fprintf(f, "\n%s\n", cf_before_incoming_folders) == EOF)
		  goto io_err;
	    }
	    else if(var == &variables[V_FEATURE_LIST]){
		if(fprintf(f, "\n%s\n", cf_before_feature_list) == EOF)
		  goto io_err;
	    }
	    else if(var == &variables[V_PRINTER]){
		if(fprintf(f, "\n%s\n", cf_before_printer) == EOF)
		  goto io_err;
	    }
	}

	/*
	 * set description to NULL to eliminate preceding
	 * blank and comment line.
	 */
	if(var->descrip && *var->descrip
	   && fprintf(f, "\n# %s\n", var->descrip) == EOF)
	  goto io_err;

	if((var->is_list && (!var->user_val.l
			     || (!var->user_val.l[0] && !var->global_val.l)))
	   || (!var->is_list && !var->user_val.p)){
	    if(fprintf(f, "%s=\n", var->name) == EOF)
	      goto io_err;
	}
	else if((var->is_list
		 && (!var->user_val.l[0] || !var->user_val.l[0][0]))
		|| (!var->is_list && var->user_val.p[0] == '\0')){
	    if(fprintf(f, "%s=\"\"\n", var->name) == EOF)
	      goto io_err;
	}
	else if(var->is_list){
	    int i = 0;

	    for(i = 0; var->user_val.l[i] ; i++)
	      if(fprintf(f, "%s%s%s%s\n", (i) ? "\t" : var->name,
			 (i) ? "" : "=", var->user_val.l[i],
			 var->user_val.l[i+1] ? ",":"") == EOF)
		goto io_err;
	}
	else{
	    if(fprintf(f, "%s=%s\n", var->name, var->user_val.p) == EOF)
	      goto io_err;
	}
    }

    if(fclose(f) == EOF || rename_file(buf, ps->pinerc) < 0)
      goto io_err;

    ps->outstanding_pinerc_changes = 0;

    return(0);

  io_err:
    q_status_message2(SM_ORDER | SM_DING, 3, 5,
		      "Error saving configuration in file \"%s\": %s",
		      ps->pinerc, error_description(errno));
    dprint(1, (debugfile, "Error writing %s : %s\n", ps->pinerc,
	       error_description(errno)));
    return(-1);
}


/*------------------------------------------------------------
  Return TRUE if the given string was a feature name present in the
  pinerc as it was when pine was started...
  ----*/
var_in_pinerc(s)
char *s;
{
    struct pinerc_line *pline;
    for(pline = pinerc_lines; pline->var || pline->line; pline++)
      if(pline->var && pline->var->name && !strucmp(s, pline->var->name))
	return(1);

    return(0);
}



/*------------------------------------------------------------
  Free resources associated with static pinerc_lines data
 ----*/
void
free_pinerc_lines()
{
    struct pinerc_line *pline;

    if(pline = pinerc_lines){
	for( ; pline->var || pline->line; pline++)
	  if(pline->line)
	    fs_give((void **)&pline->line);

	fs_give((void **)&pinerc_lines);
    }
}



/*
 * This is only used at startup time.  It sets ps->def_sort and
 * ps->def_sort_rev.  The syntax of the sort_spec is type[/reverse].
 * A reverse without a type is the same as arrival/reverse.  A blank
 * argument also means arrival/reverse.
 */
int
decode_sort(ps, sort_spec)
     struct pine *ps;
     char        *sort_spec;
{
    char *sep;
    int    x, reverse;

    if(*sort_spec == '\0' ||
		   struncmp(sort_spec, "reverse", strlen(sort_spec)) == 0){
	ps->def_sort_rev = 1;
        return(0);
    }
     
    reverse = 0;
    if((sep = strindex(sort_spec, '/')) != NULL){
        *sep = '\0';
        sep++;
        if(struncmp(sep, "reverse", strlen(sep)) == 0)
          reverse = 1;
        else
          return(-1);
    }

    for(x = 0; ps_global->sort_types[x] != EndofList; x++)
      if(struncmp(sort_name(ps_global->sort_types[x]),
                  sort_spec, strlen(sort_spec)) == 0)
        break;

    if(ps_global->sort_types[x] == EndofList)
      return(-1);

    ps->def_sort     = ps_global->sort_types[x];
    ps->def_sort_rev = reverse;
    return(0);
}


/*------------------------------------------------------------
    Dump out a global pine.conf on the standard output with fresh
    comments.  Preserves variables currently set in SYSTEM_PINERC, if any.
  ----*/
void
dump_global_conf()
{
     FILE            *f;
     struct variable *var;

     read_pinerc(SYSTEM_PINERC, variables, ParseGlobal);

     f = stdout;
     if(f == NULL) 
       goto io_err;

     fprintf(f, "#      %s -- system wide pine configuration\n#\n",
	     SYSTEM_PINERC);
     fprintf(f, "# Values here affect all pine users unless they've overidden the values\n");
     fprintf(f, "# in their .pinerc files.  A copy of this file with current comments may\n");
     fprintf(f, "# be obtained by running \"pine -conf\". It will be printed to standard output.\n#\n");
     fprintf(f,"# For a variable to be unset its value must be null/blank.  This is not the\n");
     fprintf(f,"# same as the value of \"empty string\", which can be used to effectively\n");
     fprintf(f,"# \"unset\" a variable that has a default or previously assigned value.\n");
     fprintf(f,"# To set a variable to the empty string its value should be \"\".\n");
     fprintf(f,"# Switch variables are set to either \"yes\" or \"no\", and default to \"no\".\n");
     fprintf(f,"# Except for feature-list items, which are additive, values set in the\n");
     fprintf(f,"# .pinerc file replace those in pine.conf, and those in pine.conf.fixed\n");
     fprintf(f,"# over-ride all others.  Features can be over-ridden in .pinerc or\n");
     fprintf(f,"# pine.conf.fixed by pre-pending the feature name with \"no-\".\n#\n");
     fprintf(f,"# (These comments are automatically inserted.)\n");

     for(var = variables; var->name != NULL; var++){
         dprint(5,(debugfile,"write_pinerc: %s = %s\n", var->name,
                   var->user_val.p ? var->user_val.p : "<not set>"));
         if(!var->is_global || !var->is_used || var->is_obsolete)
           continue;

         if(var->descrip && *var->descrip){
           if(fprintf(f, "\n# %s\n", var->descrip) == EOF)
             goto io_err;
	 }

	 if(var->is_list){
	     if(var->global_val.l == NULL){
		 if(fprintf(f, "%s=\n", var->name) == EOF)
		   goto io_err;
	     }else{
		 int i;

		 for(i=0; var->global_val.l[i]; i++)
		   if(fprintf(f, "%s%s%s%s\n", (i) ? "\t" : var->name,
			      (i) ? "" : "=", var->global_val.l[i],
			      var->global_val.l[i+1] ? ",":"") == EOF)
		     goto io_err;
	     }
	 }else{
	     if(var->global_val.p == NULL){
		 if(fprintf(f, "%s=\n", var->name) == EOF)
		   goto io_err;
	     }else if(strlen(var->global_val.p) == 0){
		 if(fprintf(f, "%s=\"\"\n", var->name) == EOF)
               goto io_err;
	     }else{
		 if(fprintf(f,"%s=%s\n",var->name,var->global_val.p) == EOF)
		   goto io_err;
	     }
	 }
     }
     exit(0);


   io_err:
     fprintf(stderr, "Error writing config to stdout: %s\n",
             error_description(errno));
     exit(-1);
}


/*------------------------------------------------------------
    Dump out a pinerc to filename with fresh
    comments.  Preserves variables currently set in pinerc, if any.
  ----*/
void
dump_new_pinerc(filename)
char *filename;
{
    FILE            *f;
    struct variable *var;
    char             buf[MAXPATH];

#if defined(DOS) || defined(OS2)
    if(!ps_global->pinerc){
	char *p;
	int   l;

	if(p = getenv("PINERC")){
	    ps_global->pinerc = cpystr(p);
	}else{
	    char buf2[MAXPATH];
	    build_path(buf2, ps_global->home_dir, DF_PINEDIR);
	    build_path(buf, buf2, SYSTEM_PINERC);
	}
    }
#else	/* !DOS */
    if(!ps_global->pinerc)
	build_path(buf, ps_global->home_dir, ".pinerc");
#endif	/* !DOS */

    read_pinerc(buf, variables, ParseLocal);

    f = NULL;;
    if(filename[0] == '\0'){
	fprintf(stderr, "Missing argument to \"-pinerc\".\n");
    }else if(!strcmp(filename, "-")){
	f = stdout;
    }else{
	f = fopen(filename, "w");
    }

    if(f == NULL) 
	goto io_err;

    if(fprintf(f, "%s%s%s", cf_text_comment1, pine_version,
	       cf_text_comment2) == EOF)
	goto io_err;
    if(fprintf(f, "%s", cf_text_comment3) == EOF)
	goto io_err;

    for(var = variables; var->name != NULL; var++){
	dprint(5,(debugfile,"write_pinerc: %s = %s\n", var->name,
                   var->user_val.p ? var->user_val.p : "<not set>"));
        if(!var->is_user || !var->is_used || var->is_obsolete)
	    continue;

	/* Add extra comments about categories of variables */
        if(var == &variables[V_PERSONAL_NAME]){
	    if(fprintf(f, "\n%s\n", cf_before_personal_name) == EOF)
		goto io_err;
        }else if(var == &variables[V_INCOMING_FOLDERS]){
	    if(fprintf(f, "\n%s\n", cf_before_incoming_folders) == EOF)
		goto io_err;
        }else if(var == &variables[V_FEATURE_LIST]){
	    if(fprintf(f, "\n%s\n", cf_before_feature_list) == EOF)
		goto io_err;
        }else if(var == &variables[V_PRINTER]){
	    if(fprintf(f, "\n%s\n", cf_before_printer) == EOF)
		goto io_err;
        }

	/*
	 * set description to NULL to eliminate preceding
	 * blank and comment line.
	 */
         if(var->descrip && *var->descrip){
           if(fprintf(f, "\n# %s\n", var->descrip) == EOF)
             goto io_err;
	 }

	if(var->is_list){
	    if(var->user_val.l == NULL){
		if(fprintf(f, "%s=\n", var->name) == EOF)
		    goto io_err;
	    }else{
		int i;

		for(i=0; var->user_val.l[i]; i++)
		    if(fprintf(f, "%s%s%s%s\n", (i) ? "\t" : var->name,
			      (i) ? "" : "=", var->user_val.l[i],
			      var->user_val.l[i+1] ? ",":"") == EOF)
		    goto io_err;
	    }
	}else{
	    if(var->user_val.p == NULL){
		if(fprintf(f, "%s=\n", var->name) == EOF)
		    goto io_err;
	    }else if(strlen(var->user_val.p) == 0){
		if(fprintf(f, "%s=\"\"\n", var->name) == EOF)
		    goto io_err;
	    }else{
		if(fprintf(f,"%s=%s\n",var->name,var->user_val.p) == EOF)
		    goto io_err;
	    }
	}
    }
    exit(0);


io_err:
    fprintf(stderr, "Error writing config to %s: %s\n",
             filename, error_description(errno));
    exit(-1);
}


/*----------------------------------------------------------------------
  Dump the whole config enchilada using the given function
   
  Args: ps -- pine struct containing vars to dump
	pc -- function to use to write the config data

 Result: command line, global, user var's written with given function

 ----*/ 
void
dump_config(ps, pc)
    struct pine *ps;
    gf_io_t pc;
{
    int	       i;
    char       quotes[3], tmp[MAILTMPLEN];
    register struct variable *vars;
    NAMEVAL_S *feat;

    quotes[0] = '"'; quotes[1] = '"'; quotes[2] = '\0';

    for(i=0; i < 5; i++){
	sprintf(tmp, "======= %s_val options set",
		(i == 0) ? "Current" :
		 (i == 1) ? "Command_line" :
		  (i == 2) ? "User" :
		   (i == 3) ? "Global"
			    : "Fixed");
	gf_puts(tmp, pc);
	if(i > 1){
	    sprintf(tmp, " (%s)",
		    (i == 2) ? ((ps->pinerc) ? ps->pinerc
					     : ".pinerc") :
		    (i == 3) ? ((ps->pine_conf) ? ps->pine_conf
						: SYSTEM_PINERC) :
#if defined(DOS) || defined(OS2)
		    "NO FIXED"
#else
		    ((can_access(SYSTEM_PINERC_FIXED, ACCESS_EXISTS) == 0)
			     ? SYSTEM_PINERC_FIXED : "NO pine.conf.fixed")
#endif
		    );
	    gf_puts(tmp, pc);
	}

	gf_puts(" =======\n", pc);
	for(vars = ps->vars; vars->name; vars++){
	    if(vars->is_list){
		char **t;
		t = (i == 0) ? vars->current_val.l :
		     (i == 1) ? vars->cmdline_val.l :
		      (i == 2) ? vars->user_val.l :
			(i == 3) ? vars->global_val.l
				 : vars->fixed_val.l;
		if(t && *t){
		    sprintf(tmp, " %20.20s : %s\n", vars->name,
			    **t ? *t : quotes);
		    gf_puts(tmp, pc);
		    while(++t && *t){
			sprintf(tmp," %20.20s : %s\n","",**t ? *t : quotes);
			gf_puts(tmp, pc);
		    }
		}
	    }
	    else{
		char *t;
		t = (i == 0) ? vars->current_val.p :
		     (i == 1) ? vars->cmdline_val.p :
		      (i == 2) ? vars->user_val.p :
		       (i == 3) ? vars->global_val.p
				: vars->fixed_val.p;
		if(t){
		    sprintf(tmp, " %20.20s : %s\n", vars->name,
			    *t ? t : quotes);
		    gf_puts(tmp, pc);
		}
	    }
	}
    }

    gf_puts("========== Feature settings ==========\n", pc);
    for(i = 0; feat = feature_list(i) ; i++)
      if(feat->value != F_OLD_GROWTH){
	  sprintf(tmp, "  %s%s\n", F_ON(feat->value,ps) ? "   " : "no-",
		  feat->name);
	  gf_puts(tmp, pc);
      }
}


/*----------------------------------------------------------------------
  Dump interesting variables from the given pine struct
   
  Args: ps -- pine struct to dump 
	pc -- function to use to write the config data

 Result: various important pine struct data written

 ----*/ 
void
dump_pine_struct(ps, pc)
    struct pine *ps;
    gf_io_t pc;
{
    char *p;
    extern char term_name[];

    gf_puts("========== struct pine * ==========\n", pc);
    if(!ps){
	gf_puts("!No struct!\n", pc);
	return;
    }

    gf_puts("ui:\tlogin = ", pc);
    gf_puts((ps->ui.login) ? ps->ui.login : "NULL", pc);
    gf_puts(", full = ", pc);
    gf_puts((ps->ui.fullname) ? ps->ui.fullname : "NULL", pc);
    gf_puts("\n\thome = ", pc);
    gf_puts((ps->ui.homedir) ? ps->ui.homedir : "NULL", pc);

    gf_puts("\nhome_dir=\t", pc);
    gf_puts(ps->home_dir ? ps->home_dir : "NULL", pc);
    gf_puts("\nhostname=\t", pc);
    gf_puts(ps->hostname ? ps->hostname : "NULL", pc);
    gf_puts("\nlocaldom=\t", pc);
    gf_puts(ps->localdomain ? ps->localdomain : "NULL", pc);
    gf_puts("\nuserdom=\t", pc);
    gf_puts(ps->userdomain ? ps->userdomain : "NULL", pc);
    gf_puts("\nmaildom=\t", pc);
    gf_puts(ps->maildomain ? ps->maildomain : "NULL", pc);

    if(ps->mail_stream){
	gf_puts("\ncur_cntxt=\t", pc);
	gf_puts((ps->context_current && ps->context_current->context)
		? ps->context_current->context : "None", pc);
	gf_puts("\ncur_fldr=\t", pc);
	gf_puts(ps->cur_folder, pc);
	gf_puts("\nactual mbox=\t", pc);
	gf_puts(ps->mail_stream->mailbox ? ps->mail_stream->mailbox
					 : "no mailbox!", pc);
	if(ps->msgmap){
	    gf_puts("\nmsgmap: tot=", pc);
	    gf_puts(long2string(mn_get_total(ps->msgmap)), pc);
	    gf_puts(", cur=", pc);
	    gf_puts(long2string(mn_get_cur(ps->msgmap)), pc);
	    gf_puts(", del=", pc);
	    gf_puts(long2string(count_flagged(ps->mail_stream, "DELETED")),pc);
	    gf_puts(", hid=", pc);
	    gf_puts(long2string(any_lflagged(ps->msgmap, MN_HIDE)), pc);
	    gf_puts(", exld=", pc);
	    gf_puts(long2string(any_lflagged(ps->msgmap, MN_EXLD)), pc);
	    gf_puts(", slct=", pc);
	    gf_puts(long2string(any_lflagged(ps->msgmap, MN_SLCT)), pc);
	    gf_puts(", sort=", pc);
	    if(mn_get_revsort(ps->msgmap))
	      gf_puts("rev-", pc);

	    gf_puts(sort_name(mn_get_sort(ps->msgmap)), pc);
	}
	else
	  gf_puts("\nNo msgmap", pc);
    }
    else
      gf_puts("\nNo mail_stream", pc);

    if(ps->inbox_stream && (ps->mail_stream != ps->inbox_stream)){
	gf_puts("\nactual inbox=\t", pc);
	gf_puts(ps->inbox_stream->mailbox ? ps->inbox_stream->mailbox
					  : "no mailbox!", pc);
	if(ps->inbox_msgmap){
	    gf_puts("\ninbox map: tot=", pc);
	    gf_puts(long2string(mn_get_total(ps->inbox_msgmap)), pc);
	    gf_puts(", cur=", pc);
	    gf_puts(long2string(mn_get_cur(ps->inbox_msgmap)), pc);
	    gf_puts(", del=", pc);
	    gf_puts(long2string(count_flagged(ps->inbox_stream,"DELETED")),pc);
	    gf_puts(", hid=", pc);
	    gf_puts(long2string(any_lflagged(ps->inbox_msgmap, MN_HIDE)), pc);
	    gf_puts(", exld=", pc);
	    gf_puts(long2string(any_lflagged(ps->inbox_msgmap, MN_EXLD)), pc);
	    gf_puts(", slct=", pc);
	    gf_puts(long2string(any_lflagged(ps->inbox_msgmap, MN_SLCT)), pc);
	    gf_puts(", sort=", pc);
	    if(mn_get_revsort(ps->inbox_msgmap))
	      gf_puts("rev-", pc);

	    gf_puts(sort_name(mn_get_sort(ps->inbox_msgmap)), pc);
	}
	else
	  gf_puts("\nNo inbox_map", pc);
    }
    else
      gf_puts(ps->inbox_stream ? "\ninbox is mail_stream"
			       : "\nno inbox stream", pc);
    gf_puts("\nterm ", pc);
#if !defined(DOS) && !defined(OS2)
    gf_puts("type=", pc);
    gf_puts(term_name, pc);
    gf_puts(", ttyname=", pc);
    gf_puts((p = (char *)ttyname(0)) ? p : "NONE", pc);
#endif
    gf_puts(", size=", pc);
    gf_puts(int2string(ps->ttyo->screen_rows), pc);
    gf_puts("x", pc);
    gf_puts(int2string(ps->ttyo->screen_cols), pc);
    gf_puts(", speed=", pc);
    gf_puts((ps->low_speed) ? "slow" : "normal", pc);
    gf_puts("\n", pc);
}


/*----------------------------------------------------------------------
      Set a user variable and save the .pinerc
   
  Args:  var -- The index of the variable to set from pine.h (V_....)
         value -- The string to set the value to

 Result: -1 is returned on failure and 0 is returned on success

 The vars data structure is updated and the pinerc saved.
 ----*/ 
set_variable(var, value, commit)
     int   var, commit;
     char *value;
{
    struct variable *v;

    v = &ps_global->vars[var];

    if(!v->is_user) 
      panic1("Trying to set non-user variable %s", v->name);

    if(v->user_val.p)
      fs_give((void **) &v->user_val.p);

    if(v->current_val.p)
      fs_give((void **) &v->current_val.p);

    v->user_val.p    = value ? cpystr(value) : NULL;
    v->current_val.p = value ? cpystr(value) : NULL;

    ps_global->outstanding_pinerc_changes = 1;

    return(commit ? write_pinerc(ps_global) : 1);
}


/*----------------------------------------------------------------------
      Set a user variable list and save the .pinerc
   
  Args:  var -- The index of the variable to set from pine.h (V_....)
         lvalue -- The list to set the value to

 Result: -1 is returned on failure and 0 is returned on success

 The vars data structure is updated and the pinerc saved.
 ----*/ 
set_variable_list(var, lvalue)
    int    var;
    char **lvalue;
{
    int              i;
    struct variable *v = &ps_global->vars[var];

    if(!v->is_user || !v->is_list)
      panic1("BOTCH: Trying to set non-user or non-list variable %s", v->name);

    if(v->user_val.l){
	for(i = 0; v->user_val.l[i] ; i++)
	  fs_give((void **) &v->user_val.l[i]);

	fs_give((void **) &v->user_val.l);
    }

    if(v->current_val.l){
	for(i = 0; v->current_val.l[i] ; i++)
	  fs_give((void **) &v->current_val.l[i]);

	fs_give((void **) &v->current_val.l);
    }

/* BUG: HAVING MULTIPLE COPIES OF CONFIG DATA IS BOGUS */
    if(lvalue){
	for(i = 0; lvalue[i] ; i++)	/* count elements */
	  ;

	v->user_val.l    = (char **) fs_get((i+1) * sizeof(char *));
	v->current_val.l = (char **) fs_get((i+1) * sizeof(char *));

	for(i = 0; lvalue[i] ; i++){
	    v->user_val.l[i]    = cpystr(lvalue[i]);
	    v->current_val.l[i] = cpystr(lvalue[i]);
	}

	v->user_val.l[i]    = NULL;
	v->current_val.l[i] = NULL;
    }


    return(write_pinerc(ps_global));
}
           

/*----------------------------------------------------------------------
    Make sure the pine folders directory exists, with proper folders

   Args: ps -- pine structure to get mail directory and contexts from

  Result: returns 0 if it exists or it is created and all is well
                  1 if it is missing and can't be created.
  ----*/
init_mail_dir(ps)
    struct pine *ps;
{
    /*
     * We don't really care if mail_dir exists if it isn't 
     * part of the first folder collection specified.  If this
     * is the case, it must have been created external to us, so
     * just move one...
     */
    if(ps->VAR_FOLDER_SPEC && ps->VAR_FOLDER_SPEC[0]){
	char *p  = context_string(ps->VAR_FOLDER_SPEC[0]);
	int   rv = strncmp(p, ps->VAR_MAIL_DIRECTORY,
			   strlen(ps->VAR_MAIL_DIRECTORY));
	fs_give((void **)&p);
	if(rv)
	  return(0);
    }

    switch(is_writable_dir(ps->folders_dir)){
      case 0:
        /* --- all is well --- */
	return(0);

      case 1:
	sprintf(tmp_20k_buf, init_md_exists, ps->folders_dir);
	display_init_err(tmp_20k_buf, 1);
	return(-1);

      case 2:
	sprintf(tmp_20k_buf, init_md_file, ps->folders_dir);
	display_init_err(tmp_20k_buf, 1);
	return(-1);

      case 3:
	sprintf(tmp_20k_buf, init_md_create, ps->folders_dir);
	display_init_err(tmp_20k_buf, 0);
#ifndef	_WINDOWS
    	sleep(4);
#endif
        if(create_mail_dir(ps->folders_dir) < 0){
            sprintf(tmp_20k_buf, "Error creating subdirectory \"%s\" : %s",
		    ps->folders_dir, error_description(errno));
	    display_init_err(tmp_20k_buf, 1);
            return(-1);
        }
    }

    return(0);
}


/*----------------------------------------------------------------------
  Make sure the default save folders exist in the default
  save context.
  ----*/
void
display_init_err(s, err)
    char *s;
    int   err;
{
#ifdef	_WINDOWS
    mswin_messagebox(s, err);
#else
    int n = 0;

    if(err)
      fputc(BELL, stdout);

    for(; *s; s++)
      if(++n > 60 && isspace((unsigned char)*s)){
	  n = 0;
	  fputc('\n', stdout);
	  while(*(s+1) && isspace((unsigned char)*(s+1)))
	    s++;
      }
      else
	fputc(*s, stdout);

    fputc('\n', stdout);
#endif
}


/*----------------------------------------------------------------------
  Make sure the default save folders exist in the default
  save context.
  ----*/
void
init_save_defaults()
{
    CONTEXT_S  *save_cntxt;

    if(!ps_global->VAR_DEFAULT_FCC || !*ps_global->VAR_DEFAULT_FCC)
      return;

    if(!(save_cntxt = default_save_context(ps_global->context_list)))
      save_cntxt = ps_global->context_list;

    if(context_isambig(ps_global->VAR_DEFAULT_FCC)){
	find_folders_in_context(NULL, save_cntxt, ps_global->VAR_DEFAULT_FCC);
	if(folder_index(ps_global->VAR_DEFAULT_FCC, save_cntxt->folders) < 0)
	  context_create(save_cntxt->context, NULL,
			 ps_global->VAR_DEFAULT_FCC);
    }

    find_folders_in_context(NULL, save_cntxt,
			    ps_global->VAR_DEFAULT_SAVE_FOLDER);
    if(folder_index(ps_global->VAR_DEFAULT_SAVE_FOLDER,
		    save_cntxt->folders) < 0)
      context_create(save_cntxt->context, NULL,
		     ps_global->VAR_DEFAULT_SAVE_FOLDER);

    free_folders_in_context(save_cntxt);
}



/*----------------------------------------------------------------------
   Routines for pruning old Fcc, usually "sent-mail" folders.     
  ----*/
struct sm_folder {
    char *name;
    int   month_num;
};


/*
 * Pruning prototypes
 */
void	 delete_old_mail PROTO((struct sm_folder *, CONTEXT_S *, char *));
struct	 sm_folder *get_mail_list PROTO((CONTEXT_S *, char *));
int	 prune_folders PROTO((CONTEXT_S *, char *, int, char *));



/*----------------------------------------------------------------------
      Put sent-mail files in date order 

   Args: a, b  -- The names of two files.  Expects names to be sent-mail-mmm-yy
                  Other names will sort in order and come before those
                  in above format.
 ----*/
int   
compare_sm_files(aa, bb)
    const QSType *aa, *bb;
{
    struct sm_folder *a = (struct sm_folder *)aa,
                     *b = (struct sm_folder *)bb;

    if(a->month_num == -1 && b->month_num == -1)
      return(strucmp(a->name, b->name));
    if(a->month_num == -1)      return(-1);
    if(b->month_num == -1)      return(1);

    return(a->month_num - b->month_num);
}



/*----------------------------------------------------------------------
      Create an ordered list of sent-mail folders and their month numbers

   Args: dir -- The directory to find the list of files in

 Result: Pointer to list of files is returned. 

This list includes all files that start with "sent-mail", but not "sent-mail" 
itself.
  ----*/
struct sm_folder *
get_mail_list(list_cntxt, folder_base)
    CONTEXT_S *list_cntxt;
    char      *folder_base;
{
#define MAX_FILES  (150)
    register struct sm_folder *sm  = NULL;
    struct sm_folder          *sml = NULL;
    char                      *filename;
    int                        i, folder_base_len;
    char		       searchname[MAXPATH+1];

    sml = sm = (struct sm_folder *)fs_get(sizeof(struct sm_folder)*MAX_FILES);
    memset((void *)sml, 0, sizeof(struct sm_folder) * MAX_FILES);
    if((folder_base_len = strlen(folder_base)) == 0 || !list_cntxt){
        sml->name = cpystr("");
        return(sml);
    }

#ifdef	DOS
    if(*list_cntxt->context != '{'){	/* NOT an IMAP collection! */
	sprintf(searchname, "%4.4s*", folder_base);
	folder_base_len = strlen(searchname) - 1;
    }
    else
#endif
    sprintf(searchname, "%s*", folder_base);

    find_folders_in_context(NULL, list_cntxt, searchname);
    for(i = 0; i < folder_total(list_cntxt->folders); i++){
	filename = folder_entry(i, list_cntxt->folders)->name;
#ifdef	DOS
        if(struncmp(filename, folder_base, folder_base_len) == 0
           && strucmp(filename, folder_base)){

	if(*list_cntxt->context != '{'){
	    int j;
	    for(j = 0; j < 4; j++)
	      if(!isdigit((unsigned char)filename[folder_base_len + j]))
		break;

	   if(j < 4)		/* not proper date format! */
	     continue;		/* keep trying */
	}
#else
#ifdef OS2
        if(strnicmp(filename, folder_base, folder_base_len) == 0
           && stricmp(filename, folder_base)){
#else
        if(strncmp(filename, folder_base, folder_base_len) == 0
           && strcmp(filename, folder_base)){
#endif
#endif
	    sm->name = cpystr(filename);
#ifdef	DOS
	    if(*list_cntxt->context != '{'){ /* NOT an IMAP collection! */
		sm->month_num  = (sm->name[folder_base_len] - '0') * 10;
		sm->month_num += sm->name[folder_base_len + 1] - '0';
	    }
	    else
#endif
            sm->month_num = month_num(sm->name + (size_t)folder_base_len + 1);
            sm++;
            if(sm >= &sml[MAX_FILES])
               break; /* Too many files, ignore the rest ; shouldn't occur */
        }
    }

    sm->name = cpystr("");

    /* anything to sort?? */
    if(sml->name && *(sml->name) && (sml+1)->name && *((sml+1)->name)){
	qsort(sml,
	      sm - sml,
	      sizeof(struct sm_folder),
	      compare_sm_files);
    }

    return(sml);
}



/*----------------------------------------------------------------------
      Rename the current sent-mail folder to sent-mail for last month

   open up sent-mail and get date of very first message
   if date is last month rename and...
       if files from 3 months ago exist ask if they should be deleted and...
           if files from previous months and yes ask about them, too.   
  ----------------------------------------------------------------------*/
int
expire_sent_mail()
{
    int		 cur_month, ok = 1;
    time_t	 now;
    char	 tmp[20], **p;
    struct tm	*tm_now;
    CONTEXT_S	*prune_cntxt;

    dprint(5, (debugfile, "==== expire_mail called ====\n"));

    now = time((time_t *)0);
    tm_now = localtime(&now);

    /*
     * If the last time we did this is blank (as if pine's run for
     * first time), don't go thru list asking, but just note it for 
     * the next time...
     */
    if(ps_global->VAR_LAST_TIME_PRUNE_QUESTION == NULL){
	ps_global->last_expire_year = tm_now->tm_year;
	ps_global->last_expire_month = tm_now->tm_mon;
	sprintf(tmp, "%d.%d", ps_global->last_expire_year,
		ps_global->last_expire_month + 1);
	set_variable(V_LAST_TIME_PRUNE_QUESTION, tmp, 1);
	return(0);
    }

    if(ps_global->last_expire_year != -1 &&
      (tm_now->tm_year <  ps_global->last_expire_year ||
       (tm_now->tm_year == ps_global->last_expire_year &&
        tm_now->tm_mon <= ps_global->last_expire_month)))
      return(0); 
    
    cur_month = (1900 + tm_now->tm_year) * 12 + tm_now->tm_mon;
    dprint(5, (debugfile, "Current month %d\n", cur_month));

    /*
     * locate the default save context...
     */
    if(!(prune_cntxt = default_save_context(ps_global->context_list)))
      prune_cntxt = ps_global->context_list;

    /*
     * Since fcc's and read-mail can be an IMAP mailbox, be sure to only
     * try expiring a list if it's an ambiguous name associated with some
     * collection...
     *
     * If sentmail set outside a context, then pruning is up to the
     * user...
     */
    if(prune_cntxt){
	if(ps_global->VAR_DEFAULT_FCC && *ps_global->VAR_DEFAULT_FCC
	   && context_isambig(ps_global->VAR_DEFAULT_FCC))
	  ok = prune_folders(prune_cntxt, ps_global->VAR_DEFAULT_FCC,
			     cur_month, "SENT");

	if(ok && ps_global->VAR_READ_MESSAGE_FOLDER 
	   && *ps_global->VAR_READ_MESSAGE_FOLDER
	   && context_isambig(ps_global->VAR_READ_MESSAGE_FOLDER))
	  ok = prune_folders(prune_cntxt, ps_global->VAR_READ_MESSAGE_FOLDER,
			     cur_month, "READ");
    }

    /*
     * Within the default prune context,
     * prune back the folders with the given name
     */
    if(ok && prune_cntxt && (p = ps_global->VAR_PRUNED_FOLDERS))
      for(; ok && *p; p++)
	if(**p && context_isambig(*p))
	  ok = prune_folders(prune_cntxt, *p, cur_month, "");

    /*
     * Mark that we're done for this month...
     */
    if(ok){
	ps_global->last_expire_year = tm_now->tm_year;
	ps_global->last_expire_month = tm_now->tm_mon;
	sprintf(tmp, "%d.%d", ps_global->last_expire_year,
		ps_global->last_expire_month + 1);
	set_variable(V_LAST_TIME_PRUNE_QUESTION, tmp, 1);
    }

    return(1);
}



/*----------------------------------------------------------------------
     Offer to delete old sent-mail folders

  Args: sml -- The list of sent-mail folders
 
  ----*/
int
prune_folders(prune_cntxt, folder_base, cur_month, type)
    CONTEXT_S *prune_cntxt;
    char      *folder_base, *type;
    int        cur_month;
{
    char         path[MAXPATH+1], path2[MAXPATH+1],  prompt[128], tmp[20];
    int          month_to_use, i;
    MAILSTREAM  *prune_stream;
    struct sm_folder *mail_list, *sm;

    mail_list = get_mail_list(prune_cntxt, folder_base);

#ifdef	DEBUG
    for(sm = mail_list; sm != NULL && sm->name[0] != '\0'; sm++)
      dprint(5, (debugfile,"Old sent-mail: %5d  %s\n",sm->month_num,sm->name));
#endif

    for(sm = mail_list; sm != NULL && sm->name[0] != '\0'; sm++)
      if(sm->month_num == cur_month - 1)
        break;  /* matched a month */
 
    month_to_use = (sm == NULL || sm->name[0] == '\0') ? cur_month - 1 : 0;

    dprint(5, (debugfile, "Month_to_use : %d\n", month_to_use));

    if(month_to_use == 0)
      goto delete_old;

    strcpy(path, folder_base);
/* BUG: how to check that a folder is zero_length via c-client?? */
    strcpy(path2, folder_base);
    strcpy(tmp, month_abbrev((month_to_use % 12)+1));
    lcase(tmp);
#ifdef	DOS
    if(*prune_cntxt->context != '{'){
      sprintf(path2 + (size_t)(((i = strlen(path2)) > 4) ? 4 : i),
	      "%2.2d%2.2d", (month_to_use % 12) + 1,
	      ((month_to_use / 12) - 1900) % 100);
    }
    else
#endif
    sprintf(path2 + strlen(path2), "-%s-%2d", tmp, month_to_use/12);

    Writechar(BELL, 0);
    sprintf(prompt, "Move current \"%s\" to \"%s\"", path, path2);
    switch(folder_exists(prune_cntxt->context, folder_base)){
      case -1 :			/* error! */
        dprint(5, (debugfile, "prune_folders: Error testing existance\n"));
        return(0);

      case 0 :			/* doesn't exist */
        dprint(5, (debugfile, "prune_folders: nothing to prune <%s %s>\n",
		   prune_cntxt->context, folder_base));
        goto delete_old;

      default :
	if(want_to(prompt, 'n', 0, h_wt_expire, 1, 1) == 'n'){
	    dprint(5, (debugfile, "User declines renaming %s\n",
		       ps_global->VAR_DEFAULT_FCC));
	    goto delete_old;
	}
	/* else fall thru processing matching folders */

	break;
    }

    /*--- User says OK to rename ---*/
    dprint(5, (debugfile, "rename \"%s\" to \"%s\"\n", path, path2));
    prune_stream = context_same_stream(prune_cntxt->context, path2,
				       ps_global->mail_stream);

    if(!prune_stream && ps_global->mail_stream != ps_global->inbox_stream)
      prune_stream = context_same_stream(prune_cntxt->context, path2,
					 ps_global->inbox_stream);

    if(!context_rename(prune_cntxt->context, prune_stream, path, path2)){
        q_status_message2(SM_ORDER | SM_DING, 3, 4,
			  "Error renaming \"%s\": %s",
                          pretty_fn(folder_base),
			  error_description(errno));
        dprint(1, (debugfile, "Error renaming %s to %s: %s\n",
                   path, path2, error_description(errno)));
        display_message('x');
        goto delete_old;
    }

    context_create(prune_cntxt->context, prune_stream ? prune_stream : NULL,
		   folder_base);

  delete_old:
    delete_old_mail(mail_list, prune_cntxt, type);
    if(sm = mail_list){
	while(sm->name){
	    fs_give((void **)&(sm->name));
	    sm++;
	}

        fs_give((void **)&mail_list);
    }

    return(1);
}


/*----------------------------------------------------------------------
     Offer to delete old sent-mail folders

  Args: sml       -- The list of sent-mail folders
        fcc_cntxt -- context to delete list of folders in
        type      -- label indicating type of folders being deleted
 
  ----*/
void
delete_old_mail(sml, fcc_cntxt, type)
    struct sm_folder *sml;
    CONTEXT_S        *fcc_cntxt;
    char             *type;
{
    char  prompt[150];
    int   rc;
    struct sm_folder *sm;
    MAILSTREAM       *del_stream;

    for(sm = sml; sm != NULL && sm->name[0] != '\0'; sm++){
        sprintf(prompt,
	       "To save disk space, delete old %.4s mail folder \"%.30s\" ",
	       type, sm->name);
        if(want_to(prompt, 'n', 0, h_wt_delete_old, 1, 1) == 'y'){
	    del_stream = context_same_stream(fcc_cntxt->context, sm->name,
					     ps_global->mail_stream);

	    if(!del_stream 
	       && ps_global->mail_stream != ps_global->inbox_stream)
	      del_stream = context_same_stream(fcc_cntxt->context, sm->name,
					       ps_global->inbox_stream);

	    if(!context_delete(fcc_cntxt->context, del_stream, sm->name)){
		q_status_message1(SM_ORDER,
				  3, 3, "Error deleting \"%s\".", sm->name);
		dprint(1, (debugfile, "Error context_deleting %s in \n",
			   sm->name, fcc_cntxt->context));
            }
	}else{
		/* break; /* skip out of the whole thing when he says no */
		/* Decided to keep asking anyway */
        }
    }
}
