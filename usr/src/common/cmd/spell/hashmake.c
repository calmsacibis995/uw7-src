/*		copyright	"%c%" 	*/

#ident	"@(#)spell:hashmake.c	1.2.1.3"
#ident "$Header$"
#include <stdio.h>
#include "hash.h"

main()
{
	char word[30];
	long h;
	hashinit();
	while(fgets(word,sizeof(word),stdin)) {
		word[strlen(word)-1] = '\0';
		printf("%.*lo\n",(HASHWIDTH+2)/3,hash(word));
	}
	return(0);
}
