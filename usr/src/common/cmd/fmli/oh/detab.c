/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */

#ident	"@(#)fmli:oh/detab.c	1.5.3.3"

#include <stdio.h>
#include "inc.types.h"		/* abs s14 */
#include "wish.h"
#include "typetab.h"
#include "detabdefs.h"

/* the Object Detection Function Table for this FMLI session */
struct odft_entry Detab[MAXODFT];
