/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/basename.c	1.1.1.2"
#ident  "$Header$"

#include	"MS-DOS.h"

#include	<stdio.h>
 
char	*
basename(input)
char	*input;
{
	char	*c_ptr;

	if ((c_ptr = strrchr(input + 1, '/')) == NULL)
		c_ptr = *input == '/' ? input + 1 : input;
	else
		c_ptr++;

	return(c_ptr);
}
