#define TC_NSUBSBIND \
"TRACE:NAME: DL_SUBS_BIND_REQ/DL_SUBS_BIND_ACK\n"
#define SUBSBIND_PASS \
"INFO:DL_SUBS_BIND_REQ/DL_SUBS_BIND_ACK transaction is successful\n"
#define SUBSBIND_FAIL \
"INFO: - DL_SUBS_BIND_REQ/DL_SUBS_BIND_ACK transaction failure\n"
#define BADADDR_PASS1 \
"INFO:DL_SUBS_BIND_REQ to DL_ETHER sap fails with DL_BADADDR\n"
#define BADADDR_PASS2 \
"INFO:DL_SUBS_BIND_REQ to DL_CSMACD sap fails with DL_BADADDR\n"
#define BADADDR_FAIL0 \
"INFO: - DL_SUBS_BIND_REQ to DL_ETHER sap fails with %x [expected DL_BADADDR]\n"
#define BADADDR_FAIL1 \
"INFO: - DL_SUBS_BIND_REQ to DL_ETHER sap is succcessfull!!\n"
#define BADADDR_FAIL2 \
"INFO: - DL_SUBS_BIND_REQ to DL_CSMACD sap fails with %x [expected DL_BADADDR]\n"
#define BADADDR_FAIL3 \
"INFO: - DL_SUBS_BIND_REQ to FL_CSMACD sap is successfull!!\n"
#define OUTSTATE_PASS \
"INFO:DL_SUBS_BIND_REQ in an invalid state fails with DL_OUTSTATE\n"
#define OUTSTATE_FAIL0 \
"INFO: - DL_SUBS_BIND_REQ in an invalid state fails with %x [expected DL_OUTSTATE]\n"
#define OUTSTATE_FAIL1 \
"INFO: - DL_SUBS_BIND_REQ in an invalid state is successfull!!\n"
#define SYSERR_ASSERT \
"INFO:DL_SUBS_BIND_REQ fails with DL_SYSERR during system error.\n"
#define BADADDR_PASS3 \
"INFO:DL_SUBS_BIND_REQ with invalid DLSAP address fails with DL_BADADDR\n"
#define BADADDR_FAIL4 \
"INFO: - DL_SUBS_BIND_REQ with invalid DLSAP address is successfull!!\n"
#define BADADDR_FAIL5 \
"INFO: - DL_SUBS_BIND_REQ with invalid DLSAP address fails with %x [expected DL_BADADDR]\n"
