#ident	"@(#)sccs:lib/mpwlib/zero.c	6.2"
/*
	Zero `cnt' bytes starting at the address `ptr'.
	Return `ptr'.
*/

char	*zero(p,n)
register char *p;
register int n;
{
	char *op = p;
	while (--n >= 0)
		*p++ = 0;
	return(op);
}
