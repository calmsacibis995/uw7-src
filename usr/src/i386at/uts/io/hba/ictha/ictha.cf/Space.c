#ident	"@(#)kern-i386at:io/hba/ictha/ictha.cf/Space.c	1.9.1.1"
#ident	"$Header$"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/st01.h>
#include <sys/ictha.h>
#include "config.h"

ulong	ictha_rdwr_duration 	 = 60*1;   /* 2 minute */
ulong	ictha_cmds_duration 	 = 60*1;   /* 2 minute */
ulong	ictha_retention_duration = 60*15;  /* 15 minutes */
ulong	ictha_rewind_duration 	 = 60*15;  /* 15 minutes */
ulong	ictha_erase_duration 	 = 60*15;  /* 15 minutes */
ulong	ictha_space_duration 	 = 60*15;  /* 15 minutes */

long	ictha_blocksize = ICTHA_BLOCKSIZE;

ulong	ictha_init_wait_limit = ICTHA_INIT_WAIT_LIMIT;
ulong	ictha_wait_poll_limit = ICTHA_WAIT_POLL_LIMIT;
ulong	ictha_wr_maxout       = ICTHA_WR_MAXTOUT;
ulong	ictha_waitcnt	      = 250;

#ifdef	ICTHA_CPUBIND

#define	ICTHA_VER	HBA_UW21_IDATA | HBA_IDATA_EXT

struct	hba_ext_info	ictha_extinfo = {
	0, ICTHA_CPUBIND
};

#else

#define	ICTHA_VER	HBA_UW21_IDATA

#endif	/* ICTHA_CPUBIND */

struct	hba_idata_v5	_icthaidata[]	= {
	{ ICTHA_VER, "(ictha,1) QICTAPE",
	  7, 768, 1, 5, 1, 0, 0 }
#ifdef	ICTHA_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&ictha_extinfo }
#endif	/* ICTHA_CPUBIND */
};

int	ictha_cntls	= 1;

