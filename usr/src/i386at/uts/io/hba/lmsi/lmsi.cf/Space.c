#ident	"@(#)kern-i386at:io/hba/lmsi/lmsi.cf/Space.c	1.7.1.1"
#ident	"$Header$"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/param.h>
#include "config.h"
#include <sys/lmsi.h>

int	lmsi_gtol[ MAX_HAS ];		/* global to local */
int	lmsi_ltog[ MAX_HAS ];		/* local to global */

int	lmsi_lowat = LOWATER_JOBS;	/* LU queue low water mark	*/
int	lmsi_hiwat = HIWATER_JOBS;	/* LU queue high water mark	*/

#ifdef  LMSI_CPUBIND
#define LMSI_VER	HBA_UW21_IDATA | HBA_IDATA_EXT
struct	hba_ext_info lmsi_extinfo = {
	0, LMSI_CPUBIND
};
#else
#define LMSI_VER	HBA_UW21_IDATA
#endif

struct	hba_idata_v5	_lmsiidata[] = {
	{ LMSI_VER, "(lmsi,1) Philips",
	  7, 0, -1, 0, -1, 0 }
#ifdef  LMSI_CPUBIND
	,{ HBA_EXT_INFO, (char *) &lmsi_extinfo }
#endif
};

int	lmsi_cntls		= 1;

int 	lmsi_rdy_timeout 	= 60000;

int	lmsi_max_idle_time 	= LMSI_MAX_IDLE_TIME;   /* 5 minutes */
int	lmsi_chktime		= LMSI_CHKTIME;
int	lmsi_cmd_timeout 	= LMSI_CMD_TIMEOUT; 
ushort_t lmsi_max_tries 	= 2;  	  		  /* 5 times */
