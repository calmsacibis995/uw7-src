#ident	"@(#)sgs:libsgs/common/setcat.c	1.2"

#ifdef __STDC__	
	#pragma weak setcat = _setcat
#endif
#include "synonyms.h"
#include <stdio.h>

/* setcat(cat): dummy function.  Returns NULL since there is no
 * message catalog in the cross environment
 */
/*ARGSUSED*/
const char *
setcat(cat)
const char *cat;
{
	return NULL;
}
