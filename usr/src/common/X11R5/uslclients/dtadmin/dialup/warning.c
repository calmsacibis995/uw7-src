#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/warning.c	1.2"
#endif

#include <stdio.h>

extern void	OlWarning();
extern void	OlError();
/*
 * Warning() - hack to add printf capability to OlWarning.  
 * WARNING: if this doesn't work on your machine, ensure that the type 
 * for p0-10 matches your stack type.  That is, if your stack pushes 
 * things as longs, change the declaration here.
 * (On almost all UNIX machines, stack_type == int == long == 32 bits)
 */
void
Warning(format, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)
	char	*format;
	int	p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10;
{
	char	buf[BUFSIZ];

	sprintf(buf, format, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
	OlWarning(buf);
}

void
Error(format, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)
	char	*format;
	int	p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10;
{
	char	buf[BUFSIZ];

	sprintf(buf, format, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
	OlError(buf);
}
