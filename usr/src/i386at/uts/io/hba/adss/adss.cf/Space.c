#ident	"@(#)kern-i386at:io/hba/adss/adss.cf/Space.c	1.11.1.3"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include "config.h"

int		adss_gtol[SDI_MAX_HBAS]; /* xlate global hba# to loc */
int		adss_ltog[SDI_MAX_HBAS]; /* local hba# to global     */

struct	ver_no    adss_sdi_ver;

#ifndef ADSS_CNTLS
#define ADSS_CNTLS 1
#endif

#ifdef	ADSS_CPUBIND

#define	ADSS_VER	HBA_UW21_IDATA | HBA_IDATA_EXT

struct	hba_ext_info	adss_extinfo = {
	0, ADSS_CPUBIND
};

#else

#define	ADSS_VER	HBA_UW21_IDATA

#endif	/* ADSS_CPUBIND */

struct	hba_idata_v5	_adssidata[]	= {
	{ ADSS_VER, "(adss,1) Adaptec AIC-6X60 SCSI",  7, 0, -1, 0, -1, 0 }
#ifdef	ADSS_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&adss_extinfo }
#endif
};

int adss_cntls = ADSS_CNTLS;

int	adss_io_per_tar = 2;	/* Maximum number of I/O to be allowed in
				   the adapter queue per target. This value
				   should be kept at 2 or 1.  If is is not set
				   to either a 1 or 2, the driver will adjust
				   it to 2.
				   Default = 2  */

char	adss_sg_enable = 0;	/* If this is set to 0, no local scatter/gather
				   will be done in the driver, however, there
				   will be scatter/gather done by the kernel.
				   If this is set to 1, local scatter/gather
				   will be done in the driver, and there will
				   be scatter/gather done by the kernel.
 				   Default = 0  */

char	adss_disconnect = 1;	/* If this is set to 0, no disconnects will
				   be allowed.
				   Default = 1  */

char	adss_timeout_enable = 0; /*If this is set to a 1, the driver will use
				   the following variable to set a timeout for
				   each SCSI command started.
				   Default = 0 (disabled) */

int	adss_timeout_count = 30; /*The default for SCSI command timeouts are
				   30 seconds.  NOTE:  If you have a tape drive
				   attached, you should set this to whatever
				   time it takes to do a complete tape retension
				   or the command will timeout.
				   Default = 30 seconds */
