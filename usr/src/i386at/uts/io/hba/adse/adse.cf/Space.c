#ident	"@(#)kern-i386at:io/hba/adse/adse.cf/Space.c	1.12.1.2"


#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/adse.h>
#include "config.h"

int	adse_gtol[SDI_MAX_HBAS]; /* xlate global hba# to loc */
int	adse_ltog[SDI_MAX_HBAS]; /* local hba# to global     */

struct	ver_no    adse_sdi_ver;

#ifndef ADSE_CNTLS
#define ADSE_CNTLS 1
#endif

#ifdef	ADSE_CPUBIND

#define	ADSE_VER	HBA_UW21_IDATA | HBA_IDATA_EXT

struct	hba_ext_info	adse_extinfo = {
	0, ADSE_CPUBIND
};

#else

#define	ADSE_VER	HBA_UW21_IDATA

#endif	/* ADSE_CPUBIND */

struct	hba_idata_v5	_adseidata[]	= {
	{ ADSE_VER, "(adse,1) Adaptec AHA-1740 EISA SCSI", 7, 0, -1, 0, -1, 0 }
#ifdef	ADSE_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&adse_extinfo }
#endif
};

int	adse_io_per_tar = 2;	/* Maximum number of I/O to be allowed in
				   the adapter queue per target.  The 1740 has
				   a maximum limit of 64 on board commands
				   allowed to be executing at any given
				   time.  The maximum value is 4. */

int	adse_cntls	= ADSE_CNTLS;
int	adse_slots	= ADSE_MAX_SLOTS;	/* this number represents the maximum number
				   of EISA slots that will be searched to see
				   if any 1740's are in the system.  They will
				   be utilitzed by the driver in the order they
				   are found.  The search always starts at slot
				   1 and stops at this number.  */

/*
 * If this is set to 0, no local scatter/gather will be done in the driver
 * however, there will be scatter/gather done by the kernel.
 * If this is set to 1, local scatter/gather will be done in the driver, and
 * there will be scatter/gather done by the kernel.
 * Default = 0
 */
int	adse_sg_enable = 0;

/*
 * This variable controls the amount of time the EISA adapter will stay
 * on the EISA bus after being pre-empted by another EISA device.  The
 * time is in micro-seconds and valid values are 0, 4, and 8.  This value
 * depends on the motherboard being able to handle the added time after
 * pre-emption occurs, if this value is not 0.
 * Default = 0
 */
int	adse_buson = 0;

/*
 * This variable enables the timeout code in the driver.  For every command
 * started the driver will start a timeout alarm.  If you have several drivers
 * loaded with this type of feature you may need to increase NCALLS in the
 * kernel.
 * Default = 0 (OFF)
 */

int	adse_timeout_enable = 0;

/*
 * The following variable is the amount of time (in seconds) the driver will
 * the timeout to.  It only applies if the adse_timeout_enable variable is
 * enabled (a one).  NOTE:  The timeout function will be called for every SCSI
 * command that the driver starts, so be careful of this length as a tape
 * retension could take several minutes.  If the timeout occurs an ABORT is
 * issued which could cause your tape retension to be cancelled.
 * Default = 30 seconds
 */

int	adse_timeout_count = 30;
