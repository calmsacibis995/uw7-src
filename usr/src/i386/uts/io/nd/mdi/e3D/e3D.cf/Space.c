#ident "@(#)Space.c	26.1"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

/*
 *	System V STREAMS TCP - Release 2.0
 *
 *	Copyright 1987, 1988 Lachman Associates, Incorporated (LAI)
 *
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */

/*
 *	(C) Copyright 1991 SCO Canada, Inc.
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	SCO Canada, Inc.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by SCO Canada, Inc.
 */

#include "sys/types.h"
#include "sys/stream.h"
#include "sys/mdi.h"

/* MAC address override now exists as MACADDR .bcfg custom parameter */

/* Debug control */
unsigned long e3Ddbgcntrl = 0x0;
