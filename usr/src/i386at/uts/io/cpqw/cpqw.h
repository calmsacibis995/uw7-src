#ident	"@(#)kern-i386at:io/cpqw/cpqw.h	1.5.2.1"

/********************************************************
 * Copyright 1994, COMPAQ Computer Corporation
 ********************************************************
 *
 * Title   : $RCSfile$
 *
 * Version : $Revision$
 *
 * Date    : $Date$
 *
 * Author  : $Author$
 *
 ********************************************************
 *
 * Change Log :
 *
 *           $Log$
 * Revision 1.2  1996/03/11  21:24:45  jayb
 * PCI BUS UTILIZATION changes.
 *
 * Revision 1.18  1995/10/05  22:27:29  gandhi
 * defined CSM_LOG_CRIT_ERROR instead of CRIT_LOG_DUMMY_ENTRY.
 *
 * Revision 1.17  1995/04/17  18:42:22  gandhi
 * defined an ioctl ECC_TOTAL_ERRORS that returns the total number of ECC
 * errors that have occurred since we booted.
 *
 * Revision 1.16  1995/04/07  23:50:43  gandhi
 * Added Novell copyright message.  Code as released in N5.1
 *
 * Revision 1.15  1995/03/08  16:06:57  gandhi
 * Added #ident line for USL to keep track of sccs releases.
 *
 * Revision 1.14  1995/02/14  16:31:03  gandhi
 * Removed CSM_FAILURE_T structure.  It has already been moved to
 * cpqw_kernel.h.  It is no longer visible to application programs.
 * Added new ioctls: GET_REQUIRED_FANS, GET_PROCESSOR_FANS,
 * GET_TYPE_STRING, GET_EISA_BUS_UTIL, GET_CSM_STATUS to allow SNMP agents
 * access to various information that was previously obtained via nlist calls.
 *  Added #defines for return values for CSM_GET_FAN/TEMP_STATUS.
 * Defined a new ioctl ASR_GET_DAEMON_STATUS and its corresponding
 *  return values to get the snmp agent figure out if the cpqasrd daemon is
 *  running or not.
 *
 * Revision 1.13  1995/01/05  22:31:57  gandhi
 * Added an ioctl command for asr - ASR_GET_DAEMON_STATUS whose values could
 * either be ASR_DAEMON_STOPPED or STARTED.
 *
 * Revision 1.12  1995/01/03  21:10:01  gandhi
 * defined 2 new ioctl commands for ASR:  ASR_START and ASR_STOP.
 *
 * Revision 1.11  1994/12/07  20:49:37  gandhi
 * Swapped the syndrome and DDR fields.  They were at incorrect offsets.
 * (in correctable_error_t structure).
 *
 * Revision 1.10  1994/09/09  16:58:57  gandhi
 * Added RCS header.
 *
 *           $EndLog$
 *
 ********************************************************/

#ifndef INCLUDE_CPQW_H
#define INCLUDE_CPQW_H

#define ENABLED			1
#define DISABLED		0

#undef TRUE
#undef FALSE
#define TRUE			1
#define FALSE			0

/*
 * ioctl commands - ASR
 * The daemon protocol should be: ASR_START, forever(ASR_RELOAD), ASR_STOP
 * One can issue an ASR_ENABLE/DISABLE at any time.
 */
#define ASR_RELOAD		0x02	/* ASR_RESTART */
#define ASR_DISABLE		0x03
#define ASR_ENABLE		0x04
#define ASR_GET_FREEFORM_DATA	0x05
#define ASR_GET_STATUS		0x06	/* See below for status */
#define ASR_START		0x07
#define ASR_STOP		0x08
#define ASR_GET_DAEMON_STATUS	0x09	/* See below for status */

/* ioctl commands - ECC */
#define SOFT_NMI		0x11	/* Generate a software NMI */
#define ECC_ENABLE		0x12	/* Enable ECC (if supported) */
#define ECC_SET_PARAMS		0x13	/* Set ECC params (interval, maxerr) */
#define	ECC_IS_SUPPORTED	0x14	/* Is ECC supported? */
#define	ECC_IS_ENABLED		0x15	/* Is ECC enabled? */
#define	ECC_LOG_DUMMY_ENTRY	0x16	/* Log a dummy corr entry */
#define	ECC_TOTAL_ERRORS	0x17	/* # of ECC errors since we booted */


