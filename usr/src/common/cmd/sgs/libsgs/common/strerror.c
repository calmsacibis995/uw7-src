#ident	"@(#)sgs:libsgs/common/strerror.c	1.2"
/*LINTLIBRARY*/
#include "synonyms.h"
#include <stdio.h>

extern const char _sys_errs[];
extern const int _sys_index[];
extern int _sys_num_err;

char *
strerror(errnum)
int errnum;
{
	if (errnum < _sys_num_err && errnum >= 0)
		return((char *)(&_sys_errs[_sys_index[errnum]]));
	else
		return(NULL);
}
