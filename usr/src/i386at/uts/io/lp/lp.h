#ident	"@(#)lp.h	1.2"

#ifndef _IO_LP_LP_H	/* wrapper symbol for kernel use */
#define _IO_LP_LP_H	/* subject to change without notice */


#if defined(__cplusplus)
extern "C" {
#endif


#if defined(_KERNEL)

/* 
 * If LP_BOUND characters are printed after acquiring the port, then 
 * relinquish the port and request it again. This will prevent the lp 
 * driver from holding the port for too long.
 */
#define LP_BOUND	512

/*
 * Timing interval for driver's timeout routine
 */
#define LP_MAXTIME	2		/* 2 sec */
#define LP_LONGTIME	0x7fffffffL	/* many ticks */

#define LP_TIMERON	0x01
#define LP_TIMEROFF	0x02

/*
 * Possible parallel port device states
 */
#define LP_OPEN		0x01	/* parallel port device is opened */
#define LP_PRESENT	0x10    /* parallel port device is present */

/*
 * Maximum number of parallel ports allowed on a system
 */
#define LP_MAX_PORTS	4


#define LP_OFF		0
#define LP_ON		1


#endif	/* _KERNEL */


/*
 * The parallel port status register bit definitions are as follows:
 *
 *   bit 7 = 1  Device is not busy, ready for data
 *       6 = 1  Normally 1; pulsed to 0 by device for ACK handshake
 *       5 = 0  Paper present; 1 = paper out
 *       4 = 1  Device selected (on-line)
 *       3 = 1  No error present; 0 = device error
 *       2 = x  pending irq (extended mode only)
 *       1 = x  Don't care
 *       0 = x  Don't care
 */
#define LP_NOTBUSY	0x80
#define LP_ACK		0x40
#define LP_NOPAPER	0x20
#define LP_ONLINE	0x10
#define LP_DEVICEOK	0x08
#define LP_OK		(LP_NOTBUSY | LP_ACK | LP_ONLINE | LP_DEVICEOK)
#define LP_STATUSMASK	(LP_OK | LP_NOPAPER)

/*
 * Macros for logical comparisons
 */
#define LP_IS_READY(s)		(((s) & LP_STATUSMASK) == LP_OK)
#define LP_IS_ACK(s)		(((s) & LP_ACK) == 0)
#define LP_IS_BUSY(s)		(((s) & LP_NOTBUSY) == 0)
#define LP_IS_NOPAPER(s)	(((s) & LP_NOPAPER) != 0)
#define LP_IS_OFFLINE(s)	(((s) & LP_ONLINE) == 0)
#define LP_IS_DEVERROR(s)	(((s) & LP_DEVICEOK) == 0)


#if defined(_KERNEL)

/*
 * Macro definitions for timeout of error message retry
 */
#define LP_BUFCALL		0x01	/* Timeout is from a bufcall() */
#define LP_TIMEOUT		0x02	/* Timeout is from a timeout() */

#define LP_TIMETICK		(drv_usectohz(250000))	/* 250 msec */


/*
 * Structure to represent a single parallel port device.
 */
struct lpdev {
	unsigned int	lp_flag;	/* per-device state flag bits */
	unsigned int	lp_datareg;	/* HW data port address */
	unsigned int	lp_statreg;	/* HW status port address */
	unsigned int	lp_cntlreg;	/* HW control port address */
	unsigned long	lp_porttype;	/* port type */
	unsigned long	lp_portnum;	/* port ID number (0-based) */
	int		lp_portcntl;	/* whether port is acquired or not */
	int		lp_portreq;	/* if port has been requested or not */
	int 		lp_count; 	/* #chars printed after acquiring port*/
	int		lp_wantintr;	/* if expecting interrupts or not */
	int		lp_timer;	/* timeout for error msg allocation */
	unsigned char	lp_timerstat;	/* Status of timeout for err msgs */
	unsigned char	lp_rderr;	/* read-side error code */
	unsigned char	lp_wrerr;	/* write-side error code */
	unsigned char	lp_curstat;	/* HW status reg. contents (errors) */
	unsigned char	lp_errchk;	/* driver error-checking mode */
	unsigned char	lp_errflush;	/* flush handling during dev. error */
	unsigned char	lp_pad0;
	unsigned char	lp_pad1;
	queue_t		*lp_errq;	/* STREAMS queue for sending err msg */
#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
	sv_t		*lp_sleep;
	lock_t		*lp_lock;
#endif
	time_t		lp_ltime;	/* watchdog timeout */
	struct strtty	lp_tty;		/* tty struct for device */
	struct mfpd_rqst lp_req;	/* mfpd request structure */
};


#endif	/* _KERNEL */



/*
 * Structure for reporting error conditions to user
 */
struct lperrmsg {
	unsigned char	lp_cmd;		/* message id code */
	unsigned char	lp_rderr;	/* read-side error code */
	unsigned char	lp_wrerr;	/* write-side error code */
	unsigned char	lp_hwstat;	/* HW status register contents */
};


/*
 * Driver command codes
 */
#define LP_ERRCHK_ON		0x01	/* enable driver's error checking */
#define LP_ERRCHK_OFF		0x02	/* disable driver's error checking */
#define LP_ERRFLUSH_ON		0x03	/* enable driver flush handling */
#define LP_ERRFLUSH_OFF		0x04	/* disable driver flush handling */
#define LP_ERRINFO		0x05	/* error info code */

/*
 * Driver-reported lp error conditions
 */
#define LP_NOERROR		0x00	/* No error detected */
#define LP_PAPEROUT		0x01	/* Printer reports on paper */
#define LP_OFFLINE		0x02	/* Printer indicates it's off-line */
#define LP_DEVERROR		0x03	/* Printer indicates a general error */



#if defined(__cplusplus)
}
#endif

#endif	/* _IO_LP_LP_H */

