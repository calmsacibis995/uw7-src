#ident	"@(#)sccs:lib/cassi/error.c	6.4"
#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <sys/euc.h>
#include <limits.h>
void error(dummy)	
	char *dummy;
	{
	 pfmt(stdout, MM_NOSTD,dummy);
	 printf("\n");
	}
