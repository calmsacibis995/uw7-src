/*
 *	@(#) getmons.c 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1994.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *      S001    Tue Jun  6 13:45:05 PDT 1995    toma
 *      - Change the printf lines to match the new format that
 *        iqm is expecting. 
 *      S000    Thu Jan 12 17:50:52 PST 1995    davidw@sco.com
 *      - Store monitors in case-insensitive order based on description string.
 *	Was storing monitors based on file name layout.
 *
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "grafinfo.h"
#include "y.tab.h"

extern int grafLineno;

extern FILE *yyin;

typedef struct _mon {
	char	*label;
	char	*description;
	char	*description_lwr;				/* S000 */
	char	*output;
	struct _mon	*next;
} mon;
	
mon *mons = (mon *) NULL, *curMon = (mon *) NULL;

/* make a string c variable name complaint - convert anything which
   is not a letter or digit to an underbar */

void fixstring (c)
char *c;
{
	int l = strlen (c);
	int i;

	for (i=0; i<l; i++)
		if (!isalnum(c[i]))
			c[i]='_';
}

Usage (n)
char *n;
{
	fprintf (stderr, "%s file1 [file2] ..\n", n);
	exit (1);
}


main (argc, argv)
int argc;
char **argv;
{
	int i;

	if (argc <2)
		Usage (argv[0]);

	for (i=1; i< argc; i++) {
		GetMons (argv[i]);
	}

	DumpMons ();
}

GetMons (filename)
char *filename;
{
	int i, t, err;
	FILE *fp;

	mon *amon;

	char vendor[256], model[256], des[256], outMon[256];


	/* open file */
	if (!(fp=fopen(filename, "r"))) {
		fprintf (stderr, "Unable to open %s for reading\n", filename);
		return;
	}

	yyin = fp;

	err = 0;

	vendor[0] = model[0] = des[0] = '\0';

	while (t=yylex()) {

		if (t!=IDENT)
			continue;

		if (strcmp (yylval.string, "DESCRIPTION")==0) {
			if (yylex()!= EQUAL) {
				err = 1;
				fprintf (stderr, "Error 1 in moninfo file %s\n",
					 filename);
				break;
			}
			if (yylex()!= STRING) {
				err = 1;
				fprintf (stderr, "Error 2 in moninfo file %s\n",
					 filename);
				break;
			}
			strcpy (des, yylval.string);
		} else if (strcmp (yylval.string, "MON_VENDOR")==0) {
			if (yylex()!= EQUAL) {
				err = 1;
				fprintf (stderr, "Error 3 in moninfo file %s\n",
					 filename);
				break;
			}
			if (yylex()!= STRING) {
				err = 1;
				fprintf (stderr, "Error 4 in moninfo file %s\n",
					 filename);
				break;
			}
			strcpy (vendor, yylval.string);
		} else if (strcmp (yylval.string, "MON_MODEL")==0) {
			if (yylex()!= EQUAL) {
				err = 1;
				fprintf (stderr, "Error 5 in moninfo file %s\n",
					 filename);
				break;
			}
			if (yylex()!= STRING) {
				err = 1;
				fprintf (stderr, "Error 6 in moninfo file %s\n",
					 filename);
				break;
			}
			strcpy (model, yylval.string);
		}
	}

	fclose (fp);

	if (err)
		return;

	/* create a new mon entry */

	if ((amon = (mon *) calloc(1, sizeof (mon))) == NULL) {
		fprintf (stderr, "Unable to malloc memory\n");
		return;
	}
	
	/* fill in the card info */

	sprintf (outMon, "%s.%s", vendor, model);

	strlwr(outMon);

	amon->description = strdup(des);
	amon->description_lwr = strdup(des);			/* S000 */
	strlwr(amon->description_lwr);				/* S000 */
	amon->output = strdup(outMon);
	amon->label = strdup(outMon);
	fixstring (amon->label);

	if (mons == (mon *) NULL) {
		mons = curMon = amon;
	} else {
		int saved = 0;					/* S000 vvv */
		mon *tmpPrev, *tmpMon;
		/* store monitor at proper position in list (case-insensitive)*/
		for (tmpMon=mons, tmpPrev=mons; tmpMon; tmpMon=tmpMon->next) {
			if (strcmp(amon->description_lwr, 
					tmpMon->description_lwr) <= 0) {
				amon->next = tmpPrev->next;
				tmpPrev->next = amon;
				saved=1;
                  		break;
			}
			tmpPrev = tmpMon;
		}
		if (!saved) {
			/* otherwise store monitor at end of list */
			amon->next = tmpPrev->next;
			tmpPrev->next = amon;
		} 						/* S000 ^^^ */
	}


}

DumpMons ()
{
        printf ("##\n");
        printf ("# video monitors\n");
	printf ("MONITORS:\n");
	for (curMon=mons; curMon; curMon=curMon->next) {
		printf ("\t%s	=%s\n", curMon->output,         
			curMon->description);			/* S001 */
	}

	printf ("\tDEFAULT\t\t=misc.standard\n\n");		/* S001 */

}
