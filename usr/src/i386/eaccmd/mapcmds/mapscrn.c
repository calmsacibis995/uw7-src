#ident	"@(#)mapscrn.c	1.2"
/*
 *	@(#) mapscrn.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987, 1988.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */
/*
 *	mapscrn - get/set console screenmap table.
 *
 */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>

#define DEFAULT "/usr/lib/console/screens"
int 	pervt, kern;
struct scrn_dflt screen;

FILE *fp, *fopen();

main(argc, argv)
int argc;
char *argv[];
{
	char *datafile = DEFAULT;
	extern int	optind;
	extern char	*optarg;
	int	ch = 0;
	char	*basename = argv[0];
        
	fflush(stdout);
	kern = 0;
	pervt = 1; /* per VT by default */
	while ((ch = getopt(argc, argv, "dg")) != EOF) {
		switch (ch) {
		case 'd': kern = 1;
			  break;
		case 'g':
			pervt = 0; /* global change */
			break;
		default:
			fprintf(stderr, "usage: %s [-dg] [file]\n", basename);
			exit(1);
		}
	}
	if ( optind < argc ) {
		datafile = argv[optind++];
	}
	if ( optind < argc ) {
		fprintf(stderr, "usage: %s [-dg] [file]\n", basename);
		exit(1);
	}

	if (ioctl(0, KIOCINFO, 0) < 0 ) {
		fprintf(stderr,"Not the console driver\n");
		exit(1);
	}
	if ( kern ) {
		displaytab();
		exit(0);
	} 
	puttab(datafile);
	exit(0);
}

displaytab()
{
	unsigned k, i;

	if (pervt) {
	   if (ioctl(0, GIO_SCRNMAP, &screen.scrn_map) < 0) {
		perror("console screen configuration");
		exit(1);
	   }
	}
	else {
	   screen.scrn_direction = KD_DFLTGET;
	   if (ioctl(0, KDDFLTSCRNMAP, &screen) < 0) {
		perror("Error executing KDDFLTSCRNMAP ioctl");
		exit(1);
	   }
	}

    	printf("Font values:\n");
    	for(i=0; i<NUM_ASCII; i++) {   
		printf("'%s'", printc(screen.scrn_map[i]));
		printf( (i%8==7) ? "\n" : "\t");
    	}
}

puttab(fname)
char *fname;
{
	int i, k, quote;

	if (NULL == (fp = fopen(fname,"r"))) {   
	perror(fname);
	exit(1);
	}
	for(i=0; i<NUM_ASCII; i++) {   
	   while('\'' != (quote=getc(fp)))	/* scan to the open quote */
	       if (quote == EOF) {   
		   printf("Premature end of file\n");
		   printf("configuation aborted\n");
		   exit(1);
	       }
	   k = readc(fp);
	   if('\'' != (quote=getc(fp))) {	/* check for closing quote */
	       printf("Missing closing quote on character '%s'\n",printc(k));
	       printf("%s is where the ' was expected\n", quote);
	       printf("configuation aborted\n");
	       exit(1);
	   }
	   screen.scrn_map[i] = k;
	}
	if (pervt) {
	   if (ioctl(0, PIO_SCRNMAP, &screen.scrn_map) < 0) {
		fprintf(stderr,"Error executing PIO_SCRNMAP ioctl\n");
		exit(1);
	   }
	} else {
	   screen.scrn_direction = KD_DFLTSET;
	   if (ioctl(0, KDDFLTSCRNMAP, &screen) < 0) {
		fprintf(stderr,"Error executing KDDFLTSCRNMAP ioctl\n");
		exit(1);
	   }
	}
}
