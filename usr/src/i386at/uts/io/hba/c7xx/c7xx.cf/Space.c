#ident	"@(#)kern-i386at:io/hba/c7xx/c7xx.cf/Space.c	1.1.3.1"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/conf.h>
#include "config.h"

/*
 * C7XX debugging flags
 */
unsigned long c7xx_debugging = 0;

int	 	c7xx_gtol[SDI_MAX_HBAS]; /* map global hba # to local # */
int	 	c7xx_ltog[SDI_MAX_HBAS]; /* map local hba # to global # */
int		c7xx_cntls	= 1;
struct		ver_no    c7xx_sdi_ver;

/************************************************************************
 * These are the tunning parameters set using idtune parameters.
 * c7xx_MAX_DLP :
 *      Minimum value = 0             Maximum value = 128
 *      Defines the maximum number of scatter/gather data list 
 *      supported for a request by the device driver.
 * c7xx_QDEPTH :
 *      Minimum value = 0             Maximum value = 128
 *      Defines the number of qtags supported by the device driver.
 * c7xx_WIDE :
 *      Disable WIDE = 0              Enable WIDE = 1
 *      Enables/Disables the Wide device support.
 * c7xx_SORT :
 *      Disable SORT = 0              Enable SORT = 1
 *      Enables/Disables Sorting of scsi reads depending on the
 *      block address.
 * c7xx_MaxBlocks :
 *      Minimum value = 1             Maximum value = 0xffff
 *      Defines the maximum number of blocks to concetenate for
 *      scsi read concatenation mentioned below.
 * c7xx_CONCAT :
 *      Disable Concatenation = 0     Enable Concatenation = 1
 *      Enables/Disables scsi reads concetenation for performance
 *      improvements. If enabled then subsequent block reads are
 *      concatenated into a single read and send to scsi device.
 * c7xx_DHP :
 *      When c7xx_DHP =1, sxfer bit 8 will be asserted.
 *      From the 53c710 data manual:
 *      The 53c710 will not halt the scsi transfer when a parity error
 *      occurs until the end of a block move operation. When set and the
 *      initiator asserts ATN/, the 53c710 will complete the block move
 *      and then, depending on whether or not the ATN/ interrupt is 
 *      enabled, either generate an interrupt or continue fetching 
 *      instructions. 
 * c7xx_ASYNC :
 *      Enable Asyncrhonous = 1     Enable Synchronous = 0
 *      Disable synchronous mode. Force all i/o's to be asynchronous.
 ***********************************************************************/

int	c7xx_MAX_DLP = 128;		/* Number of MAX outstanding reqs */
int	c7xx_QDEPTH = 128;
char	c7xx_WIDE = 0;
char	c7xx_SORT = 0;
ulong	c7xx_MaxBlocks = 0xffff;
char	c7xx_CONCAT = 1;
int     c7xx_DHP = 0;

/*
 * Asyncrhonous is enabled to due a problem w/ syncrhonous negotiations
 * with some SCSI drives.  The driver contains a fix that appears to 
 * correct the problem negotiating synchronous mode.  It was decided to
 * leave asynchronous enabled until testing indicates that the problem 
 * is corrected.
 */
int	c7xx_ASYNC = 1;

#ifdef	C7XX_CPUBIND

#define	C7XX_VER		HBA_UW21_IDATA | HBA_IDATA_EXT

struct	hba_ext_info	c7xx_extinfo = {
	0, C7XX_CPUBIND
};

#else

#define	C7XX_VER		HBA_UW21_IDATA

#endif	/* C7XX_CPUBIND */

/*
 * Change the name of the original idata array in space.c.
 * We do this so we don't have to change all references
 * to the original name in the driver code.
 */

struct	hba_idata_v5	_c7xxidata[]	= {
	{ C7XX_VER, "(c7xx,1) Symbios Logic SCSI 53C7X0",
	  7, 0, -1, 0, -1, 0 }
#ifdef	C7XX_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&c7xx_extinfo }
#endif
};
