#ident	"@(#)sccs:lib/mpwlib/userexit.c	6.2"
/*
	Default userexit routine for fatal and setsig.
	User supplied userexit routines can be used for logging.
*/

userexit(code)
{
	return(code);
}
