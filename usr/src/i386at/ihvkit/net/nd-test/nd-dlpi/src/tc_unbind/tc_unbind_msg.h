#define TC_UNBIND \
"TRACE:NAME: UNBIND\n"
#define UNBIND_PASS \
"INFO:Unbind request is successful\n"
#define OUTSTATE_PASS \
"INFO:Unbind request in an invalid state fails with DL_OUTSTATE error\n"
#define OUTSTATE_FAIL0 \
"INFO: - Unbind request in an invalid state fails with %x [not DL_OUTSTATE]\n"
#define OUTSTATE_FAIL1 \
"INFO: - Unbind request in an invalid state passed [not DL_OUTSTATE]\n"
#define SYSERR_ASSERT\
"INFO:Unbind request during system error fails with DL_SYSERR\n"
