#ident	"@(#)setup_txt.h	1.3"
/*
//  This file contains the text strings used by getStr().  
*/


#ifndef SETUP_TXT_H
#define SETUP_TXT_H


#ifndef F_SEP
#define F_SEP	"\001"
#endif	//  F_SEP


//  Help file related items.
#define HELP_FILE		"/usr/X/lib/locale/C/help/setup/setup.hlp"

#define TXT_appHelpTitle	"gensetup:1"  F_SEP "No-Name Setup Application"
#define TXT_appHelpSection	"gensetup:2"  F_SEP "10"

#define TXT_appNoName		"gensetup:3"  F_SEP "No-Name Setup Application"
#define TXT_iconNoName		"gensetup:4"  F_SEP "No-Name"
#define TXT_defaultIconFile	"gensetup:5"  F_SEP "exec48.icon"

#define TXT_category		"gensetup:6"  F_SEP "Category: "
#define MNEM_category		"gensetup:7"  F_SEP "C"

#define TXT_browserAdd		"gensetup:8"  F_SEP "Add..."
#define MNEM_browserAdd		"gensetup:9"  F_SEP "A"
#define TXT_browserDelete	"gensetup:10" F_SEP "Delete"
#define MNEM_browserDelete	"gensetup:11" F_SEP "D"

#define TXT_variables		"gensetup:12" F_SEP "Variables"
#define TXT_rightTitle		"gensetup:13" F_SEP "Description"


//  Toggle labels
#define TXT_toggleOff		"gensetup:14" F_SEP "No"
#define TXT_toggleOn		"gensetup:15" F_SEP "Yes"


//  Action Area Button Labels
#define TXT_OkButton		"gensetup:16" F_SEP "OK"
#define TXT_ResetButton		"gensetup:17" F_SEP "Reset"
#define TXT_CancelButton	"gensetup:18" F_SEP "Cancel"
#define TXT_HelpButton		"gensetup:19" F_SEP "Help"


#define TXT_fontLoadErr		"gensetup:20" F_SEP "Error loading Desktop fonts."
#define TXT_fontListErr		"gensetup:21" F_SEP "Error creating fonts."

//  Error messages which appear in the error dialog popup.
#define TXT_setupWebNoAccess	"gensetup:22" F_SEP \
	"The setup library initialization has failed.\n"\
	"Please see your system administrator."
#define TXT_noModes		"gensetup:23" F_SEP \
	"This application has no variables\n"\
	"to display.  Please see your system\n"\
	"administrator."
#define TXT_noPerms		"gensetup:24" F_SEP \
	"You do not have permission to change\n"\
	"parameters in this setup application."

//  Error messages which appear in the error dialog popup for the password field.
#define TXT_invalPasswd		"gensetup:25" F_SEP \
	"Your second password does not match the first.\n"\
	"Please re-enter your password."

//  Action Area Button Mnemonics (unfortunately, mnemonic support was added
//  later, so these don't appear above with the action area button labels).
#define MNEM_OkButton		"gensetup:26" F_SEP "O"
#define MNEM_ResetButton	"gensetup:27" F_SEP "R"
#define MNEM_CancelButton	"gensetup:28" F_SEP "C"
#define MNEM_HelpButton		"gensetup:29" F_SEP "H"

#define TXT_invalIntChars	"gensetup:30" F_SEP \
	"This integer field contains invalid characters.\n"\
	"Please re-enter this field."
#define TXT_invalIntRange	"gensetup:31" F_SEP \
	"This integer is either too large or too small.\n"\
	"Please re-enter this field."



//  Add-ons ...
#define TXT_noLabel		"gensetup:32" F_SEP "No Label"
#define TXT_noDescription	"gensetup:33" F_SEP \
	"There is no description available for this variable."

#define TXT_fileTitle		"gensetup:34" F_SEP "File"
#define MNEM_fileTitle		"gensetup:35" F_SEP "F"
#define TXT_fileExitTitle	"gensetup:36" F_SEP "Exit"
#define MNEM_fileExitTitle	"gensetup:37" F_SEP "x"
#define TXT_helpTitle		"gensetup:38" F_SEP "Help"
#define MNEM_helpTitle		"gensetup:39" F_SEP "H"
#define TXT_helpTOC		"gensetup:40" F_SEP "Table of Contents..."
#define MNEM_helpTOC		"gensetup:41" F_SEP "T"
#define TXT_helpHelpDesk	"gensetup:42" F_SEP "Help Desk..."
#define MNEM_helpHelpDesk	"gensetup:43" F_SEP "H"

#define TXT_okFailed		"gensetup:44" F_SEP \
	"Some (or all) of the variable values\n"\
	"in this setup window could not be saved.\n"\
	"The reason could not be determined.\n"\
	"Please contact your system administrator."


#endif	//  SETUP_TXT_H
