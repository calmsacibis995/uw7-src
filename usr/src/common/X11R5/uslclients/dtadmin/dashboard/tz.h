#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:dashboard/tz.h	1.5"
#endif

#include <X11/StringDefs.h>

/*
 *************************************************************************
 *
 * Forward definitions
 *
 **************************forward*declarations***************************
 */

#ifndef FS
#define FS      "\001"
#define FS_CHR      		'\001'
#endif

#define TXT_save		"tz:1" FS "Apply"		
#define TXT_cancel		"tz:2" FS "Cancel"		
#define TXT_timezones		"tz:3" FS "System Status: Time Zones"		
#define MNEM_save		"tz:4" FS "A"		
#define MNEM_cancel		"tz:5" FS "C"		
#define TXT_TZHelp		"tz:6" FS "System Status"		
#define TXT_TZHelpSect		"tz:7" FS "40" 
#define TXT_reset		"tz:8" FS "Reset"		
#define MNEM_reset		"tz:9" FS "R" 
#define MNEM_help		"tz:10" FS "H" 
#define TXT_GMTHelpSect		"tz:11" FS "40" 
#define TXT_GMTHelp		"tz:12" FS "System Status"		
#define TXT_GMT			"tz:13" FS "Enter your time relative to GMT:"
#define TXT_gmt			"tz:14" FS "System Status:Relative GMT TIME"		
#define TXT_Greenwich		"tz:15" FS "Greenwich"		
#define TXT_North_America	"tz:16" FS "North America"		
#define TXT_Korea		"tz:17" FS "Korea"		
#define TXT_Australia		"tz:18" FS "Australia"		
#define TXT_Asia		"tz:19" FS "Asia"		
#define TXT_Other		"tz:20" FS "Other"		
#define TXT_Daylight		"tz:21" FS "Daylight"		
#define TXT_NoDaylight		"tz:22" FS "No Daylight"		
#define TXT_Hawaii_Alaska	"tz:23" FS "Hawaii/Alaska"		
#define TXT_Yukon		"tz:24" FS "Yukon"		
#define TXT_Atlantic		"tz:25" FS "Atlantic"		
#define TXT_Eastern		"tz:26" FS "Eastern"		
#define TXT_Central		"tz:27" FS "Central"		
#define TXT_Mountain		"tz:28" FS "Mountain"		
#define TXT_Pacific		"tz:29" FS "Pacific"		
#define TXT_Thailand		"tz:30" FS "Thailand"		
#define TXT_Singapore		"tz:31" FS "Singapore"		
#define TXT_China		"tz:32" FS "China"		
#define TXT_Japan		"tz:33" FS "Japan"		
#define TXT_Mexico		"tz:34" FS "Mexico"		
#define TXT_Hongkong		"tz:35" FS "Hong Kong"		
#define TXT_Europe		"tz:36" FS "Europe"		
#define TXT_Eastern_Europe	"tz:37" FS "Eastern Europe"		
#define TXT_Western_Europe	"tz:38" FS "Western Europe"		
#define TXT_Great_Britain	"tz:39" FS "Great Britain"		
#define TXT_selectone		"tz:40" FS "Please select any one of the items in the list"
#define TXT_continue		"tz:41" FS "Continue"		
#define TXT_Fail		"tz:42" FS "Could not open /etc/TIMEZONE"
#define TXT_TZtitle		"tz:43" FS "System Status: Time Zones"
#define TXT_AsianZone		"tz:44" FS "Asian Time Zones" 
#define TXT_DSTNODST		"tz:45" FS "Choose One:"
#define TXT_wrongno		"tz:46" FS "Invalid Time Zone."
#define MNEM_continue		"tz:47" FS "C"
#define TXT_FailedtoCopy	"tz:48" FS "Failed to set up the time zone correctly"
#define TXT_AmericaZone		"tz:49" FS "American Time Zones"
#define TXT_EuropeZone		"tz:50" FS "European Time Zones"
#define TXT_Central_Europe	"tz:51" FS "Central Europe"		
#define TXT_AustraliaZone	"tz:52" FS "Australian Time Zones"
#define TXT_NorthA		"tz:53" FS "Northern Australia"		
#define TXT_WestA		"tz:54" FS "Western Australia"		
#define TXT_SouthA		"tz:55" FS "Southern Australia"		
#define TXT_Queensland		"tz:56" FS "Queensland"		
#define TXT_MexicoGeneral	"tz:57" FS "Mexico/General"		
#define TXT_OK			"tz:58" FS "OK"		
#define MNEM_OK			"tz:59" FS "O"		
#define TXT_help		"tz:60" FS "Help" 

#define MNEM_Greenwich           "tz:61" FS "G"
#define MNEM_North_America       "tz:62" FS "N"
#define MNEM_Europe              "tz:63" FS "E"
#define MNEM_Mexico              "tz:64" FS "M"
#define MNEM_Australia           "tz:65" FS "U"
#define MNEM_Asia                "tz:66" FS "A"
#define MNEM_Other               "tz:67" FS "O"

#define HELP_PATH		"dtadmin/dashboard.hlp"
#define DOUBLE			"double"

typedef struct _choice_items {
        XtArgVal     label;
        XtArgVal     select;
	XtArgVal     userData;
	XtArgVal     mnemonic;
} _choice_items;

typedef struct {
    char        *title;
    char        *file;
    char        *section;
} HelpText;

typedef struct {
    XtArgVal    lbl;
    XtArgVal    mnem;
    XtArgVal    sensitive;
    XtArgVal    selectProc;
    XtArgVal    dflt;
    XtArgVal  	clientData;
} MenuItem;


#define 		TZ_FILE	"desktop/dashboard/countries"
#define 		TZ_VARIABLE "TZ"
#define			TZ_EQUAL "=:"

#define TIMEZONE_FILE   "/etc/TIMEZONE"
#define TMP_TIMEZONE   "/tmp/TIMEZONE"
#define TIME_COMMAND	"cp /tmp/TIMEZONE /etc/TIMEZONE"
#define TIME_COMMAND2	"rm /tmp/TIMEZONE"
#define exportTZ	"export TZ;"
#define NEWLINE		"\n"

