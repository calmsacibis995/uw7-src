#ident	"@(#)sccs:lib/mpwlib/strend.c	6.2"
char *strend(p)
register char *p;
{
	while (*p++)
		;
	return(--p);
}
