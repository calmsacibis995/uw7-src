#ifndef NOIDENT
#ident	"@(#)dtadmin:print/use/error.h	1.13.2.8"
#endif

#ifndef ERROR_H
#define ERROR_H

#ifndef FS
#define FS	"\001"
#define FS_CHR	'\001'
#endif

#define HELP_FILE		"dtadmin/printuse.hlp"

#define TXT_intrnlErr		"PrtMgr:1" FS "Internal Error:\n\t%s\n"
#define TXT_errnoEq		"PrtMgr:2" FS "\t(errno=%d)\n"
#define TXT_winTitle		"PrtMgr:3" FS \
    "Printer:  Request Properties - %s"
#define TXT_noneSelected	"PrtMgr:4" FS \
    "No Print Request has been selected"
#define TXT_badCopies		"PrtMgr:5" FS \
    "\"Number of copies\" entry is invalid"
#define TXT_badGetFiles		"PrtMgr:6" FS "Cannot get temporary files.\n"\
    "The print scheduler is probably not running"
#define TXT_badFile		"PrtMgr:7" FS "Cannot find file '%s'"
#define TXT_noDir		"PrtMgr:8" FS "Cannot print directory '%s'"
#define TXT_noOpen		"PrtMgr:9" FS "Cannot read file '%s'"
#define TXT_badCopy		"PrtMgr:10" FS \
    "Cannot make a copy of file '%s'"
#define TXT_noPrinter		"PrtMgr:11" FS \
    "Printer unknown!  It may have been deleted.  Choose another printer."
#define TXT_stdinName		"PrtMgr:12" FS "Standard Input"
#define TXT_multiFiles		"PrtMgr:13" FS "%s, etc."

#define TXT_rqId		"PrtMgr:14" FS \
    "Your print request has been accepted.\n" \
    "(%d file(s))\n\n"	\
    "The request id is:\n%s"
#define TXT_prtDeleted		"PrtMgr:15" FS \
    "This printer seems to have been recently deleted!\n"
#define TXT_badCharSet		"PrtMgr:16" FS \
    "The character set selected is not valid for this printer"
#define TXT_badPitch		"PrtMgr:17" FS \
    "You can not change the character or line pitch for this printer"
#define TXT_badPageSize		"PrtMgr:18" FS \
    "You can not change the page dimensions on this printer"
#define TXT_bannerRqd		"PrtMgr:19" FS \
    "The banner page is required"
#define TXT_denied		"PrtMgr:20" FS \
    "You are not permitted to use this printer"
#define TXT_badForm		"PrtMgr:21" FS \
    "The Form can not be found"
#define TXT_formDenied		"PrtMgr:22" FS \
    "You are not permitted to use this form"
#define TXT_unmounted		"PrtMgr:23" FS \
    "The form or print wheel is not available on this printer"
#define TXT_reject		"PrtMgr:24" FS \
    "This printer is currently not accepting new print requests"
#define TXT_noFilter		"PrtMgr:25" FS \
    "This printer is unable to print files of this type, or the options you " \
    "specified are incompatible"
#define TXT_unsettable		"PrtMgr:26" FS \
    "You attempted to set page dimensions or character pitch on a " \
    "printer that is not capable of setting these attributes."
#define TXT_badJob		"PrtMgr:27" FS \
    "The spooler was unable to schedule your request for printing.  " \
    "Perhaps the spooler is not running."
#define TXT_noJob		"PrtMgr:28" FS \
    "No files were queued for printing"
#define TXT_noPerm		"PrtMgr:29" FS \
    "You are not allowed to delete the following print requests:\n\n"
#define TXT_jobDone		"PrtMgr:30" FS \
    "The following requests have already printed:\n\n"
#define TXT_reallyDelete	"PrtMgr:31" FS \
    "Do you really want to delete the following requests:\n\n"
#define TXT_badCancel		"PrtMgr:32" FS \
    "Print requests can not be deleted, " \
    "most likely because the lp scheduler is not running.\n\n"
#define TXT_badPgLen		"PrtMgr:33" FS \
    "Page length must be a positive number"
#define TXT_badPgWid		"PrtMgr:34" FS \
    "Page width must be a positive number"
#define TXT_badCpi		"PrtMgr:35" FS \
    "Character pitch must be a positive number"
