#ident	"@(#)kern-i386at:io/hba/c8xx/c8xx.cf/Space.c	1.1.5.1"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/conf.h>
/*#include <config.h>*/

int	 	c8xx_gtol[SDI_MAX_HBAS]; /* map global hba # to local # */
int	 	c8xx_ltog[SDI_MAX_HBAS]; /* map local hba # to global # */
int		c8xx_cntls	= 1;
struct		ver_no    c8xx_sdi_ver;
/************************************************************************
 * These are the tunning parameters set using idtune parameters.
 * c8xx_QDEPTH :
 *      Minimum value = 0             Maximum value = 128
 *      Defines the number of qtags supported by the device driver.
 ***********************************************************************/

int c8xx_QDEPTH = 5;

#ifdef	C8XX_CPUBIND

#define	C8XX_VER		HBA_UW21_IDATA | HBA_IDATA_EXT

struct	hba_ext_info	c8xx_extinfo = {
	0, C8XX_CPUBIND
};

#else

#define	C8XX_VER		HBA_UW21_IDATA

#endif	/* C8XX_CPUBIND */

/*
 * Change the name of the original idata array in space.c.
 * We do this so we don't have to change all references
 * to the original name in the driver code.
 */

struct	hba_idata_v5	_c8xxidata[]	= {
	{ C8XX_VER, "(c8xx,1) Symbios Cxxx 4.02.01",
	  7, 0, -1, 0, -1, 0 }
#ifdef	C8XX_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&c8xx_extinfo }
#endif
};


/*+++HDR
 *          Name:  Space.c 
 *         Title:  
 *     $Workfile: 
 *     $Revision:
 *      $Modtime:
 *    Programmer:
 * Creation Date:
 *
 *  Version History
 *  ---------------
 *
 *    Date    Who?  Description
 *  --------  ----  -------------------------------------------------------
 *
 *-------------------------------------------------------------------------
 *
 *  $Header$
 *  $Log$
 *
 *
---*/

#ifdef OPNSVR5
#include "sys/devreg.h"
#endif

/*DECLARATIONS*/

#ifdef OPNSVR5
#define	MAX_HBA	31
SHAREG_EX drvrreg [MAX_HBA+1];
int drvr_processor = DRIVER_CPU_DEFAULT;
#endif

/* tuneable Parameters */
int drvr_do_tagged = 1;
int drvr_MAX_LUN = 31;
int drvr_InitiatorID = 7;
int drvr_ScanExcludeID = 16;
int drvr_InitializationResetsBus = 1;
int drvr_Debug_Verbose_Level = 0;