/* ioctl commands - CSM */
#define EISA_BUS_UTIL_MONITOR		0x21	/* Enable/Disable monitoring */
#define	CSM_GET_FAN_STATUS		0x22	/* Get Fan Status */
#define	CSM_GET_TEMP_STATUS		0x23	/* Get Temperature Status */
#define	CSM_GET_FTPS_STATUS		0x24	/* Get FTPS Status */
#define	CRIT_LOGTABLE_ENTRY_SET_CORR	0x25	/* Correct a Critlog entry */
#define	CORR_LOGTABLE_ENTRY_SET_CORR	0x26	/* Correct a CorrLog entry */
#define	CRIT_LOG_DUMMY_ENTRY		0x27	/* Log a dummy crit entry */
#define	CSM_GET_RTC_BATTERY_STATUS	0x28	/* Get status of RTC Battery */
#define	CSM_GET_REQUIRED_FANS		0x29	/* Get bitmask of reqd fans */
#define	CSM_GET_PROCESSOR_FANS		0x30	/* Get bitmask of proc fans */
#define	CSM_GET_TYPE_STRING		0x31	/* Get CSM type string info */
#define	CSM_GET_EISA_BUS_UTIL		0x32	/* Get EISA bus util stats */
#define	CSM_GET_CSM_STATUS		0x33	/* CSM enabled/disabled */
#define	CSM_GET_PCI_BUS_UTIL		0x34	/* Get PCI bus util stats */
#define	CSM_LOG_CRIT_ERROR		CRIT_LOG_DUMMY_ENTRY

/* Values for ASR_GET_STATUS */
#define ASR_FAILED		(-1)
#define ASR_DISABLED		0
#define ASR_ENABLED		1

/* Values for ASR_GET_DAEMON_STATUS */
#define ASR_DAEMON_STOPPED	0	/* The cpqasrd daemon is stopped */
#define ASR_DAEMON_STARTED	1	/* The cpqasrd daemon is running */


/* Values for RTC_BATTERY_STATUS */
#define RTC_BATTERY_STATUS_UNKNOWN		(-1)
#define RTC_BATTERY_STATUS_LOW			0
#define RTC_BATTERY_STATUS_OK			1
#define RTC_BATTERY_STATUS_NOTSUPPORTED		2

/* Argument for EISA_BUS_UTIL_MONITOR ioctl() command */
#define	ENABLE_MONITORING	1	/* Enable monitoring */
#define	DISABLE_MONITORING	0	/* Disable monitoring */

/* Values for CSM_GET_CSM_STATUS */
#define	CSM_IS_ENABLED		0x00000001	/* CSM is enabled */
#define	CSM_IS_PRESENT		0x00000100	/* CSM is present */

/*
 * Values for CSM_GET_FAN_STATUS:
 * Each bit represents a failed fan.  A total of 8 fans are defined.
 * Bit #0 implies fan #1, bit #1 implies fan #2 and so on.
 */
#define CSM_FAN1_FAILED		0x00000001	/* Fan #1 failed */
#define CSM_FAN2_FAILED		0x00000002	/* Fan #2 failed */
#define CSM_FAN3_FAILED		0x00000004	/* Fan #3 failed */
#define CSM_FAN4_FAILED		0x00000008	/* Fan #4 failed */
#define CSM_FAN5_FAILED		0x00000010	/* Fan #5 failed */
#define CSM_FAN6_FAILED		0x00000020	/* Fan #6 failed */
#define CSM_FAN7_FAILED		0x00000040	/* Fan #7 failed */
#define CSM_FAN8_FAILED		0x00000080	/* Fan #8 failed */

/*
 * Values for CSM_GET_TEMP_STATUS:
 * Each bit represents an alert form a temperature sensor.  A total of
 * 7 temperature sensors are defined.
 * Bit #0 implies temp #1, bit #1 implies temp #2 and so on.
 * Bit #7 is used to indicate Deadly Temperature Conditions.  If it is
 * clear, the temperature condition is only "Caution" (not Critical).
 */
#define CSM_TEMP1_FAILED	0x00000001	/* Temp #1 failed */
#define CSM_TEMP2_FAILED	0x00000002	/* Temp #2 failed */
#define CSM_TEMP3_FAILED	0x00000004	/* Temp #3 failed */
#define CSM_TEMP4_FAILED	0x00000008	/* Temp #4 failed */
#define CSM_TEMP5_FAILED	0x00000010	/* Temp #5 failed */
#define CSM_TEMP6_FAILED	0x00000020	/* Temp #6 failed */
#define CSM_TEMP7_FAILED	0x00000040	/* Temp #7 failed */