#define TXT_badLpi		"PrtMgr:36" FS \
    "Line Pitch must be a positive number"
#define TXT_badCharOpt		"PrtMgr:37" FS \
    "%s option must be a single character"
#define TXT_badIntOpt		"PrtMgr:38" FS \
    "%s option must be an integer"
#define TXT_badUnsignedOpt	"PrtMgr:39" FS \
    "%s option must be an unsigned integer"
#define TXT_badFloatOpt		"PrtMgr:40" FS \
    "%s option must be a floating point number"
#define TXT_noChgPerm		"PrtMgr:41" FS \
    "You are not allowed to change the request"
#define TXT_jobChgDone		"PrtMgr:42" FS \
    "The request is either currently printing or has already finished.  " \
    "You cannot change it now."
#define TXT_filterTitle		"PrtMgr:43" FS "Printer:  Filter Options"
#define TXT_dfltErr		"PrtMgr:44" FS \
    "Could not write default options to a file."

#define TXT_copies		"PrtMgr:45" FS "Copies:"

#define TXT_prtreq		"PrtMgr:46" FS "Print Request"
#define TXT_delete		"PrtMgr:47" FS "Delete"
#define TXT_continue		"PrtMgr:48" FS "Continue"
#define TXT_cancel		"PrtMgr:49" FS "Cancel"
#define TXT_properties		"PrtMgr:50" FS "Properties..."
#define TXT_help		"PrtMgr:51" FS "Help"
#define TXT_apply		"PrtMgr:52" FS "Apply"
#define TXT_setDflts		"PrtMgr:53" FS "Set Defaults"
#define TXT_reset		"PrtMgr:54" FS "Reset"
#define TXT_more		"PrtMgr:55" FS "Other Options..."
#define TXT_yes			"PrtMgr:56" FS "Yes"
#define TXT_no			"PrtMgr:57" FS "No"
#define TXT_simple		"PrtMgr:58" FS "Text"
#define TXT_other		"PrtMgr:59" FS "Other"
#define TXT_mail		"PrtMgr:60" FS "Send mail when done?"
#define TXT_default		"PrtMgr:61" FS "Default"
#define TXT_charSet		"PrtMgr:62" FS "Character Set:"
#define TXT_banner		"PrtMgr:63" FS "Print banner page:"
#define TXT_inputType		"PrtMgr:64" FS "File to be printed is:"
#define TXT_otherInputType	"PrtMgr:65" FS "Specify Type:"
#define TXT_in			"PrtMgr:66" FS "in"
#define TXT_cm			"PrtMgr:67" FS "cm"
#define TXT_chars		"PrtMgr:68" FS "chars"
#define TXT_title		"PrtMgr:69" FS "Banner Page Title:"
#define TXT_pgLen		"PrtMgr:70" FS "Page Length:"
#define TXT_pgWid		"PrtMgr:71" FS "Page Width:"
#define TXT_cpi			"PrtMgr:72" FS "Character Pitch:"
#define TXT_lpi			"PrtMgr:73" FS "Line Pitch:"
#define TXT_options		"PrtMgr:74" FS "Options:"
#define TXT_miscOptions		"PrtMgr:75" FS "Misc. Options:"

#define MNEM_prtreq		"PrtMgr:76" FS "P"
#define MNEM_delete		"PrtMgr:77" FS "D"
#define MNEM_continue		"PrtMgr:78" FS "C"
#define MNEM_cancel		"PrtMgr:79" FS "C"
#define MNEM_properties		"PrtMgr:80" FS "P"
#define MNEM_help		"PrtMgr:81" FS "H"
#define MNEM_apply		"PrtMgr:82" FS "A"
#define MNEM_setDflts		"PrtMgr:83" FS "S"
#define MNEM_reset		"PrtMgr:84" FS "R"
#define MNEM_more		"PrtMgr:85" FS "O"

#define TXT_actions		"PrtMgr:86" FS "Actions"
#define MNEM_actions		"PrtMgr:87" FS "A"
#define TXT_exit		"PrtMgr:88" FS "Exit"
#define MNEM_exit		"PrtMgr:89" FS "E"

