
#ident	"@(#)not_found.c	15.1"

char STR1[] = "An attempt was made to execute the ";
char STR2[] = " program.  This program is installed as part of the ";
char STR3[] = " package which you do not have on your system.  Please install this package before attempting to run this program again.  Consult your 'Getting Started Guide' for further assistance.";

#define CNT 2

main(argc, argv)
int argc;
char *argv[];
{
#ifndef CNT
	int CNT;
	for (CNT=1; CNT<=2; i++)
#endif
	char buf[5120];
	{
		strcpy (buf,"message -d \"");
		strcat (buf, STR1);
		strcat (buf, argv[0]);
		strcat (buf, STR2);
		strcat (buf, PACKAGE);
		strcat (buf, STR3);
		strcat (buf, "\" 1>&2");
		system (buf);
	}
	exit (2);
}
