/*
 *	@(#) getgmodes.c 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1994.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *      S001    Tue Jun  6 13:47:54 PDT 1995    toma
 *      - Change the printf lines so that the video.stz file is
 *        in the format that iqm is expecting. 
 *      S000    Thu Jan 12 17:50:52 PST 1995    davidw@sco.com
 *      - Store cards in case-insensitive order based on description string.
 *	Was storing cards based on file name layout.
 *
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "grafinfo.h"
#include "y.tab.h"

extern int grafLineno;

extern FILE *yyin;

typedef struct _mode {
	char	*label;
	char	*description;
	char	*output;
	struct _mode	*next;
} mode;

typedef struct _card {
	char	*label;
	char	*description;
	char	*description_lwr;				/* S000 */
	char	*output;
	mode	*modes;
	struct _card	*next;
} card;
	
card *cards = (card *) NULL, *curCard = (card *) NULL;

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
		GetModes (argv[i]);
	}

	DumpModes ();
}

GetModes (filename)
char *filename;
{
	int i, t, err;
	FILE *fp;

	card *acard;
	mode *amode;

	char vendor[256], model[256], class[256], moode[256],
	     vendorS[256], modelS[256], classS[256], moodeS[256],
	     outCard[256],outMode[256], desCard[256], desMode[256],
	     label[256];


	/* open file */
	if (!(fp=fopen(filename, "r"))) {
		fprintf (stderr, "Unable to open %s for reading\n", filename);
		return;
	}

	yyin = fp;

	/* get the first mode for card info */

	err = 1;

	while (t=yylex()) {

		if (t!=VENDOR)
			continue;

		strcpy (vendor, yylval.string);
		if (yylex () != STRING) {
			err = 1;
			break;
		}
		strcpy (vendorS, yylval.string);

		if (yylex()!=MODEL) {
			err = 1;
			break;
		}
		strcpy (model, yylval.string);

		if (yylex () != STRING) {
			err = 1;
			break;
		}
		strcpy (modelS, yylval.string);

		if (yylex()!=CLASS) {
			err = 1;
			break;
		}
		strcpy (class, yylval.string);

		if (yylex () != STRING) {
			err = 1;
			break;
		}
		strcpy (classS, yylval.string);

		if (yylex()!=MODE) {
			err = 1;
			break;
		}
		strcpy (moode, yylval.string);
		if (yylex () != STRING) {
			err = 1;
			break;
		}
		strcpy (moodeS, yylval.string);

		err=0;
		break;
	}

	if (err != 0) {
		fprintf (stderr, "Unable to get first mode for %s\n",
			 filename);
		fclose (fp);
		return;
	}

	/* create a new card entry */

	if ((acard = (card *) calloc(1, sizeof (card))) == NULL) {
		fprintf (stderr, "Unable to malloc memory\n");
		fclose (fp);
		return;
	}
	
	/* fill in the card info */

	sprintf (outCard, "%s.%s", vendor, model);
	sprintf (desCard, "%s %s", vendorS, modelS);

	strlwr(outCard);

	acard->description = strdup(desCard);
	acard->description_lwr = strdup(desCard);		/* S000 */
	strlwr(acard->description_lwr);				/* S000 */
	acard->output = strdup(outCard);
	acard->label = strdup(outCard);
	fixstring (acard->label);

	/* add the first mode entry */
	
	if ((amode = (mode *) calloc(1, sizeof (mode))) == NULL) {
		fprintf (stderr, "Unable to malloc memory\n");
		fclose (fp);
		return;
	}
	
	sprintf (outMode, "%s.%s", class, moode);
	sprintf (label, "%s_%s_%s", outCard, class, moode);
	fixstring (label);
	sprintf (desMode, "%s %s", classS, moodeS);

	strlwr(outMode);
	strlwr(label);

	amode->description = strdup (desMode);
	amode->output = strdup(outMode);
	amode->label = strdup(label);

	acard->modes = amode;

	/* fill in the rest of the modes. The error exit is not clean
	   but a real cleanup would be a pain.
	 */

	err = 0;

	while (t=yylex()) {

		if (t!=VENDOR)
			continue;

		strcpy (vendor, yylval.string);
		if (yylex () != STRING) {
			err = 1;
			break;
		}
		strcpy (vendorS, yylval.string);

		if (yylex()!=MODEL) {
			err = 1;
			break;
		}
		strcpy (model, yylval.string);

		if (yylex () != STRING) {
			err = 1;
			break;
		}
		strcpy (modelS, yylval.string);

		if (yylex()!=CLASS) {
			err = 1;
			break;
		}
		strcpy (class, yylval.string);

		if (yylex () != STRING) {
			err = 1;
			break;
		}
		strcpy (classS, yylval.string);

		if (yylex()!=MODE) {
			err = 1;
			break;
		}
		strcpy (moode, yylval.string);
		if (yylex () != STRING) {
			err = 1;
			break;
		}
		strcpy (moodeS, yylval.string);

		/* add the mode entry */
		
		if ((amode->next = (mode *) calloc(1, sizeof (mode))) == NULL) {
			fprintf (stderr, "Unable to malloc memory\n");
			fclose (fp);
			return;
		}
	
		amode = amode->next;

		sprintf (outMode, "%s.%s", class, moode);
		sprintf (label, "%s_%s_%s", outCard, class, moode);
		fixstring (label);
		sprintf (desMode, "%s %s", classS, moodeS);

		strlwr(outMode);
		strlwr(label);

		amode->description = strdup (desMode);
		amode->output = strdup(outMode);
		amode->label = strdup(label);
	}

	fclose (fp);

	if (err) {
	    fprintf (stderr, "%s:%d Error in vendor/model/class/mode def\n",
	    	     filename, grafLineno);

	    return;
	}

	/* link it into the cards list */

	if (cards == (card *) NULL) {
		cards = curCard = acard;
	} else {
		int saved = 0;					/* S000 vvv */
		card *tmpPrev, *tmpCard;
		/* store card at proper position in list (case-insensitive)*/
		for (tmpCard=cards, tmpPrev=cards; tmpCard; 
					tmpCard=tmpCard->next) {
			if (strcmp(acard->description_lwr, 
					tmpCard->description_lwr) <= 0) {
				acard->next = tmpPrev->next;
				tmpPrev->next = acard;
				saved=1;
                  		break;
			}
			tmpPrev = tmpCard;
		}
		if (!saved) {
			/* otherwise store card at end of list */
			acard->next = tmpPrev->next;
			tmpPrev->next = acard;
		} 						/* S000 ^^^ */
	}
}

DumpModes ()
{
	mode *amode;

	printf ("####\n");
	printf ("#  Hardware->video parameters\n");
	printf ("#\n");
	printf ("VIDEO_CARDS:\n");
	for (curCard=cards; curCard; curCard=curCard->next) {
		printf ("\t%s	=%s\n", curCard->output,        /* S001 */
			curCard->description);
	}

        printf ("\tDEFAULT\t\t=ibm.vga\n\n");

	for (curCard=cards; curCard; curCard=curCard->next) {
		printf ("%s:\n", curCard->output);
		for (amode=curCard->modes; amode; amode=amode->next)
			printf ("\t%s\t=%s\n", amode->output,
				amode->description);
		printf ("\n");

	}
}
