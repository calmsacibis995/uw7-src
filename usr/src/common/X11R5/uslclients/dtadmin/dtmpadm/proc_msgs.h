#ifndef NOIDENT
#pragma ident	"@(#)proc_msgs.h	1.4"
#endif

/*
 * This file contains all the message strings for the MP Desktop UNIX
 * dtadmin client: Processor Setup.
 *
 */

#define FS	"\001"
#define string_runprocs		"dtmp:1" FS "Processors  Total: %d  Online: %d "
#define string_runproc		"dtmp:2" FS "Currently %d processor is running."
#define string_online		"dtmp:3" FS "Processor %d is on line."
#define string_offline		"dtmp:4" FS "Processor %d is off line."
#define string_cannotonline	"dtmp:5" FS "Processor %d cannot be on line."
#define string_alreadyonline	"dtmp:6" FS "Processor %d is already on line."
#define string_cannotoffline	"dtmp:7" FS \
				"Processor %d cannot be taken off line."
#define string_alreadyoffline	"dtmp:8" FS \
				"Processor %d is already off line."	
#define string_alreadylicense	"dtmp:9" FS \
				"Processor %d is already licensed."
#define string_null		"dtmp:10" FS ""
#define string_licenseproc	"dtmp:11" FS "Enabling additional Processors"
#define string_info		"dtmp:12" FS "Information:\n%s"


#define label_online		"dtmp:20" FS "On-line"
#define label_offline		"dtmp:21" FS "Off-line"
#define label_license		"dtmp:22" FS "License..."
#define label_properties	"dtmp:23" FS "Properties..."
#define label_exit		"dtmp:24" FS "Exit"
#define label_mphelp		"dtmp:25" FS "MpHelp"
#define label_mpadmin		"dtmp:26" FS "Mp Admin..."
#define label_toc		"dtmp:27" FS "Table of Contents..."
#define label_helpdesk		"dtmp:28" FS "Help Desk..."
#define label_actions		"dtmp:29" FS "Actions"
#define label_help		"dtmp:30" FS "Help"
#define label_ok		"dtmp:31" FS "OK"
#define label_reset		"dtmp:32" FS "Reset"
#define label_cancel		"dtmp:33" FS "Cancel"
#define label_serialkey		"dtmp:34" FS "Serial Key:"
#define label_serialnum		"dtmp:35" FS "Serial Number:"
#define label_pid		"dtmp:36" FS "Processor id:"
#define label_proctype		"dtmp:37" FS "Processor Type:"
#define label_clockspeed	"dtmp:38" FS "Clock Speed:"
#define label_floatpttype	"dtmp:39" FS "Floating Point Type:"
#define label_iobus		"dtmp:40" FS "I/O Bus Type:"
#define label_separator		"dtmp:41" FS "separator"
#define label_apply		"dtmp:42" FS "Apply"
#define label_state		"dtmp:43" FS "State:"
#define label_OnLine		"dtmp:44" FS "On-Line"
#define label_OffLine		"dtmp:45" FS "OFF-Line"
			

#define mnemonic_ok		"dtmp:50" FS "O"
#define mnemonic_offline	"dtmp:51" FS "F"
#define mnemonic_license	"dtmp:52" FS "L"
#define mnemonic_properties	"dtmp:53" FS "P"
#define mnemonic_exit		"dtmp:54" FS "X"

#define mnemonic_mpadmin	"dtmp:56" FS "M"
#define mnemonic_toc		"dtmp:57" FS "T"
#define mnemonic_helpdesk	"dtmp:58" FS "D"
#define mnemonic_actions	"dtmp:59" FS "A"
#define mnemonic_help		"dtmp:60" FS "H"
#define mnemonic_separator	"dtmp:61" FS "S"

#define title_license		"dtmp:100" FS "Processor Setup: License"
#define title_properties	"dtmp:101" FS "Processor Setup: Properties"
#define title_procadmin		"dtmp:102" FS "Processor Administration"
#define title_information	"dtmp:103" FS "Processor Setup: Information"

#define title_toperr		"dtmp:104" FS "Processor Setup: Error"
#define ERR_cantGetProcInfo	"dtmp:105" FS "Cannot get Information on Processor %d"
#define ERR_cantGetNumProc	"dtmp:106" FS "Cannot get the total number of configured processor"
#define ERR_cantGetOnlineInfo	"dtmp:107" FS "Cannot check status on Process %d"
#define ERR_malloc		"dtmp:108" FS "Internal error: memory problem"
#define ERR_noItems		"dtmp:109" FS "Internal error: no processors"
#define TXT_iconName		"dtmp:110" FS "Processor_Setup"
#define mnemonic_exit2		"dtmp:111" FS "E"
#define mnemonic_online2	"dtmp:112" FS "n"
#define mnemonic_offline2	"dtmp:113" FS "f"
