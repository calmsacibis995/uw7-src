#ident	"@(#)sccs:lib/mpwlib/logname.c	6.3"
char *
logname()
{
	char	 *getenv();
	return(getenv("LOGNAME"));
}
