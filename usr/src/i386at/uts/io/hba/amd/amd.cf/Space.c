#ident	"@(#)kern-i386at:io/hba/amd/amd.cf/Space.c	1.1.1.1"
#ident	"$Header$"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/conf.h>
#include "config.h"

int	 amd_gtol[SDI_MAX_HBAS];	/* map global hba # to local # */
int	 amd_ltog[SDI_MAX_HBAS];	/* map local hba # to global # */

int amd_hba_max = 64;	/* Maximum jobs concurrently on HBA 
			 * The hardware supports up to 64 concurrent jobs
			 */

struct	ver_no    amd_sdi_ver;

int	amd_ctlr_id = 7;	/* HBA's SCSI id number		*/

#ifdef	AMD_CPUBIND

#define	AMD_VER		HBA_UW21_IDATA | HBA_IDATA_EXT

struct	hba_ext_info	amd_extinfo = {
	0, AMD_CPUBIND
};

#else

#define	AMD_VER		HBA_UW21_IDATA

#endif

/*
 * Change the name of the original idata array in space.c.
 * We do this so we don't have to change all references
 * to the original name in the driver code.
 */

struct	hba_idata_v5	_amdidata[]	= {
	{ AMD_VER, "(amd,1) AMD SCSI",
	  7, 0, -1, 0, -1, 0 }
#ifdef	AMD_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&amd_extinfo }
#endif
};

int	amd_cntls	= 1;
