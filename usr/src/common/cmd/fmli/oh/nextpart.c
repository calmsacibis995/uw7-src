/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */

#ident	"@(#)fmli:oh/nextpart.c	1.4.3.3"

#include <stdio.h>
#include <string.h>
#include "inc.types.h"		/* abs s14 */
#include "but.h"
#include "wish.h"
#include "typetab.h"
#include "optabdefs.h"
#include "partabdefs.h"

struct one_part *
opt_next_part(entry)
struct opt_entry *entry;
{
	static int partsleft;
	static int curoffset;
	struct one_part *retval;
	extern struct one_part  Parts[MAXPARTS];

	if (entry) {
		partsleft = entry->numparts;
		curoffset = entry->part_offset;
	}
	if (partsleft > 0) {
		retval = Parts + curoffset++;
		partsleft--;
	} else
		retval = NULL;

	return(retval);
}
