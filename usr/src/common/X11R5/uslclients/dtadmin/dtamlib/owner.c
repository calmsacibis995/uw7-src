#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:dtamlib/owner.c	1.7"
#endif
/*
 *	owner.c:	validate owner privileges
 *
 */
#include <stdio.h>
#include <string.h>
#include <X11/Intrinsic.h>
#include "owner.h"


Boolean	_DtamIsOwner(char *adm_name)
{
	char	buf[BUFSIZ];

	sprintf(buf, "/sbin/tfadmin -t %s 2>/dev/null", adm_name);
	return (system(buf)==0);
}
