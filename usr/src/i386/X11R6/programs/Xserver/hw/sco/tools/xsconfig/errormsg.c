/*
 *	@(#) errormsg.c 11.1 97/10/22
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
	 "@(#) errormsg.c 11.1 97/10/22
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

#include <stdio.h>
#include <varargs.h>

#include "errormsg.h"

/*
   errormsg.c - write an error message
*/

char errHeap[] = "Out of heap space.\n";

void
ErrorMsg(fmt, ...)
{
    vfprintf(stderr, fmt, &fmt+1);
}


