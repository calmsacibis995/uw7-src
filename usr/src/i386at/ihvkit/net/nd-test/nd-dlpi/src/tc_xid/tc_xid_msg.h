
#define TC_XID \
"TRACE:NAME: DL_XID_REQ/DL_XID_CON"
#define XIDREQ_PASS \
"INFO:DL_XID_REQ/DL_XID_CON transaction is successful\n"
#define XIDREQ_FAIL \
"INFO: - DL_XID_REQ/DL_XID_CON transaction is not successful\n"
#define XID_INIT_FAIL \
"INFO:The testreq test could not be initialized\n"
#define BADADDR_PASS \
"INFO: - DL_XID_REQ fails with DL_BADADDR for invalid destination address format\n"
#define BADADDR_FAIL \
"INFO: - DL_XID_REQ fails with 0x%x [expected DL_BADADDR] for invalid destination address format\n"
#define BADADDR_FAIL0 \
"INFO: - DL_XID_REQ with invalid destination address format is not responded with DL_ERROR_ACK\n"
#define OUTSTATE_PASS \
"INFO:DL_XID_REQ issued from an invalid state fails with DL_OUTSTATE\n"
#define OUTSTATE_FAIL \
"INFO: - DL_XID_REQ issued from an invalid state fails with 0x%x [expected DL_OUTSTATE]\n"
#define OUTSTATE_FAIL0 \
"INFO: - DL_XID_REQ in an invalid state is not responded with DL_ERROR_ACK\n" 
#define XID_WRONG_SZ_PASS \
"INFO: - DL_XID_REQ of invalid size is successfull\n"
#define BADADDR_TIMEOUT \
"INFO: - DL_XID_REQ with bad address does not respond with DL_ERROR_ACK\n"
#define XIDREQ_INVALARG_PASS0 \
"INFO:DL_XID_REQ with invalid argument is rejected with DL_ERROR_ACk\n"
#define XIDREQ_INVALARG_PASS1 \
"INFO:DL_XID_REQ with invalid argument is rejected with DL_UDERROR_IND\n"
