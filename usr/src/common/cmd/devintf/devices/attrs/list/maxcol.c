#ident	"@(#)devintf:common/cmd/devintf/devices/attrs/list/maxcol.c	1.1.3.2"
#ident  "$Header$"

#include <stdio.h>

main(argc,argv)
int argc;
char **argv;
{
	int maxcol = 0;
	int linecol = 0;
 	int c;
	while ((c = getchar()) != EOF) {
	   if (c == '\n') {
	      if (linecol > maxcol)
		 maxcol = linecol;
	      linecol = 0;
	      continue;
	   }
	   linecol++;
	}
	printf("%d\n", maxcol);
}