#define TXT_application		"PrtMgr:90" FS "Printer..."
#define MNEM_application	"PrtMgr:91" FS "P"
#define TXT_appHelp		"PrtMgr:92" FS "Printer"
#define TXT_appHelpSect		"PrtMgr:93" FS "10"
#define TXT_helpDesk		"PrtMgr:94" FS "Help Desk..."
#define MNEM_helpDesk		"PrtMgr:95" FS "H"
#define TXT_TOC			"PrtMgr:96" FS "Table of Contents..."
#define MNEM_TOC		"PrtMgr:97" FS "T"
#define TXT_tocHelp		"PrtMgr:98" FS "Table of Contents"
#define TXT_propHelp		"PrtMgr:99" FS "Properties"
#define TXT_propHelpSect	"PrtMgr:100" FS "120"
#define TXT_filterHelp		"PrtMgr:101" FS "Filter Options"
#define TXT_filterHelpSect	"PrtMgr:102" FS "180"

#define TXT_helpW		"PrtMgr:103" FS "Help..."
#define MNEM_helpW		"PrtMgr:104" FS "H"

#define TXT_appName		"PrtMgr:105" FS "Printer"

#define TXT_held		"PrtMgr:106" FS "held "
#define TXT_filtering		"PrtMgr:107" FS "filtering "
#define TXT_filtered		"PrtMgr:108" FS "filtered "
#define TXT_printing		"PrtMgr:109" FS "printing "
#define TXT_printed		"PrtMgr:110" FS "printed "
#define TXT_changing		"PrtMgr:111" FS "changing "
#define TXT_canceled		"PrtMgr:112" FS "canceled "
#define TXT_immediate		"PrtMgr:113" FS "next "
#define TXT_failed		"PrtMgr:114" FS "failed "
#define TXT_notify		"PrtMgr:115" FS "notify "
#define TXT_notifying		"PrtMgr:116" FS "notifying "

#define TXT_id			"PrtMgr:117" FS "Request ID:"
#define TXT_user		"PrtMgr:118" FS "User:"
#define TXT_size		"PrtMgr:119" FS "Size:"
#define TXT_date		"PrtMgr:120" FS "Date:"
#define TXT_state		"PrtMgr:121" FS "State:"
#define TXT_pgmTitle		"PrtMgr:122" FS "Printer:  %s"

#define TXT_print		"PrtMgr:123" FS "Print"
#define MNEM_print		"PrtMgr:124" FS "P"

#define TXT_deleteTitle		"PrtMgr:125" FS "Printer:  Delete Request"
#define TXT_errorTitle		"PrtMgr:126" FS "Printer:  Message"
#define TXT_noOpenOutput	"PrtMgr:127" FS \
    "Internal Error:  Cannot open output file!\n"
#define TXT_locale		"PrtMgr:128" FS "Locale:"
#define TXT_cLocale		"PrtMgr:129" FS "American English"
#define TXT_selection		"PrtMgr:130" FS "Selection: "
#define TXT_ok			"PrtMgr:131" FS "OK"
#define MNEM_ok			"PrtMgr:132" FS "O"
#define TXT_newlocale		"PrtMgr:133" FS "Locale (Code Set):"
#define TXT_nfilterHelpSect	"PrtMgr:134" FS "30"
#define TXT_no_dPrinter	"PrtMgr:135" FS "No default desktop printer defined.  Please define a default printer and try again."
#define TXT_no_Printer2		"PrtMgr:136" FS "You have not assigned a \"default\" printer.\n\nTo assign a default printer open \"Printer_Setup\" from the\nAdmin_Tools folder, click on a printer and select \"Make Default\"\nfrom the \"Printer\" menu.  If no printers appear within Printer_Setup\nthen you must first add a printer."
#define TXT_inputis		"PrtMgr:137" FS "File Type:"
#define TXT_Type1		"PrtMgr:138" FS "Text with Long Lines"
#define TXT_Type2		"PrtMgr:139" FS "Output from Troff"
#define MNEM_other		"PrtMgr:140" FS "T"
#define TXT_2propHelpSect	"PrtMgr:141" FS "20"

extern void	Error (Widget, char *);
extern void	ErrorConfirm (Widget, char *, XtCallbackProc, XtPointer);
extern char	*GetStr (char *idstr);

#endif /* ERROR_H */
