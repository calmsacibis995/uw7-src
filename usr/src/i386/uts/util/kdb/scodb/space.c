#ident	"@(#)kern-i386:util/kdb/scodb/space.c	1.1"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

/*
 * Only for space.c definitions
 */

#include	"histedit.h"
#include	"sent.h"
#include	"stunv.h"
#include	"dbg.h"
#include	"alias.h"
#include	"val.h"
#include	"bkp.h"

#define MAXACPUS	10
#define SCODB_LINETB	1
#define SCODB_HISTORY	32
#define SCODB_VARS	16
#define SCODB_DECL	16
#define SCODB_ALIAS	8
#define SCODB_BKP	10

int scodb_ssregs = 0;

struct bkp scodb_bkp[SCODB_BKP];
int scodb_mxnbp = SCODB_BKP;

#define NBPCMDB (SCODB_BKP*5)

struct ilin      scodb_bibufs[NBPCMDB];
int              scodb_nbibufs           = NBPCMDB;

char	*db_stuntable = NULL;
int	db_stuntablesize = 0;

char	*db_varitable = NULL;
int	db_varitablesize = 0;

char	scodb_stk[512];			/* required for "" string eval */

/*
 *	History buffers
 */

struct ilin	 scodb_ibufs[SCODB_HISTORY];
int		 scodb_maxhist		= SCODB_HISTORY;
struct scodb_list	 scodb_history		= { LF_CANERR|LF_WRAPS, 0 };
struct ilin	*scodb_ibufp[SCODB_HISTORY];

/*
 *	Debugger variables
 */

struct value	 scodb_var[SCODB_VARS];
int		 scodb_nvar		= SCODB_VARS;

int		 scodb_nalias		= SCODB_ALIAS;
int		 scodb_ndcvari		= SCODB_DECL;
struct cvari	 scodb_dcvari[SCODB_DECL];
char		 scodb_dcvarinames[SCODB_VARS * NAMEL];

struct alias	 scodb_alias[SCODB_ALIAS];

struct dbcmd	 scodb_ocmds[1];
int		 scodb_nocmds		= 0;

/*
 * Line number table
 */

char		db_linetable[SCODB_LINETB] = {0};
int		db_linetablesize	   = SCODB_LINETB;

/*
 * SCODB options - see space.h
 */

int		scodb_options		= 0;

