#ident	"@(#)pcintf:pkg_lockset/lsetunuse.c	1.1.1.2"
#include <sccs.h>

SCCSID(@(#)lsetunuse.c	3.1);	/* 10/15/90 15:39:43 */

/*
   (c) Copyright 1985 by Locus Computing Corporation.  ALL RIGHTS RESERVED.

   This material contains valuable proprietary products and trade secrets
   of Locus Computing Corporation, embodying substantial creative efforts
   and confidential information, ideas and expressions.  No part of this
   material may be used, reproduced or transmitted in any form or by any
   means, electronic, mechanical, or otherwise, including photocopying or
   recording, or in connection with any information storage or retrieval
   system without permission in writing from Locus Computing Corporation.
*/

/*---------------------------------------------------------------------------
 *
 *	lsetunuse.c - 'unuse' a lockset (remove its resources)
 *
 *	routines defined:
 *		lsUnuse()
 *
 *---------------------------------------------------------------------------
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

#include <lockset.h>

#include <internal.h>

extern void free();

/* 
 *	lsUnuse() - remove the resource for a lockset
 *
 *	NOTE:	this function negates the actions of lsNew() and lsUse().
 *
 *	input:	lockSet - the lockset ID
 *
 *	proc:	convert the lockset ID, free the memory and return.
 *
 *	output:	(void) - none
 *
 *	global:	(none)
 */

void
lsUnuse(lockSet)
lockSetT lockSet;
{
	/*
	 * translate (if possible), then free the memory.
	 */

	if (lockSet != INVALID_LOCKSET)
		free((char FAR *)lockSet);
}
