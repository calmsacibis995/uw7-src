#ident	"@(#)sgs:libsgs/common/setlabel.c	1.2"

#ifdef __STDC__
	#pragma weak setlabel = _setlabel
#endif
#include "synonyms.h"

/* setlabel -- does notthing -- a dummy function for the cross envrionment
 * since the cross environment does not have message catalogs
 */

/*ARGSUSED*/
int
setlabel(label)
const char *label;
{
	return 0;
}
