#ident	"@(#)kern-i386at:io/hba/zl5380/zl5380.cf/Space.c	1.1.1.2"


#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include "config.h"

int		zl5380_gtol[8];	/* xlate global hba# to local */
int		zl5380_ltog[8];	/* xlate local hba# to global */

struct ver_no	zl5380_sdi_ver;

struct	hba_idata_v5	_zl5380idata[]	= {
	{ HBA_UW21_IDATA, "ZL5380 SCSI HBA",7,0x388,-1,10,-1,0,0 }
};
 
unsigned long zl5380_wait_count = 1000000;
unsigned int  zl5380_retry_count = 30000;
unsigned int  zl5380_arb_retry_count = 10;
unsigned long zl5380_reset_wait_count = 5000000;
unsigned long zl5380_reset_delay_count1 = 50;
unsigned long zl5380_reset_delay_count2 = 100;

