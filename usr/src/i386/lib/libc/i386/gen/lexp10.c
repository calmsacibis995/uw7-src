
#ident	"@(#)libc-i386:gen/lexp10.c	1.2"

#ifdef __STDC__
	#pragma weak lexp10 = _lexp10
#endif
#include	"synonyms.h"
#include	<sys/types.h>
#include	<sys/dl.h>

dl_t
lexp10(exp)
dl_t	exp;
{
	dl_t	result;

	result = lone;

	while(exp.dl_hop != 0  ||  exp.dl_lop != 0){
		result = lmul(result, lten);
		exp    = lsub(exp, lone);
	}

	return(result);
}
