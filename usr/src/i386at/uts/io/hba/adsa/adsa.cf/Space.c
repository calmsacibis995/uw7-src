#ident	"@(#)kern-i386at:io/hba/adsa/adsa.cf/Space.c	1.14.1.2"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include "config.h"
#include <sys/adsa.h>

int		adsa_gtol[SDI_MAX_HBAS];	/* global-to-local hba# mapping */
int		adsa_ltog[SDI_MAX_HBAS];	/* local-to-global hba# mapping */

struct	ver_no    adsa_sdi_ver;

#ifndef	ADSA_CNTLS
#define ADSA_CNTLS 1
#endif

#ifdef	ADSA_CPUBIND

#define	ADSA_VER	HBA_UW21_IDATA | HBA_IDATA_EXT

struct	hba_ext_info	adsa_extinfo = {
	0, ADSA_CPUBIND
};

#else

#define	ADSA_VER	HBA_UW21_IDATA

#endif	/* ADSA_CPUBIND */

struct	hba_idata_v5	_adsaidata[]	= {
	{ ADSA_VER, "(adsa,1) Adaptec AIC-7770 EISA SCSI", 7, 0, -1, 0, -1, 0 }
#ifdef	ADSA_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&adsa_extinfo }
#endif
};

int adsa_cntls = ADSA_CNTLS;

int	adsa_io_per_tar = 2;	/* Maximum number of I/O to be allowed in
				   the adapter queue per target. This value
				   should be kept at 2 or 1.  If is is not set
				   to either a 1 or 2, the driver will adjust
				   it to 2.
				   Default = 2  */

int	adsa_slots = ADSA_MAX_SLOTS;	/* this number represents the maximum number
				   of EISA slots that will be searched to see
				   if any 7770's are in the system.  They will
				   be utilitzed by the driver in the order they
				   are found.  The search always starts at slot
				   1 and stops at this number.
				   NOTE:  Motherboard implementations may
				   use 15 as the slot number for the chip.
				   Default = 15  */

int	adsa_sg_enable = 0;	/* If this is set to 0, no local scatter/gather
				   will be done in the driver, however, there
				   will be scatter/gather done by the kernel.
				   If this is set to 1, local scatter/gather
				   will be done in the driver, and there will
				   be scatter/gather done by the kernel.
 				   Default = 0  */

/*****************************************************************************
 *  NOTE:  The following variables control many functions of the adapter, but *
 * 	the variables will not be used unless the adsa_reinit variable is set*
 *	or the adapter reports it needs to be initialized.  A message from   *
 *	the driver will report whether the following variables will be  *
 *	used or not.							     *
 *****************************************************************************/

char	adsa_reinit = 0;	/* By default, the BIOS initializes the adapter
				   but if the BIOS is disabled, the adapter will
				   require a full initialization.
				   Setting this variable to a 1 will force
				   re-initialization of the AIC-7770 chip/
				   adapter.  This allows the driver to load the
				   adapter/chip with it's program during init
				   time.  By default the BIOS loads the adapter
				   /chip with it's program (sequencer code).
 				   Default = 0   */

/*
 * These control variables are used to tell the driver whether or not to use
 * disconnect/reconnect, wide or synchronous negotiation on a per device
 * basis.  It is a binary level variable where bit 0 = target ID 0 and so on;
 *	 bit		target ID
 *	 0x1 -  0	0
 *	 0x2 -	1	1
 *	 0x4 -  2	2
 *	 0x8 -  3	3
 *	 0x10 - 4	4
 *	 0x20 - 5 	5
 *	 0x40 - 6	6
 *	 0x80 - 7	7	this is the adapter ID and has no effect on
 *				disconnect/reconnect, wide or synchronous
 *				negotiation but is used to tell the driver
 *				that this variable is to be used (valid).
 *				Otherwise, the default configuration setup done
 *				with the EISA config utility will be used.
 *
 * To enable disconnect/reconnect, wide or synchronous negotiation for various
 * SCSI devices, simply add the values together for the various target ID's you
 * wish disconnect/reconnect, wide or synchronous negotiation to be used on.
 * For instance, if you want target ID's 2 and 3 to disconnect/reconnect or you
 * want the adapter to negotiate for wide or synchronous transfers you would
 * make the value 0x4 (TID 2) + 0x8 (TID 3) = 0x0c.
 * To let the driver know you want this value to be used you need to add 0x80
 * to the value, so to properly translate it so the driver will actually use
 * use it you need to do:
 *
 *	(TID 2)  + (TID 3)  + (0x80)   = 0x8c
 *	0x4	 + 0x8	    + (0x80)   = 0x8c
 *	00000100 + 00001000 + 10000000 = 10001100
 *
 * By default, all targets are allowed to disconnect/reconnect and synchronous
 * negotiation will be done to all targets.  By default wide negotiation is not
 * done.  This requires a wide SCSI device to implement.
 *  However, it is suggested you turn off disconnect/reconnect if you only have
 * 1 hard disk attached to the adapter.  This will help improve performance.
 *
 * Changing these variables only alters the disconnect/reconnect, wide or
 * synchronous negotiation while running USL UNIX.  It does not change the
 * original EISA configuration information you selected.
 *
 * NOTE:  The adsa_wideneg control variable tells the adapter which devices
 *	are WIDE SCSI devices (16 bit).  Setting this variable assumes you
 *	have already used the EISA config program to tell the adapter to use
 *	WIDE negotiation and that the adapter is not configured for dual
 *	channel operation.  Setting this control variable and not setting the
 *	EISA configuration could cause unpredictable results.
 *
 *	The variables adsa_rediscntl and adsa_syncneg are 2 byte(int) arrays
 *	where the first byte(int) is for channel 0(A) and the second byte(int)
 *	is for channel 1(B).
 */
