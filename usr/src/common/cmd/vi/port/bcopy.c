/*		copyright	"%c%" 	*/

#ident	"@(#)vi:port/bcopy.c	1.8.1.4"
#ident  "$Header$"
bcopy(from, to, count)
	register unsigned char *from, *to;
	register int count;
{
	while ((count--) > 0)
		*to++ = *from++;
}
