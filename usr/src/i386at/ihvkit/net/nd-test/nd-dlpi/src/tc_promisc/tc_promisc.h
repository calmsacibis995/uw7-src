#define TC_PROMISC \
"TRACE:NAME: Promiscuous mode tests\n"

/* messages */

#define PROMISCMODE_PASS \
"INFO: Enable, get and disable promiscuous mode flag is successful"
#define PROMISCON_FAIL \
"INFO: Cannot enable promiscuous mode"
#define GPROMISC_FAIL \
"INFO: Cannot get promiscuous flag"
#define PROMISCOFF_FAIL \
"INFO: Cannot disable promiscuous mode"
#define PROMISCMATCH_FAIL \
"INFO: Promiscuous mode is not enabled after a successful enable request"
#define SAPPROMISC_PASS \
"INFO: SAP Promiscuous mode gets packets destined for all the saps"
#define SAPPROMISC_FAIL \
"INFO: SAP Promiscuous mode failed to get packets destined for all the saps"
#define PHYSPROMISC_PASS \
"INFO: PHYS Promiscuous mode gets all the packets "
#define PHYSPROMISC_FAIL \
"INFO: PHYS Promiscuous mode failed to get all the packets"
