#define TC_NBIND \
"TRACE:NAME: BIND(Normal-User)\n"
#define TC_PBIND \
"TRACE:NAME: BIND(Previleged-User)\n"
#define BIND_PASS \
"INFO:Bind request is successful\n"
#define UNSUPPORTED_PASS \
"INFO:Request for unimplemented service mode fails with DL_UNSUPPORTED error\n"
#define UNSUPPORTED_FAIL0 \
"INFO: - Request for unimplemented service mode fails with %x [expected DL_UNSUPPORTED]\n" 
#define UNSUPPORTED_FAIL1 \
"INFO: - Request for unimplemented service mode passed [expected DL_UNSUPPORTED]\n" 	
#define PPA_INIT_FAIL \
"INFO:Bind request before PPA initialization fails with DL_NOTINIT\n"
#define OUTSTATE_PASS \
"INFO:Bind request in an invalid state fails with DL_OUTSTATE error\n"
#define OUTSTATE_FAIL0 \
"INFO: - Bind request in an invalid state fails with %x [expected DL_OUTSTATE]\n"
#define OUTSTATE_FAIL1 \
"INFO: - Bind request in an invalid state passed [expected DL_OUTSTATE]\n"
#define BADADDR_PASS \
"INFO:Bind request with invalid DLSAP address fails with DL_BADADDR error\n"
#define BADADDR_FAIL0 \
"INFO: - Bind request with invalid DLSAP address fails with %x [expected DL_BADADDR]\n" 
#define BADADDR_FAIL1 \
"INFO: - Bind request with invalid DLSAP address passed [expected DL_BADADDR]\n"
#define ACCESS_PASS \
"INFO:Previleged user is allowed to bind to promiscuous sap\n"
#define ACCESS_FAIL0 \
"INFO: - Previleged user is not allowed to bind to promiscuous sap\n"
#define ACCESS_FAIL1 \
"INFO: - Normal user is allowed to bind to promiscuous sap [expected DL_ACCESS]\n" 
#define ACCESS_FAIL2 \
"INFO: - Bind to promiscuous sap by normal user fails with %x [expected DL_ACCESS]\n"
#define ACCESS_PASS0 \
"INFO:Bind to promiscuous sap by normal user fails with DL_ACCESS\n"
#define NORMAL_BOUND_PASS \
"INFO:Attempt to bind already bound DLSAP fails with DL_BOUND for normal user\n"
#define NORMAL_BOUND_FAIL \
"INFO: - Even normal user is also allowed to bind an already bound DLSAP\n" 
#define NORMAL_BOUND_FAIL0 \
"INFO: - Attempt to bind already bound DLSAP fails with %x [expected DL_BOUND] for normal user\n"
#define BOUND_PASS \
"INFO:Previleged user is allowed to bind to an already bound DLSAP\n"
#define BOUND_FAIL0 \
"INFO: - Previleged user is not allowed to bind to an already bound DLSAP\n"
#define NOADDR_ASSERT\
"INFO:Bind request fails with DL_NOADDR if no DLSAP address is allocated\n"
#define MULTI_STREAM_BIND_PASS \
"INFO:DLPI allows multiple streams to be bound to the same DLSAP address\n"
#define MULTI_STREAM_BIND_FAIL \
"INFO: - DLPI does not allow multiple stream to be bound to the same DLSAP address\n"
#define WRONG_REQ_ACKD \
"INFO: - DLPI accepts bind request with wrong DL_BIND_REQ message size\n"
#define WRONG_REQ_PASS \
"INFO:DLPI rejects bind request with wrong DL_BIND_REQ messgae size\n"
