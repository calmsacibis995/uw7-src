#ident	"@(#)sgs:libsgs/common/gettxt.c	1.3"

/*	gettxt()
 *	returns the default message
 */
#ifdef __STDC__
	#pragma weak gettxt = _gettxt
#endif
#include "synonyms.h"

/*ARGSUSED*/
char *
gettxt(msgid, dflt)
const char *msgid, *dflt;
{
	return((char *)dflt);
}
