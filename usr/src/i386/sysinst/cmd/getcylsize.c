#ident	"@(#)getcylsize.c	15.1	97/12/19"
#include <stdlib.h>

#define FIRSTVAR	1024.0
#define ONEMEG		1048576.0

main(int argc, char **argv)
{
	printf("%d\n", (int)((atof(argv[1])) * (atof(argv[2])) *
	  (atof(argv[3])) * FIRSTVAR / ONEMEG));
	return 0;
}
