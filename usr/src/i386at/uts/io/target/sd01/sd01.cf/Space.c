#ident	"@(#)kern-i386at:io/target/sd01/sd01.cf/Space.c	1.16.5.1"
#ident	"$Header$"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/elog.h>
#include <sys/vtoc.h>
#include <sys/alttbl.h>
#include <sys/altsctr.h>
#include <sys/fdisk.h>
#include <sys/sd01.h>

struct dev_spec *sd01_dev_spec[] = {
	0
};

struct dev_cfg SD01_dev_cfg[] = {
{	SDI_CLAIM|SDI_ADD, DEV_CFG_UNUSED, DEV_CFG_UNUSED, 
		DEV_CFG_UNUSED, ID_RANDOM, 0, "", DEV_CFG_UNUSED	},
};

int SD01_dev_cfg_size = sizeof(SD01_dev_cfg)/sizeof(struct dev_cfg);

char   Sd01_debug[] = {
	0,0,0,0,0,0,0,0,0,0		/* Debug levels			*/
};

char	Sd01_modname[] = "sd01";	/* driver modname   */

major_t	Sd01_Identity  = (major_t)(('S'<<24)|('D'<<16)|('0'<<8)|('1'));

int	Sd01log_marg = 0;		/* Marginal bad block logging   */
unsigned int Sd01bbh_flg = BBH_RDERR;		/* bad block handling flag	*/
					/* see sd01.h for values	*/

/*
 * Define the logical geometry to be used for the various
 * bytes/sector values.  Array can be indexed by the
 * number of bit-shifts required to convert 512 into
 * the desired bps value.
 * Note: Hex value = <secs/trk> and <# of heads>
 */
short	Sd01diskinfo[] = {
	0x2040,				/* 512 bytes/sector		*/
	0x2020,				/* 1K bps			*/
	0x2010,				/* 2K bps			*/
	0x2008,				/* 4K bps			*/
	0x2004,				/* 8K bps			*/
	0x2002,				/* 16K bps			*/
	0x2001,				/* 32K bps			*/
	0x1001,				/* 64K bps			*/
	0x0801,				/* 128K bps			*/
	0x0401,				/* 256K bps			*/
	0x0201,				/* 512K bps			*/
	0x0101,				/* 1M bps			*/
};

#ifdef RAMD_BOOT
int Sd01_boot_time = 1;
#else
int Sd01_boot_time = 0;
#endif

int	sd01_lightweight = 1;	/* Enable lightweight checks for variables
				 * that would otherwise be synchronized
				 * by a LOCK/UNLOCK.  If a 32-bit aligned
				 * word cannot be read/written simultaneously
				 * whith the read returning either the value
				 * before or after the write, this variable
				 * should be set to 0.
				 */

int sd01_sync=0;

	/* Timeout tunables: Disabled by default
	 *
	 * For HBA's that perform timeouts (indicated to sd01
	 * by setting HBA_TIMEOUT in the devflag) the following
	 * description is unneeded: sd01 timeouts are always
	 * disabled for these controllers and the controllers always 
	 * timeout jobs when appropriate.
	 *
	 * sd01_insane:
	 *
	 * Number of seconds we should give an active disk
	 * to successfully respond to any job.  If the
	 * disk does not complete some job within sd01_insane
	 * seconds, all outstanding jobs are failed.
	 * On some HBA drivers that implement timeouts this 
	 * may cause problems.  If sd01_insane is 0, this
	 * type of timeout will be disabled. 
	 *
	 * sd01_timeout:
	 *
	 * Number of seconds we should give any one job before 
	 * it should be timed out.  Note that sd01_timeout must 
	 * be unusually long because some HBA drivers implement 
	 * unfair scheduling  heuristics that may result in starvation.  
	 * sd01_timeout must be at least 3 * sd01_insane.  If 
	 * sd01_timeout is 0, this type of timeout is not performed.
	 *
	 * WARNING:
	 *
	 * Since HBA drivers are not informed of the timeout,
	 * if a job returns after a disk is thought to be
	 * insane, unpredictable results occur.  For example,
	 * the file system may be dirty.
	 *
	 * Recommended values:
	 *
	 * The following values are recommended to avoid unwanted
	 * timeouts:
	 *
	 *	sd01_timeout: 0 or >= 300
	 *	sd01_insane:  0 or >= 10
	 * 
	 * Example:
	 *
	 * If sd01_timeout = 360 and sd01_insane = 10, a disk
	 * must respond to some active command within 10 seconds,
	 * but does not have to respond to a particular
	 * command until 300 seconds have elapsed.
	 */

int sd01_timeout = 0;		/* Disable single job timeouts */
int sd01_insane =  0;		/* Disable insane disk timeouts */

int sd01_retrycnt = SD01_RETRYCNT;	/* Number of times to retry */
