#define TC_SEND \
"TRACE:NAME: DL_UNITDATA_REQ/DL_UNITDATA_IND\n"
#define LENINFO_FAIL \
"INFO: - Unable to get DLSDU limit information\n"
#define UNITDATAREQ_PASS \
"INFO:DL_UNITDATA_REQ sends a DLPI datagram successfully\n"
#define UNITDATAREQ_FAIL \
"INFO: - DL_UNITDATA_REQ could not send datagram successfully\n"
#define BADADDR_PASS \
"INFO:DL_UNITDATA_REQ fails with DL_BADADDR for invalid destination address format\n"
#define BADADDR_FAIL \
"INFO: - DL_UNITDATA_REQ fails with 0x%x [expected DL_BADADDR] for invalid destination address format\n"
#define BADADDR_FAIL0 \
"INFO: - DL_UNITDATA_REQ with invalid destination address format is not responded with DL_UDERROR_IND\n"
#define BADDATA_PASS \
"INFO:DL_UNITDATA_REQ fails with DL_BADDATA when datagram exceeds DLSDU limit\n"
#define BADDATA_FAIL \
"INFO: - DL_UNITDATA_REQ fails with 0x%x [expected DL_BADDATA] when datagram exceeds DLSDU limit\n"
#define BADDATA_FAIL0 \
"INFO: - DL_UNITDATA_REQ with invalid datgram length is not responded with DL_UDERROR_IND\n"
#define OUTSTATE_PASS \
"INFO:DL_UNITDATA_REQ issued from an invalid state fails with DL_OUTSTATE\n"
#define OUTSTATE_FAIL \
"INFO: - DL_UNITDATA_REQ issued from an invalid state fails with 0x%x [expected DL_OUTSTATE]\n"
#define OUTSTATE_FAIL0 \
"INFO: - DL_UNITDATA_REQ in an invalid state is not responded with DL_UDERROR_IND\n" 
#define SNDUD_MSG \
"INFO: - DL_UNITDATA_REQ with datagram length [%d]\n"
#define UNITDATA_BCONT_NULL_PASS \
"INFO:DL_UNITDATA_REQ with no data block is successfull\n"
#define UNITDATA_WRONG_SZ_PASS \
"INFO:DL_UNITDATA_REQ of invalid size is successfull\n"
#define BADADDR_TIMEOUT \
"INFO: - DL_UNITDATA_REQ with bad address does not respond with DL_UDERROR_IND\n"
#define SNDUD_TIMEOUT \
"INFO: - DL_UNIDATA_REQ with data length beyond DLSDU limit not responded with DL_UDERR_IND\n"
#define UNITDATAREQ_LOOP_PASS \
"INFO:DL_UNITDATA_REQ to itself is successfull\n"
#define UNITDATAREQ_LOOP_FAIL \
"INFO: - DL_UNITDATA_REQ to itself is not successfull!!\n"
#define BASIC_FRAME \
"INFO: DLPI sends frame correctly to itself\n"
#define WRONG_DATA \
"INFO: - DLPI received wrong data\n"
#define LOOP_RCV_FAILURE \
"INFO: - Loopback receive failure [dl_errno: %d errno: %d]\n"
#define LOOP_SEND_FAILURE \
"INFO: - Loopback send failure [dl_errno: %d errno: %d]\n"
