#ident	"@(#)sccs:lib/mpwlib/imatch.c	6.3"
/*
	initial match
	if `prefix' is a prefix of `string' return 1
	else return 0
*/

int
imatch(prefix,string)
register char *prefix, *string;
{
	while (*prefix++ == *string++)
		if (*prefix == 0)
			return(1);
	return(0);
}
