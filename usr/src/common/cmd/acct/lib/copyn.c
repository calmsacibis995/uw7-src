/*		copyright	"%c%" 	*/

#ident	"@(#)acct:common/cmd/acct/lib/copyn.c	1.5.3.3"
#ident "$Header$"
/*
 * Copy n bytes from s2 to s1
 * return s1
 */

char *
copyn(s1, s2, n)
register char *s1, *s2;
{
	register i;
	register char *os1;

	os1 = s1;
	for (i = 0; i < n; i++)
		*s1++ = *s2++;
	return(os1);
}