int adsa_wideneg = 0;			/* Default = 0			*/
int adsa_rediscntl[] = {0xff, 0xff};	/* Default = 0xff, 0xff		*/
int adsa_syncneg[] = {0xff, 0xff};	/* Default = 0xff, 0xff		*/

/*
 * This array controls the rate that will be negotiated for if synchronous
 * negotiation is enabled for the given target.  The default is 5MB/sec as
 * some older SCSI-1 devices cannot handle the negotiation.  If you are using
 * a SCSI-2 device, which support FAST SCSI transfer (10MB/sec), you will want
 * to alter this array.  The array coincides with the SCSI bus target ID.
 * Legal values for this array are 36, 40, 44, 50, 57, 67, 80, 100, which stands
 * for 3.6, 4.0, 4.4, 5.0, 5.7, 6.7, 8.0, 10.0 mega-bytes per second for the
 * SCSI bus.
 * Any other values will cause the driver to default to 5.0MB/sec.
 */

unsigned char adsa_sync_rate[] = {	/* Default = 50 for all targets */
50, 50, 50, 50, 50, 50, 50, 50,
};

/*
 * This variable controls the data FIFO in the AIC-7770.  The legal values
 * are as follows:
 * 3 =	100%
 * 2 =	 75%
 * 1 =	 50%
 * 0 =	  0%
 *
 * Here are the meanings and effect this variable can have:
 * 100% - Host transfers much faster than SCSI transfers.
 *	The AIC-7770 will start transferring data from the FIFO to host
 *	memory as soon as it detects the FIFO is full for READ commands.
 *	For WRITE commands, the data will be transferred into the FIFO as soon
 *	as the FIFO is empty.
 * 75%  - Host transfers faster than SCSI transfers
 *	The AIC-7770 will start transferring data from the FIFO to host
 *	memory as soon as it detects the FIFO is 75% full for READ commands.
 *	For WRITE commands, the data will be transferred into the FIFO as soon
 *	as the FIFO is 75% empty.
 * 50%  - Host and SCSI transfer rates are nearly equal
 *	The AIC-7770 will start transferring data from the FIFO to host
 *	memory as soon as it detects the FIFO is 50% full for READ commands.
 *	For WRITE commands, the data will be transferred into the FIFO as soon
 *	as the FIFO is 50% empty.
 * 0%   - Host transfer rates are slower than SCSI
 *	The AIC-7770 will start transferring data from the FIFO to host
 *	memory as soon as data is available for READ commands.
 *	For WRITE commands, the data will be transferred into the FIFO as soon
 *	as there is room in the FIFO.
 */
unsigned char	adsa_threshold = 1;			/* Default = 1		*/

/*
 * This variable controls the amount of time (EISA BCLKS) the adapter will
 * continue to transfer data after being pre-empted.  The minimum value is
 * 2 and the maximum value is 60 (decimal).
 */

unsigned char	adsa_bus_release = 60;		/* Default = 60		*/

/*
 * The AIC-7770 (Rev E/AHA-274xA family) has the ability to run in two
 * different modes.
 * The first mode is fully compatible with the Rev C AHA-274x family of
 * adapters where there can only be 4 fully active SCSI commands running at
 * any one time.  This is mode 1
 * The second mode allows the Rev E (AHA-274xA family) to run up to 255
 * commands at a time.  This is mode 2
 * If you have a Rev E type part (AHA-274xA), you may run either mode.
 * If you do not know what revision of the adapter you have, the code
 * will figure it out and set the correct mode.
 * This variable is only provided for those who want to run the Rev E
 * adapter in the Rev C mode.
 */

char  adsa_access_mode = 2;		/* Default = 2	*/
