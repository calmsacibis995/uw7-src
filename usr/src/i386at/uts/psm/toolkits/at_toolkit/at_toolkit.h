#ident	"@(#)kern-i386at:psm/toolkits/at_toolkit/at_toolkit.h	1.4.2.1"
#ident	"$Header$"

#ifndef _PSM_AT_TOOLKIT_AT_TOOLKIT_H	/* wrapper symbol for kernel use */
#define _PSM_AT_TOOLKIT_AT_TOOLKIT_H	/* subject to change without notice */

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_PSM)
#if _PSM != 2
#error "Unsupported PSM version"
#endif
#else
#error "Header file not valid for Core OS"
#endif

#ifndef __PSM_SYMREF

struct __PSM_dummy_st {
	int *__dummy1__;
	const struct __PSM_dummy_st *__dummy2__;
};
#define __PSM_SYMREF(symbol) \
	extern int symbol; \
	static const struct __PSM_dummy_st __dummy_PSM = \
		{ &symbol, &__dummy_PSM }

#if !defined(_PSM)
__PSM_SYMREF(No_PSM_version_defined);
#pragma weak No_PSM_version_defined
#else
#define __PSM_ver(x) _PSM_##x
#define _PSM_ver(x) __PSM_ver(x)
__PSM_SYMREF(_PSM_ver(_PSM));
#endif

#endif  /* __PSM_SYMREF */

#include <svc/psm.h>



/*
 *         INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *     This software is supplied under the terms of a license 
 *    agreement or nondisclosure agreement with Intel Corpo-
 *    ration and may not be copied or disclosed except in
 *    accordance with the terms of that agreement.
 */

/*
 * Softreset related definitions
 */

#define AT_BIOS_SEG     0xf000
#define AT_BIOS_INIT    0xfff0
#define AT_RESET_FLAG   0x1234
#define AT_CMOS_ADDR    0x70    /* I/O port address for CMOS ram address */
#define AT_CMOS_DATA    0x71    /* I/O port address for CMOS ram data */
#define AT_SHUTDOWN_ADDR 0x0F
#define AT_SOFT_ADDR	0x8F
#define AT_SOFT_RESET   0x05
#define AT_WARM_RESET   0x0A

/*
 * System reset related definitions
 */

#define AT_KB_CMD_ADDR  0x64
#define AT_KB_RESET     0xFE

void    psm_warmreset();
void    psm_softreset(unsigned char *);
void    psm_sysreset();

#if defined(__cplusplus)
	}
#endif

#endif /* _PSM_AT_TOOLKIT_AT_TOOLKIT_H */
