#include <stdio.h>

b_argprint(argc, argv)
int argc;
char *argv[];
{
	register int i;

	for (i = 1; i < argc; i++) {
		altprintf("ARG[%d] = %s\n", i, argv[i]);
	}
	return(0);
}
