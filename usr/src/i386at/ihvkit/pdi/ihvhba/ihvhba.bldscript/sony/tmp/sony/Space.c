/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ihvkit:pdi/ihvhba/ihvhba.bldscript/sony/tmp/sony/Space.c	1.2"
#ident	"$Header$"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/param.h>
#include "config.h"
#include <sys/sony.h>


extern struct sonyvector sony31avector;
extern struct sonyvector sony535vector;

struct sonyvector* sonyvectorlist[3] = {
	&sony535vector,
	&sony31avector,
	NULL
};

int	sony_gtol[ MAX_HAS ];		/* global to local */
int	sony_ltog[ MAX_HAS ];		/* local to global */

int	sony_lowat = LOWATER_JOBS;	/* LU queue low water mark	*/
int	sony_hiwat = HIWATER_JOBS;	/* LU queue high water mark	*/

#ifdef  SONY_CPUBIND
#define SONY_VER	HBA_SVR4_2MP | HBA_IDATA_EXT
struct	hba_ext_info sony_extinfo = {
	0, SONY_CPUBIND
};
#else
#define SONY_VER	1
#endif

struct	hba_idata_v4	_sonyidata[] = {
	{ SONY_VER, "(sony,1) SONY",
	  7, 0, -1, 0, -1, 0 },
#ifdef  SONY_CPUBIND
	{ HBA_EXT_INFO, (char *) &sony_extinfo }
#endif
};

int	sony_cntls		= 1;

int	sony31a_max_idle_time 	= SONY31A_MAX_IDLE_TIME;   /* 5 minutes */
int	sony31a_chktime		= SONY31A_CHKTIME;	  /* 2 seconds */
int	sony31a_data_timeout 	= SONY31A_DATA_TIMEOUT;	  /* 1 seconds */
int	sony31a_control_timeout	= SONY31A_CONTROL_TIMEOUT; /* 13 seconds */
ushort_t sony31a_max_tries 	= SONY31A_MAX_TRIES;	  /* 7 times */

int sony31a_cmdretry = 200;  /* in 10 us units */
int sony31a_resultretry = 800000;
int sony31a_dataretry = 200000;
int sony31a_spininc = 4; /* to do more or less spinup retries */


int     sony535_max_idle_time   = SONY535_MAX_IDLE_TIME;   /* 16 seconds */
int     sony535_readchktime     = SONY535_READCHKTIME;    /* 1 tick */
int     sony535_controlchktime  = SONY535_CONTROLCHKTIME;  /* 5 second */
int     sony535_control_timeout = SONY535_CONTROL_TIMEOUT; /* 13 seconds */
int	sony535_data_timeout 	= SONY535_DATA_TIMEOUT;	  /* 1 seconds */
ushort_t sony535_max_tries      = SONY535_MAX_TRIES;      /* 7 times */

int sony535_dataretry = 200000;
int sony535_spinwait = 9; /* 9 seconds */
int sony535_resultretry = 1000000;
int sony535_dmaon = 1;  /* force 535 DMA on. Since default to off for apricot */
int sony31a_dmaoff = 1; /* DMA doesn't work with 31/33a ? */

