/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/strupr.c	1.1.1.2"
#ident  "$Header$"

/* #define	DEBUG		1	/* */

#include	<stdio.h>

/*
			strupr(string)

	Converts passed string to upper case.
*/
strupr(target_file)
char	*target_file;
{
	int	i;

#ifdef DEBUG
	(void) fprintf(stderr, "strupr(): DEBUG - Original value \"%s\"\n", target_file);
#endif

	/*
		Convert target_file to all uppercase
	*/
	for (i = 0; *(target_file + i) != '\0'; i++)
		if (*(target_file + i) > '\140' && *(target_file + i) < '\173')
			*(target_file + i) &= '\337';

#ifdef DEBUG
	(void) fprintf(stderr, "strupr(): DEBUG - Final value \"%s\"\n", target_file);
#endif
}
