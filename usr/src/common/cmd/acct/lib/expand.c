/*		copyright	"%c%" 	*/

#ident	"@(#)acct:common/cmd/acct/lib/expand.c	1.7.3.3"
#ident "$Header$"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/acct.h>

time_t
expand(ct)
register comp_t ct;
{
	register e;
	register long f;
	e = (ct >> 13) & 07;
	f = ct & 017777;

	while (e-- > 0) 
		f <<=3;

	return f;
}
