#ident	"@(#)ihvkit:pdi/mitsumi/uw_1.1.cf/Space.c	1.1"
#ident	"$Header$"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/param.h>
#include "config.h"
#include <sys/mitsumi.h>

int	mitsumi_gtol[ MAX_HAS ];		/* global to local */
int	mitsumi_ltog[ MAX_HAS ];		/* local to global */

int	mitsumi_lowat = LOWATER_JOBS;	/* LU queue low water mark	*/
int	mitsumi_hiwat = HIWATER_JOBS;	/* LU queue high water mark	*/

#ifdef  MITSUMI_CPUBIND
#define MITSUMI_ID_DIM	MITSUMI_CNTLS+1
#define MITSUMI_VER	HBA_SVR4_2MP | HBA_IDATA_EXT
struct	hba_ext_info mitsumi_extinfo = {
	0, MITSUMI_CPUBIND
};
#else
#define MITSUMI_ID_DIM	MITSUMI_CNTLS
#define MITSUMI_VER	1
#endif

struct	hba_idata	mitsumiidata[MITSUMI_ID_DIM] = {
#ifdef	MITSUMI_0
	{ MITSUMI_VER, "(mitsumi,1st) MITSUMI MITSUMI",
	  7, MITSUMI_0_SIOA, MITSUMI_0_CHAN, MITSUMI_0_VECT, MITSUMI_0, 0 }
#endif
#ifdef	MITSUMI_1
	{ MITSUMI_VER, "(mitsumi,2nd) MITSUMI MITSUMI",
	  7, MITSUMI_1_SIOA, MITSUMI_1_CHAN, MITSUMI_1_VECT, MITSUMI_1, 0 }
#endif
#ifdef	MITSUMI_2
	{ MITSUMI_VER, "(mitsumi,3rd) MITSUMI MITSUMI",
	  7, MITSUMI_2_SIOA, MITSUMI_2_CHAN, MITSUMI_2_VECT, MITSUMI_2, 0 }
#endif
#ifdef	MITSUMI_3
	{ MITSUMI_VER, "(mitsumi,4th) MITSUMI MITSUMI",
	  7, MITSUMI_3_SIOA, MITSUMI_3_CHAN, MITSUMI_3_VECT, MITSUMI_3, 0 }
#endif
#ifdef	MITSUMI_4
	{ MITSUMI_VER, "(mitsumi,5th) MITSUMI MITSUMI",
	  7, MITSUMI_4_SIOA, MITSUMI_4_CHAN, MITSUMI_4_VECT, MITSUMI_4, 0 }
#endif
#ifdef	MITSUMI_5
	{ MITSUMI_VER, "(mitsumi,6th) MITSUMI MITSUMI",
	  7, MITSUMI_5_SIOA, MITSUMI_5_CHAN, MITSUMI_5_VECT, MITSUMI_5, 0 }
#endif
#ifdef	MITSUMI_6
	{ MITSUMI_VER, "(mitsumi,7th) MITSUMI MITSUMI",
	  7, MITSUMI_6_SIOA, MITSUMI_6_CHAN, MITSUMI_6_VECT, MITSUMI_6, 0 }
#endif
#ifdef	MITSUMI_7
	{ MITSUMI_VER, "(mitsumi,8th) MITSUMI MITSUMI",
	  7, MITSUMI_7_SIOA, MITSUMI_7_CHAN, MITSUMI_7_VECT, MITSUMI_7, 0 }
#endif
#ifdef  MITSUMI_CPUBIND
	,
	{ HBA_EXT_INFO, (char *) &mitsumi_extinfo }
#endif
};

int	mitsumi_cntls		= MITSUMI_CNTLS;

int 	mitsumi_rdy_timeout 	= 160000;

int	mitsumi_max_idle_time 	= MITSUMI_MAX_IDLE_TIME;   /* 5 minutes */
int	mitsumi_chktime		= MITSUMI_CHKTIME;	  /* 20 seconds */
int	mitsumi_cmd_timeout 	= MITSUMI_CMD_TIMEOUT;  	  /* 10 seconds */
ushort_t mitsumi_max_tries 	= 5;  	  		  /* 5 times */
