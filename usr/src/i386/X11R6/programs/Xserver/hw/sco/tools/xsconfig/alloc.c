/*
 *	@(#) alloc.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1991.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *
 */
#if defined(SCCS_ID)
static char Sccs_Id[] =
	 "@(#) alloc.c 11.1 97/10/22
#endif

/*
   (c) Copyright 1989 by Locus Computing Corporation.  ALL RIGHTS RESERVED.

   This material contains valuable proprietary products and trade secrets
   of Locus Computing Corporation, embodying substantial creative efforts
   and confidential information, ideas and expressions.  No part of this
   material may be used, reproduced or transmitted in any form or by any
   means, electronic, mechanical, or otherwise, including photocopying or
   recording, or in connection with any information storage or retrieval
   system without permission in writing from Locus Computing Corporation.
*/

/*
   alloc.c - wrappers for malloc, realloc and free
*/

#include <malloc.h>
#include <string.h>
#include <stdio.h>

#include "alloc.h"
#include "xserror.h"

extern ErrorMessage errHeap;

char *
AllocMem(size)
unsigned int size;
{
    char *p;
    
    if ((p = malloc(size)) == NULL) {
	ErrorMsg(errHeap);
	exit(1);
    }
    
    return p;
}

char *
ReallocMem(ptr, size)
char *ptr;
unsigned int size;
{
    if (ptr)
	ptr = realloc(ptr, size);
    else
	ptr = malloc(size);

    if (ptr == NULL) {
	ErrorMsg(errHeap);
	exit(1);
    }
    
    return ptr;
}

void
FreeMem(ptr)
char *ptr;
{
    if (ptr)
    	free(ptr);
}


char *
StrDup(str)
char *str;
{
    char *s;
    
    if ((s = strdup(str)) == NULL) {
	ErrorMsg(errHeap);
	exit(1);
    }
    
    return s;
}
