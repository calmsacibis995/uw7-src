#define TC_FRAME \
"TRACE:NAME: FRAMETYPE\n"
#define RAW_LISTEN_INIT_FAIL \
"INFO: - Listener couldn't set to RAWCSMACD mode (%x)\n"
#define TEST_FRAME_SUCCESS \
"INFO: - The frame is in correct format\n"
#define TEST_FRAME_FAILURE \
"INFO: - The frame is not in correcr format\n"
#define TEST_FRAME_UNRESOLVED \
"INFO: - Can not conclude test frame results\n"
#define NOT_APPLICABLE \
"INFO: - Frametype not applicable to this media\n"

#define FRAME_REQ \
"INFO: - Request for %s frame check\n"
#define FRAME_INFO \
"INFO:The driver %s %s frames in proper format\n"
#define FRAME_PASS	"sends"
#define FRAME_FAIL	"does not send"
