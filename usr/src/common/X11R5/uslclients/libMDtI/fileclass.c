#ifndef NOIDENT
#ident	"@(#)libMDtI:fileclass.c	1.5"
#endif

#include <X11/Intrinsic.h>
#include "DesktopP.h"

/*
 * This file contains routines to maintain an array of file class structures.
 */


/*
 * Allocate an entry for a new class.
 */
DmFclassPtr
DmNewFileClass(key)
void *key;	/* ptr to FmodeKey or FnameKey */
{
	DmFclassPtr fcp;

	if (fcp = (DmFclassPtr)calloc(1, sizeof(DmFclassRec)))
		fcp->key  = key;
	return(fcp);
}

