#ident	"@(#)pcintf:pkg_lockset/lset_estr.c	1.1.1.2"
#include <sccs.h>

SCCSID(@(#)lset_estr.c	3.1);	/* 10/15/90 15:38:34 */

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

/*------------------------------------------------------------------------
 *
 *	lset_estr.c - error strings for specific LSERR errors
 *
 *	routines included:
 *		lsEString()
 *
 *-------------------------------------------------------------------------
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include <lockset.h>

#include <lmf.h>

#define NLS_DOMAIN "LCC.PCI.UNIX.LSET_ESTR"

/* Explanation: The path will be /pci/lib/en.lmf, the domain will
 * be: LCC.PCI.UNIX.LSET_ESTR
 */

#define NLS_BUFSIZE	255

/*
 * the unknown string (actually, a format).
 */

#define UNKNOWN_STR	

/*
 *	lsEString() - print a string for a specific LSERR error
 *
 *	input:	error - the LSERR error number
 *
 *	proc:	just switch and return
 *
 *	output:	(char *) - the error string
 *
 *	global:	(none)
 */

char FAR *
lsEString(error)
int error;
{	
	char *answer = NULL;

	lmf_push_domain(NLS_DOMAIN);

	/*
	 * if the error was added correctly, it will be in this list.
	 */

	switch (error)
	{
		case LSERR_NOERR:
				answer = lmf_get_message_copy("LSET_ESTR0",
					"not an error");
				break;
		case LSERR_SYSTEM:	
				answer = lmf_get_message_copy("LSET_ESTR1",
					"system error");
				break;
		case LSERR_PARAM:	
				answer = lmf_get_message_copy("LSET_ESTR2",
					"parameter error");
				break;
		case LSERR_TABLE_LOCK:	
				answer = lmf_get_message_copy("LSET_ESTR3",
					"table is locked/unlocked");
				break;
		case LSERR_CHG_LOCK:
				answer = lmf_get_message_copy("LSET_ESTR4",
					"can't lock/unlock table");
				break;
		case LSERR_GETKEY:	
				answer = lmf_get_message_copy("LSET_ESTR5",
					"can't get IPC identifier");
				break;
		case LSERR_LOCKSET:	
				answer = lmf_get_message_copy("LSET_ESTR6",
					"can't create/access lock set");
				break;
		case LSERR_ATTACH:
				answer = lmf_get_message_copy("LSET_ESTR7",
					"can't attach share mem segment");
				break;
		case LSERR_SPACE:
				answer = lmf_get_message_copy("LSET_ESTR8",
					"no memory/table space");
				break;
		case LSERR_INUSE:	
				answer = lmf_get_message_copy("LSET_ESTR9",
					"specified slot is in use");
				break;
		case LSERR_UNUSED:
				answer = lmf_get_message_copy("LSET_ESTR10",
					"specified slot is unused");
				break;
		case LSERR_EXIST:
				answer = lmf_get_message_copy("LSET_ESTR11",
					"lockset already exists");
				break;
		case LSERR_PERM:
				answer = lmf_get_message_copy("LSET_ESTR12",
					"permission denied");
				break;
	}

	if ( answer == NULL ) {
		char unknownStr[NLS_BUFSIZE];
		/*
		 * If we got here, the error was added, 
		 * but no corresponding string exists. 
		 * Try to give the user some information.
		 */

		(void)sprintf(unknownStr, 
			lmf_get_message("LSET_ESTR13",
			"unknown LSERR error (%d)"),
			error);

		lmf_pop_domain();
		return unknownStr;
	}

	lmf_pop_domain();
	return answer;
}
