/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/fix_slash.c	1.1.1.2"
#ident  "$Header$"

/* #define	DEBUG		1	/* */

#include	<stdio.h>

/*
			fix_slash(string)

	Converts back slashes in pssed string to forward slashes
*/
fix_slash(target_file)
char	*target_file;
{
	int	i;

#ifdef DEBUG
	(void) fprintf(stderr, "fix_slash(): DEBUG - Original value \"%s\"\n", target_file);
#endif

	/*
		Convert target_file
	*/
	for (i = 0; *(target_file + i) != '\0'; i++)
		if (*(target_file + i) == '\\')
			*(target_file + i) = '/';

#ifdef DEBUG
	(void) fprintf(stderr, "fix_slash(): DEBUG - Final value \"%s\"\n", target_file);
#endif
}
