/*		copyright	"%c%" 	*/

#ident	"@(#)getopt:getopt.c	1.7.4.2"
#ident "$Header$"

#include	<stdio.h>
#include	<locale.h>
#include	<pfmt.h>

extern void exit();
extern char *strcat();
extern char *strchr();

main(argc, argv)
int argc;
char **argv;
{
	extern	int optind;
	extern	char *optarg;
	register int	c;
	int	errflg = 0;
	char	tmpstr[4];
	char	outstr[5120];
	char	*goarg;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:getopt");

	if(argc < 2) {
		pfmt(stderr, MM_ERROR, ":1:Incorrect usage\n");
		pfmt(stderr, MM_ACTION, ":306:Usage: getopt legal-args $*\n");
		exit(2);
	}

	goarg = argv[1];
	argv[1] = argv[0];
	argv++;
	argc--;
	outstr[0] = '\0';

	while((c=getopt(argc, argv, goarg)) != EOF) {
		if(c=='?') {
			errflg++;
			continue;
		}

		tmpstr[0] = '-';
		tmpstr[1] = c;
		tmpstr[2] = ' ';
		tmpstr[3] = '\0';

		strcat(outstr, tmpstr);

		if(*(strchr(goarg, c)+1) == ':') {
			strcat(outstr, optarg);
			strcat(outstr, " ");
		}
	}

	if(errflg) {
		exit(2);
	}

	strcat(outstr, "-- ");
	while(optind < argc) {
		strcat(outstr, argv[optind++]);
		strcat(outstr, " ");
	}

	(void) printf("%s\n", outstr);
	exit(0);	/*NOTREACHED*/
}
