/*		copyright	"%c%" 	*/

#ident	"@(#)rconsole:i386/cmd/rconsole/conflgs.c	1.1.4.2"
#ident  "$Header$"

/* conflgs: does nothing on the generic product.
 */

#include <stdio.h>

main(argc, argv)
int	argc;
char	**argv;
{
	fprintf(stderr, "%s will not work on this core platform\n", argv[0]);
	exit(0);
}
