#ident	"@(#)crash:i386/cmd/crash/kmasym.c	1.1"
#ident "$Header$"

#include <stdio.h>
#include <sys/mod.h>

char *Addr2Symbol(addr)
unsigned long addr;
{
	static char symbol[MAXSYMNMLEN + 12];
	unsigned long offset;

	if(getksym(symbol, &addr, &offset) != 0) {
		sprintf(symbol,"%-#8x",addr);
		return(symbol);
	}
	if(offset != 0) 
		sprintf(symbol+strlen(symbol)," + %x",offset);

	return(symbol);
}
