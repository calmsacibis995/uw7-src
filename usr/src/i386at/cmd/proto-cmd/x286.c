#ident	"@(#)x286.c	15.1"

/*
** Stub for printing out what to run when XENIX isn't loaded.
*/

main(argc, argv)
int argc;
char *argv[];
{
	printf ("The \"%s\" executable is a XENIX 286 executable.\n", argv[0]);
	printf ("Please load the \"XENIX Compatibility Package\" to run this executable.\n");
	exit (1);
}
