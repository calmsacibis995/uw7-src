#ident	"@(#)truss:i386/cmd/truss/prt.c	1.1.3.1"
#ident	"$Header$"

#include <stdio.h>

#include "pcontrol.h"
#include "ramdata.h"
#include "systable.h"
#include "proto.h"
#include "print.h"
#include "machdep.h"

/* print sysi86 code */
int
prt_si86(int val, int raw)
{
	register CONST char * s = raw? NULL : si86name(val);

	if (s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}
