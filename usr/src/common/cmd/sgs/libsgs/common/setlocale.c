#ident	"@(#)sgs:libsgs/common/setlocale.c	1.2"
/*
* setlocale - dummy function for the cross environment
*/
#include "synonyms.h"
#include <stdio.h>

/*ARGSUSED*/
char *
setlocale(cat, loc)
int cat;
const char *loc;
{
	return (char *)NULL;
}
