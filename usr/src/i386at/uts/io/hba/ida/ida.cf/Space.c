#ident	"@(#)kern-i386at:io/hba/ida/ida.cf/Space.c	1.12.3.3"
/*
    Compaq IDA and SMART hard drive array controllers
 */
#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include "config.h"

int	ida_gtol[ SDI_MAX_HBAS ];		/* global to local */
int	ida_ltog[ SDI_MAX_HBAS ];		/* local to global */

int ida_max_hbas = SDI_MAX_HBAS;

#ifndef IDA_CNTLS
#define IDA_CNTLS 1
#endif /* #ifndef IDA_CNTLS */

#ifdef ONEJOB
int	ida_lowat = 1;	/* LU queue low water mark	*/
int	ida_hiwat = 1;	/* LU queue high water mark	*/
#else /* #ifdef ONEJOB */

#define	LOWATER_JOBS		60	/* low water mark for jobs */
#define	HIWATER_JOBS		64	/* high water mark for jobs */

int	ida_lowat = LOWATER_JOBS;	/* LU queue low water mark */
int	ida_hiwat = HIWATER_JOBS;	/* LU queue high water mark */
#endif /* #ifdef ONEJOB */

struct	ver_no    ida_sdi_ver;

#ifdef IDA_CPUBIND

#define	IDA_VER	PDI_UNIXWARE20 | HBA_IDATA_EXT

struct	hba_ext_info	ida_extinfo = {
	0, IDA_CPUBIND
};

#else /* #ifdef IDA_CPUBIND */

#define	IDA_VER	PDI_UNIXWARE20

#endif /* #ifdef IDA_CPUBIND */

struct	hba_idata_v4	_idaidata[]	= {
	{ IDA_VER, "(ida,1) Compaq SMART", 31, 0, -1, 0, -1, 0 }
#ifdef IDA_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&ida_extinfo }
#endif /* #ifdef IDA_CPUBIND */
};

int	ida_cntls	= IDA_CNTLS;

/* end of file Space.c */
