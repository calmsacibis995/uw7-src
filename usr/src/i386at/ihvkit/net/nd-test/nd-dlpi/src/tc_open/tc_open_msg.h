#define TC_OPEN \
"TRACE:NAME: OPEN\n"
#define	OPEN_PASS \
"INFO:DLPI open is successful\n"
#define ECHRNG_ASSERT \
"INFO:DLPI open should fail with ECHRNG after finite no of attempts\n"
#define ECHRNG_PASS \
"INFO: - DLPI open fails with ECHRNG after %x no of attempts\n"
#define ECHRNG_FAIL0 \
"INFO: - DLPI open fails with %x [not ECHRNG] after %x no of attempts\n"  
#define ECHRNG_FAIL1 \
"INFO: - DLPI open does not fail even after %x attempts\n" 
#define ENXIO_PASS \
"INFO:DLPI open fails with ENXIO when the board is not present\n"
#define ENXIO_FAIL0 \
"INFO: - DLPI open fails with %x [not ENXIO] when the board is not present\n"
#define ENXIO_FAIL1 \
"INFO: - DLPI open does not fail even when the board is not present\n"
