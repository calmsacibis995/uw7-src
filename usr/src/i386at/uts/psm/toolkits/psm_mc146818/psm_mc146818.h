#ifndef _PSM_PSM_MC146818_H	/* wrapper symbol for kernel use */
#define _PSM_PSM_MC146818_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:psm/toolkits/psm_mc146818/psm_mc146818.h	1.3.1.1"
#ident	"$Header$"

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

/*
 * Definitions for Real Time Clock driver (Motorola MC146818 chip).
 */

#define	MC146818_ADDR	0x70	/* I/O port address of for register select */
#define	MC146818_DATA	0x71	/* I/O port address for data read/write */

/*
 * Register A definitions
 */
#define	MC146818_A	0x0a	/* register A address */
#define	MC146818_UIP	0x80	/* Update in progress bit */
#define	MC146818_DIV0	0x00	/* Time base of 4.194304 MHz */
#define	MC146818_DIV1	0x10	/* Time base of 1.048576 MHz */
#define	MC146818_DIV2	0x20	/* Time base of 32.768 KHz */
#define	MC146818_RATE6	0x06	/* interrupt rate of 976.562 */

/*
 * Register B definitions
 */
#define	MC146818_B	0x0b	/* register B address */
#define	MC146818_SET	0x80	/* stop updates for time set */
#define	MC146818_PIE	0x40	/* Periodic interrupt enable */
#define	MC146818_AIE	0x20	/* Alarm interrupt enable */
#define	MC146818_UIE	0x10	/* Update ended interrupt enable */
#define	MC146818_SQWE	0x08	/* Square wave enable */
#define	MC146818_DM	0x04	/* Date mode, 1 = binary, 0 = BCD */
#define	MC146818_HM	0x02	/* hour mode, 1 = 24 hour, 0 = 12 hour */
#define	MC146818_DSE	0x01	/* Daylight savings enable */

/* 
 * Register C definitions
 */
#define	MC146818_C	0x0c	/* register C address */
#define	MC146818_IRQF	0x80	/* IRQ flag */
#define	MC146818_PF	0x40	/* PF flag bit */
#define	MC146818_AF	0x20	/* AF flag bit */
#define	MC146818_UF	0x10	/* UF flag bit */

/*
 * Register D definitions
 */
#define	MC146818_D	0x0d	/* register D address */
#define	MC146818_VRT	0x80	/* Valid RAM and time bit */

#define	MC146818_NREG	0x0e	/* number of RTC registers */
#define	MC146818_NREGP	0x0a	/* number of RTC registers to set time */


typedef struct {
	char	rtc_sec;
	char	rtc_asec;
	char	rtc_min;
	char	rtc_amin;
	char	rtc_hr;
	char	rtc_ahr;
	char	rtc_dow;
	char	rtc_dom;
	char	rtc_mon;
	char	rtc_yr;
	char	rtc_statusa;
	char	rtc_statusb;
	char	rtc_statusc;
	char	rtc_statusd;
} psm_mc146818_time_t;


extern void psm_mc146818_init(void);
extern int  psm_mc146818_getsec(void);
ms_bool_t   psm_mc146818_rtodc(ms_daytime_t *);
ms_bool_t   psm_mc146818_wtodc(ms_daytime_t *);

#if defined(__cplusplus)
	}
#endif

#endif /* _PSM_PSM_MC146818_H */
