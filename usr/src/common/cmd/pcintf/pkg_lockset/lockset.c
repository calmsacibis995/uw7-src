#ident	"@(#)pcintf:pkg_lockset/lockset.c	1.1.1.2"
#include <sccs.h>

SCCSID(@(#)lockset.c	3.1);  /* 10/15/90 15:38:18 */

/*
   (c) Copyright 1985 by Locus Computing Corporation.  ALL RIGHTS RESERVED.

   This material contains valuable proprietary products and trade secrets
   of Locus Computing Corporation, embodying substantial creative efforts
   and confidential information, ideas and expressions.	 No part of this
   material may be used, reproduced or transmitted in any form or by any
   means, electronic, mechanical, or otherwise, including photocopying or
   recording, or in connection with any information storage or retrieval
   system without permission in writing from Locus Computing Corporation.
*/


/*--------------------------------------------------------------------------
 *
 *	lockset.c - global values for the LOCKSET package
 *
 *	routines included:
 *		(none)
 *
 *--------------------------------------------------------------------------
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include <lockset.h>

/*
 * externally visible variables.  these values may be checked by the user.
 */

int	lsetErr	=	 LSERR_NOERR;	/* error condition */
