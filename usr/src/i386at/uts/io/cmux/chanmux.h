#ifndef	_IO_CMUX_CHANMUX_H	/* wrapper symbol for kernel use */
#define	_IO_CMUX_CHANMUX_H	/* subject to change without notice */

#ident	"@(#)chanmux.h	1.7"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/stream.h>		/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/stream.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */


#define	CMUX_NUMSWTCH		10	/* keep history of last 10 switches */
#define	CMUXPSZ			32	/* maximun output packet size */


/*
 * The cmux_t structure is the per-VT data structure. It is pointed
 * by the q_ptr field of the queue pair for the VT. It points back
 * to the queues, and also to the global workstation structures.
 */
typedef struct cmux_stat {
	unsigned long	cmux_num;	/* virtual terminal number */
	unsigned long	cmux_flg;	/* internal state flag */
	queue_t		*cmux_rqp;	/* saved pointer to read queue */
	queue_t		*cmux_wqp;	/* saved pointer to write queue */
	struct ws_stat	*cmux_wsp;	/* back pointer to the workstation */
	dev_t		cmux_dev;	/* device number for this channel */
} cmux_t;

/*
 * Internal state bits of cmux_t (cmux_flg).
 */
#define	CMUX_OPEN		0x1
#define	CMUX_CLOSE		0x2
#define	CMUX_WCLOSE		0x4


struct cmux_swtch {
	clock_t		sw_time;
	unsigned long	sw_chan;	/* channel number made active */
};


/*
 * The cmux_link_t structure is associated with each lower stream 
 * linked underneath the multiplexor. The cmlb_iocmsg member is used
 * to save the response by the lower stream to the M_IOCTL message.
 * sent to it. If the response is an M_IOCNAK message, then cmlb_iocresp
 * is set to M_IOCNAK. Otherwise it is set to M_IOCACK (because it is
 * a positive response to the M_IOCTL). cmlb_flg is an "in-use" flag
 * that is zero if the structure is not associated with a lower stream.
 * cmlb_err is used to hold an error code sent upstream  by the lower
 * stream upon an open. cmlb_lblk is a saved copy of the linkblk
 * structure argument to I_LINK/I_PLINK ioclts. It contains the
 * multiplexor information about the lower stream and a pointer to
 * its read queue. This structure is used when routing messages from
 * the uppers streams to the lower streams.
 */ 
typedef struct cmux_linkblk {
	unsigned long	cmlb_iocresp;	/* ACK or NACK? */
	unsigned long	cmlb_flg;	/* in use? */
	mblk_t		*cmlb_iocmsg;	/* copy of M_IOCTL message */
	unsigned long	cmlb_err;	/* open error code */
	struct linkblk	cmlb_lblk;	/* upper-to-lower stream link */
} cmux_link_t;


/*
 * The cmux_ws_t is the state structure for the workstation. CHANMUX
 * (wsbase) maintains a list of pointers to these (each structure
 * is dynamically allocated as needed). w_ioctlchan is the number of
 * the VT doing ioctl processing at any instant, w_ioctllstrm is the
 * number of the lower stream processing the ioctl (0 for the principal
 * stream or the secondary stream number + 1), and w_ioctlcnt is the
 * number of lower streams which have been sent a copy of the M_IOCTL
 * message (pointed to by w_iocmsg) but have yet to reply. w_state
 * indicates if an ioctl is in process or not. w_cmuxpp is a pointer
 * to a list of cmux_t pointers that is w_numchan elements long. w_princp
 * and w_lstrmsp are, respectively, pointers to lists of cmux_link_t
 * pointers. The number of elements in w_princp is given by w_numchan
 * while the number for w_lstrmsp is given by w_numlstrms. w_lstrms is
 * the actual number of seconday streams linked underneath the work-
 * station. The timestamps of the last CMUX_NUMSWTCH channel switches
 * are saved in w_swtchtimes, with w_numswitch being the number of
 * channel switch structures that contain valid information.
 */ 
typedef struct ws_stat {
	unsigned long	w_ioctlchan;	/* channel number doing ioctl */
	unsigned long	w_ioctllstrm;	/* lower stream no. processing ioctl */
	unsigned long	w_ioctlcnt;	/* no. of lower streams sent an ioctl */
	mblk_t		*w_iocmsg;	/* M_IOCTL message block */
	unsigned long	w_state;	/* M_IOCTL state in progress? */ 
	cmux_t		**w_cmuxpp;	/* list of cmux_t pointers */
	unsigned long	w_numchan;	/* no. of channels allocated */
	cmux_link_t	*w_princp;	/* link to list of principal streams */
	cmux_link_t	*w_lstrmsp;	/* link to list of secondary streams */
	unsigned long	w_numlstrms;	/* no. of lower streams linked */
	unsigned long	w_lstrms;	/* actual no. of secondary streams linked */
	unsigned long	w_numswitch;	/* no. of valid switches */
	struct cmux_swtch
			w_swtchtimes[CMUX_NUMSWTCH];
} cmux_ws_t;

/*
 * Internal state of cmux_ws_t (w_state).
 */
#define	CMUX_IOCTL		0x8


/*
 * The cmux_lstrm_t structure is associated with the read and write
 * queues for the lower stream (theri q_ptr fields point to it).
 * It contains a back pointer to the workstation state structure,
 * a flag that indicates if the lower stream is a principal stream,
 * (i.e. KD) or secondary (i.e. mouse) stream. The CMUX_PRINSLEEP
 * flag is set when the wakeup should be done on a sleeping open
 * for the VT. The lstrm_id field is the channel/VT number if 
 * CMUX_PRINCSTRM is set in the flag, otherwise it translates to
 * the number of the secondary stream in the array of secondary
 * stream cmux_link_t structures in the workstation.
 */ 
typedef struct cmux_lstrm {
	cmux_ws_t	*lstrm_wsp;	/* pointer to workstation */
	unsigned long	lstrm_flg;	/* stream type -- principal/secondary */
	unsigned long	lstrm_id;	/* channel number of principal stream */
} cmux_lstrm_t;

/*
 * cmux_lstrm_t (lstrm_flg) and cmux_t (cmlb_flg) flag bits.
 */
#define	CMUX_SECSTRM		0x1
#define	CMUX_PRINCSTRM		0x2
#define	CMUX_PRINCSLEEP		0x4


#define	MAXCMUXPSZ		1024
#define	CMUX_STRMALLOC		2
#define	CMUX_CHANALLOC		8
#define CMUX_WSALLOC		4


#if defined(__cplusplus)
	}
#endif

#endif /* _IO_CMUX_CHANMUX_H */
