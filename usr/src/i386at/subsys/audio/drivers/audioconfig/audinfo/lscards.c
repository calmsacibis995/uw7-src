/*
 *	@(#)lscards.c	7.1	10/22/97	12:21:47
 */
#include "confdata.h"
#include <stdio.h>

int
main()
{
	int i;

	load_confdata("config.dat");

	for (i=0;i<nmenus;i++)
	    printf("%s\n", menus[i].name);

	exit(0);
}