#define CSM_TEMP_ISDEADLY	0x00000080	/* Temp condition is Deadly */

#pragma pack(1)

/* Server Management EV - CPHCSM */
typedef struct CQHCSM_T {
	unsigned char
		eaas,				/* EAAS byte */
		quicktest_rom_date[3],		/* Quicktest ROM date */
		confirm_recovery_required,	/* EAAS/ASR conformation of
						 * recovery required */
		eaas_shutdown_occurred;		/* eaas shutdown occurred */
} cqhcsm_t;

/* CSM type string structure */
typedef struct CSM_TYPE_STRING_T {
	char length;			/* length of string */
	char csm_interrupt;		/* CSM interrupt value */
	char fan_slots;			/* bit denotes if a fan slot exists */
	char fans_required;		/* bit denotes fans required for */
					/*   corresponding slot */
	char fans_present;		/* bit denotes fans installed */
	char processor_fans;		/* bit denotes if the fan is a */
					/*   processor fan */
	char bclk_period;		/* BCLK period in nano-seconds */
	short max_eisa_util_interval;	/* maximum EISA bus util. interval */
					/*   between two samples (in seconds) */
	short csm_base_addr;		/* base address of the CSM */
	unsigned short feature_word;	/* 16-bit value defining the features */
} csm_type_string_t;

#define ISM_FREEI2_BATTERY_SENSE	0x01

/* critical error log structure */
typedef struct CRITICAL_ERROR_T {
	unsigned long	type		:7,
			corrected	:1,
			hour		:5,
			day		:5,
			month		:4,
			year		:7,
			reserved	:3;
	unsigned long	error_info;
} critical_error_t;

/* correctable error log structure */
typedef struct CORRECTABLE_ERROR_T {
	unsigned long	count		:8,
			hour		:5,
			day		:5,
			month		:4,
			year		:7,
			reserved	:3;
	unsigned short	syndrome;
	unsigned short	DDR;
} correctable_error_t;

/* ASR freeform data structure */
typedef struct asr_freeform {
	unsigned char num_bytes;
	unsigned char major_version;
	unsigned char minor_version;
	unsigned char timeout_value;
	unsigned char reserved;
	unsigned short base_addr;
	unsigned short scale_value;
} asr_freeform_t;

/* eisa bus utilization structure */
#define	EISA_BUS_UTIL_MAX_ENTRIES	60
typedef struct EISA_BUS_UTIL_T {
	int	idletime_hz;		/* Number of nano seconds per BCLK
					 * tick.  Used to convert idletime[]
					 * to nano seconds. */
	int	interval_hz;		/* Number of clock ticks per second.
					 * Used to convert interval[] to
					 * nano seconds. */
	int	num_filled_entries;	/* entries filled in Utilization[] */
	int	curr_index;		/* Index of current entry */
	unsigned long	interval[EISA_BUS_UTIL_MAX_ENTRIES];
					/* interval (in clock ticks) between
					 * collections (approx. 1 second) */
	unsigned long	idletime[EISA_BUS_UTIL_MAX_ENTRIES];
					/* EISA bus idle time between
					 * collections (in BCLK ticks).
					 * Multiplying this value by
					 * idletime_hz gives the
					 * value in nano seconds. */
} eisa_bus_util_t;

#pragma pack()

/* Structure for use with ECC_SET_PARAMS */
typedef struct ECC_SETUP_T {
	long max_ECC_errors;		/* maximum ECC errors per interval */
	long ECC_interval;		/* time in seconds */
} ecc_setup_t;


/*
 * Fault Tolerant Power Supply
 */

/* cpqHeFltTolPwrSupplyCondition  */
#define FTPSCONDITION_DONT_EXIST	1	/* FTPS not supported or
						 * not installed */
#define FTPSCONDITION_OK		2	/* FTPS detected */
#define FTPSCONDITION_DEGRADED		3	/* FTPS not detected */
#define FTPSCONDITION_DONTKNOW		4	/* can't determine */

/* cpqHeFltTolPwrSupplyStatus  */
#define FTPSSTATUS_DONTKNOW		1	/* can't determine */
#define FTPSSTATUS_NOTSUPPORTED		2	/* No CSM, therefore no FTPS */
#define FTPSSTATUS_NOTINSTALLED		3	/* FTPS not installed */
#define FTPSSTATUS_INSTALLED		4	/* FTPS installed & detected */

typedef struct {
	int	ftps_status;
	int	ftps_condition;
} snmp_ftps_t;

#endif /* INCLUDE_CPQW_H */
