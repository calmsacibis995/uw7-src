/*		copyright	"%c%" 	*/

#ident	"@(#)cmp:cmp.c	1.4.3.3"

/***************************************************************************
 * Command: cmp
 * Inheritable Privileges: P_DACREAD,P_MACREAD
 *       Fixed Privileges: None
 * Notes: compares the difference of two files
 *
 ***************************************************************************/

/*
 *	compare two files
*/

#include	<stdio.h>
#include	<ctype.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<errno.h>
#include	<string.h>
#include	<stdlib.h>

FILE	*file1,*file2;

int	eflg;
int	lflg = 1;

llong_t	line = 1;
llong_t	chr = 0;
llong_t	skip1 = 0;
llong_t	skip2 = 0;

static char posix_var[] = "POSIX2";
static int posix; 

llong_t	otoi(char *);

/*
 * Procedure:     main
 *
 * Restrictions:
                 setlocale: 	none
                 fopen: 	none
		 pfmt:		none
 */
main(argc, argv)
char **argv;
{
	register c1, c2;
	extern int optind;
	int opt;


	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:cmp");

	if (getenv(posix_var) != 0){
                posix = 1;
        } else  {
                posix = 0;
        }

	while ((opt = getopt(argc, argv, "ls")) != EOF){
		switch(opt){
		case 's':
			if (lflg != 1)
				usage(1);
			lflg--;
			continue;
		case 'l':
			if (lflg != 1)
				usage(1);
			lflg++;
			continue;
		default:
			usage(0);
		}
	}
	argc -= optind;
	if(argc < 2 || argc > 4)
		usage(1);
	argv += optind;


	if(strcmp(argv[0], "-") == 0)
		file1 = stdin;
	else if((file1 = fopen(argv[0], "r")) == NULL) {
		barg(argv[0]);
	}

	if(strcmp(argv[1], "-") == 0)
		file2 = stdin;
	else if((file2 = fopen(argv[1], "r")) == NULL) {
		barg(argv[1]);
	}

	if (argc>2)
		skip1 = otoi(argv[2]);
	if (argc>3)
		skip2 = otoi(argv[3]);
	while (skip1) {
		if ((c1 = getc(file1)) == EOF) {
			earg(argv[0]);
		}
		skip1--;
	}
	while (skip2) {
		if ((c2 = getc(file2)) == EOF) {
			earg(argv[1]);
		}
		skip2--;
	}

	while(1) {
		chr++;
		c1 = getc(file1);
		c2 = getc(file2);
		if(c1 == c2) {
			if (c1 == '\n')
				line++;
			if(c1 == EOF) {
				if(eflg)
					exit(1);
				exit(0);
			}
			continue;
		}
		if(lflg == 0)
			exit(1);
		if(c1 == EOF)
			earg(argv[0]);
		if(c2 == EOF)
			earg(argv[1]);
		if(lflg == 1) {
			pfmt(stdout, MM_NOSTD,
				":28:%s %s differ: char %Ld, line %Ld\n",
				argv[0], argv[1], chr, line);
			exit(1);
		}
		eflg = 1;
		printf("%6Ld %3o %3o\n", chr, c1, c2);
	}
}

/*
 * Procedure:     otoi
 *
 * Restrictions:
                 isdigit: 	none
*/
llong_t
otoi(s)
char *s;
{
	llong_t v;
	char *termp;

	if (*s == '0')
		v = strtoll(s, &termp, 8);
	else
		v = strtoll(s, &termp, 10);

	if ((*termp != '\0') || (v < 0LL)) {
		usage(1);
	}
	return(v);
}

/*
 * Procedure:     usage
 *
 * Restrictions:
                 pfmt: 	none
*/
usage(complain)
int complain;
{
	if (complain)
		pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
	pfmt(stderr, MM_ACTION, ":1117:Usage: cmp [-l] [-s] filename1 filename2 [skip1 [skip2] ]\n");
	exit(2);
}

/*
 * Procedure:     barg
 *
 * Restrictions:
                 pfmt:		none
                 strerror: 	none
*/
barg(name)
char *name;
{
	if (lflg)
		pfmt(stderr, MM_ERROR, ":4:Cannot open %s: %s\n", name,
			strerror(errno));
	exit(2);
}

/*
 * Procedure:     earg
 *
 * Restrictions:
                 pfmt: 	none
*/
earg(name)
char *name;
{
	if (lflg) {
		if (posix) {
			fprintf(stderr,"cmp: ");
			pfmt(stderr,MM_NOSTD,":30:EOF on %s\n", name);
		} else
			pfmt(stderr,MM_INFO,":30:EOF on %s\n", name);
	}
	exit(1);
}
