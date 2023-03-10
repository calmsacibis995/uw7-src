/*		copyright	"%c%" 	*/

#ident	"@(#)addstring.c	1.2"
#ident	"$Header$"

#include "string.h"
#include "errno.h"
#include "stdlib.h"

#include "lp.h"

/**
 ** addstring() - ADD ONE STRING TO ANOTHER, ALLOCATING SPACE AS NEEDED
 **/

int
#if	defined(__STDC__)
addstring (
	char **			dst,
	char *			src
)
#else
addstring (dst, src)
	char			**dst;
	char			*src;
#endif
{
	size_t			len;

	if (!dst || !src) {
		errno = EINVAL;
		return (-1);
	}

	len = strlen(src) + 1;
    
	if (*dst) {
		if (!(*dst = Realloc(*dst, strlen(*dst) + len))) {
			errno = ENOMEM;
			return (-1);
		}
	} else {
		if (!(*dst = Malloc(len))) {
			errno = ENOMEM;
			return (-1);
		}
		(*dst)[0] = '\0';
	}

	(void) strcat(*dst, src);
	return (0);
}
