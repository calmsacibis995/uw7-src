#define TC_NCLOSE \
"TRACE:NAME: CLOSE(Normal-User)\n"
#define TC_PCLOSE \
"TRACE:NAME: CLOSE(Previleged-User)\n"
#define	CLOSE_PASS \
"INFO:DLPI close is successful\n"
#define AFTER_CLOSE_PASS \
"INFO:Ioctl operation on a closed stream fails with an error\n"
#define AFTER_CLOSE_FAIL \
"INFO: - Ioctl operation on a closed stream passed\n"
#define CLOSE_PROMISC_PASS \
"INFO:Close of stream bound to promiscuous sap takes the board out of promiscuos mode\n"
#define CLOSE_PROMISC_FAIL \
"INFO: - Closing of DLPI stream bound to promiscuous sap didn't take the board out of\npromiscuous mode\n"
#define SET_PROMISC_FAIL \
"INFO: - Bind request to promiscuous sap does not put the board in promiscuous mode\n"
