#define TC_TEST \
"TRACE:NAME: DL_TEST_REQ/DL_TEST_CON"
#define TESTREQ_PASS \
"INFO:DL_TEST_REQ/DL_TEST_CON transaction is successful\n"
#define TESTREQ_FAIL \
"INFO: - DL_TEST_REQ/DL_TEST_CON transaction is not successful\n"
#define TEST_INIT_FAIL \
"INFO:The testreq test could not be initialized\n"
#define BADADDR_PASS \
"INFO: - DL_TEST_REQ fails with DL_BADADDR for invalid destination address format\n"
#define BADADDR_FAIL \
"INFO: - DL_TEST_REQ fails with 0x%x [expected DL_BADADDR] for invalid destination address format\n"
#define BADADDR_FAIL0 \
"INFO: - DL_TEST_REQ with invalid destination address format is not responded with DL_ERROR_ACK\n"
#define OUTSTATE_PASS \
"INFO:DL_TEST_REQ issued from an invalid state fails with DL_OUTSTATE\n"
#define OUTSTATE_FAIL \
"INFO: - DL_TEST_REQ issued from an invalid state fails with 0x%x [expected DL_OUTSTATE]\n"
#define OUTSTATE_FAIL0 \
"INFO: - DL_TEST_REQ in an invalid state is not responded with DL_ERROR_ACK\n" 
#define TEST_WRONG_SZ_PASS \
"INFO: - DL_TEST_REQ of invalid size is successfull\n"
#define BADADDR_TIMEOUT \
"INFO: - DL_TEST_REQ with bad address does not respond with DL_ERROR_ACK\n"
#define TESTREQ_INVALARG_PASS0 \
"INFO:DL_TEST_REQ with invalid argument is rejected with DL_ERROR_ACk\n"
#define TESTREQ_INVALARG_PASS1 \
"INFO:DL_TEST_REQ with invalid argument is rejected with DL_UDERROR_IND\n"
