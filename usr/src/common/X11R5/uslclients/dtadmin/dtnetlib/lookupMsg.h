#ifndef NOIDENT
#pragma ident	"@(#)dtnetlib:lookupMsg.h	1.13"
#endif

#define FS    "\001"
#define TXT_OK		"lookup:1" FS	"OK"
#define TXT_Cancel	"lookup:2" FS	"Cancel"
#define TXT_Help	"lookup:3" FS	"Help"
#define TXT_dot		"lookup:4" FS	"."

#define TXT_etcListHeader	"lookup:5" FS "%-14s  %-15s  %-25s"
#define TXT_etcName	"lookup:6" FS "Name"
#define TXT_etcAddr	"lookup:7" FS "Address"
#define TXT_etcComment	"lookup:8" FS "Comment"

#define TXT_domain	"lookup:9" FS	"Domain:"
#define TXT_domainBut	"lookup:10" FS	"Update Listing"

#define TXT_HostTitle   "lookup:11" FS "Remote System - Lookup"
#define TXT_view        "lookup:12" FS "View:"
#define TXT_domainList	"lookup:13" FS "Domain Listing"
#define TXT_systemList	"lookup:14" FS "Systems List"
#define TXT_selection	"lookup:15" FS "Selection:"
#define TXT_etcLabel	"lookup:16" FS "Systems defined in Local Systems List (/etc/hosts)"
#define TXT_status  	"lookup:17" FS "Enter the system name."

#define NM_view		"lookup:18" FS "V"
#define NM_domainList	"lookup:19" FS "D"
#define NM_systemList	"lookup:20" FS "S"

#define TXT_info	"lookup:21" FS "Information"

/* Internal error message */
#define ERR_unkPanePos	"lookup:30" FS	"Internal Error: Unknown DNS pane position."
#define ERR_unkQuery	"lookup:31" FS	"Internal Error: Unknown Query type."
#define ERR_cantReadDns	"lookup:32" FS	"Internal Error: Can't read DNS."
#define ERR_cantFork	"lookup:33" FS	"Internal Error: Can't fork process."

#define INFO_noHelp	"lookup:34" FS	"No help facility in q3."
#define INFO_dnsRunning	"lookup:35" FS	"Running Dns... Pres OK to stop the process."
/* New in q4 */
#define ERR_noNameServer	"lookup:36" FS "Cannot find a Name Server for %s."
#define ERR_cantOpenEtcHost	"lookup:37" FS 	"Cannot open /etc/hosts file."

#define INFO_dnsRunning1	"lookup:38" FS "Looking up Domain info. for %s.  This will take a while ..."

#define ERR_noSetup	"lookup:39" FS	"Cannot display Lookup window.\n\n You do not have any Systems List entries (beyond your own) nor is your system configured for DNS access."

#define ERR_cantWait	"lookup:40" FS	"Internal Error: Can't wait the child."
#define ERR_killBySignal	"lookup:41" FS	"Lookup process terminated."
#define ERR_unkown	"lookup:42" FS	"Lookup terminated for unknown reason."
#define ERR_remoteFile	"lookup:43" FS	"Cannot drag an entry from this window."

#define TXT_etcLabel2	"lookup:44" FS "Local Systems List (/etc/hosts)"
#define TXT_Stop	"lookup:45" FS "Stop"
#define TXT_error	"lookup:46" FS "Error"
#define INFO_nextServer	"lookup:47" FS "Query on %s failed. Try the next Server..."
#define ERR_noHostInfo	"lookup:48" FS "Cannot find host information for %s."
#define TXT_inProgress	"lookup:49" FS "Lookup in Progress."
/* resolver's error messages */
#define ERR_HOST_NOT_FOUND	"lookup:50" FS "unkown domain %s."
#define ERR_NO_DATA	"lookup:51" FS "No NS records for %s."
#define ERR_TRY_AGAIN	"lookup:52" FS "No response for NS query for %s."
#define ERR_UNEXPECTED_ERR	"lookup:53" FS "Unexpected error from %s."
#define ERR_MEMORY	"lookup:54" FS "Memory problem while getting NS for %s."
#define ERR_EXPAND	"lookup:55" FS "Cannot expand the address for %s."
#define ERR_NO_ADDR	"lookup:56" FS "There is no address for this name server: %s."
#define ERR_MKQUERY	"lookup:57" FS "res_mkquery failed."
#define ERR_NO_NS_RUNNING	"lookup:58" FS "There is no name server running for %s."
#define ERR_NO_SOC_CONN	"lookup:59" FS "socket connection failed for %s."
#define ERR_CANNOT_WRITE_SOCKET "lookup:60" FS "write to socket failed."
#define ERR_CANNOT_READ_SOCKET	"lookup:61" FS "error reading from socket."
#define ERR_DN_SKIPNAME		"lookup:62" FS "dn_skipname failed."
#define ERR_FORMAT	"lookup:63" FS "format error."
#define ERR_SERVER	"lookup:64" FS "server failure %s."
#define ERR_NOT_IMP	"lookup:65" FS "not implemented."
#define ERR_REFUSED	"lookup:66" FS "query refused on %s."
#define ERR_NO_ANSWER	"lookup:67" FS "no anwer resource records."
#define ERR_NO_SOC_CRT	"lookup:68" FS "socket creation failed."
#define ERR_NOT_EXISTED	"lookup:69" FS "domain %s not existed."

/* New for q5 */
#define INFO_dnsRunning2	"lookup:70" FS "Looking up Domain info. for %s.  \n\nThis may take a while ..."
#define ERR_HOST_NOT_FOUND1	"lookup:71" FS "Unknown domain: %s."
#define ERR_MEMORY1	"lookup:72" FS "Memory problem while retrieving Name Servers for %s."
#define ERR_NO_SOC_CONN1	"lookup:73" FS "Cannot connect to Name Server for %s.  \n\nReason: Network socket connection failed."
#define ERR_CANNOT_WRITE_SOCKET1 "lookup:74" FS "Write to network socket failed.  Contact your system administrator."
#define ERR_CANNOT_READ_SOCKET1	"lookup:75" FS "Error reading from network socket. Contact your system administrator."
#define ERR_REFUSED1	"lookup:76" FS "Request for domain info. for %s was refused."
#define ERR_NO_ANSWER1	"lookup:77" FS "This domain has no systems."
#define ERR_NO_SOC_CRT1	"lookup:78" FS "Network socket creation failed. Contact your system administrator."
#define ERR_NOT_EXISTED1	"lookup:79" FS "%s does not exist."
#define TXT_INTERNET	"lookup:80" FS "Internet Setup"

/* new q9 messages */

#define MNE_update	"lookup:81" FS "U"
#define MNE_OK		"lookup:82" FS "O"
#define MNE_Cancel	"lookup:83" FS "C"
#define MNE_Help	"lookup:84" FS "H"
