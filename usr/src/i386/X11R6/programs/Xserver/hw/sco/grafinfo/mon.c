/*
 * @(#) mon.c 12.1 95/05/09 
 *
 * Copyright (C) 1983-1991 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000 Sep 12	pavelr
 *	-  Created File
 *	S002	Mon Sep 30 22:55:11 PDT 1991	mikep@sco.com
 *	- Change strdup() to Xstrdup()
 *	S003	Thu Sep 03 16:28:59 PDT 1992	hiramc@sco.COM
 *		declare Xstrdup correctly to avoid compiler warnings
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "os.h"
#include "grafinfo.h"
#include "y.tab.h"

extern char * Xstrdup( char * );	/*	in server/os/utils.c S003 */

/* from the lexical analyzer */
extern FILE	*yyin;

/* default monitor files and directories */
char *moninfo = "grafmon";
char *mondir  = "moninfo";
extern char *grafdir;

char *grafGetMonFilename (g)
grafData *g;
{
	FILE *fp;
	char tmp[256], m_vendor[256], m_model[256], *p, g_mv[256], m_mv[256];

	char *g_vendor, *g_model;
	int monlen;

	if (! grafGetString(g, "VENDOR", &g_vendor)
	     || ! grafGetString(g, "MODEL", &g_model)) {
		graferrno = GEMONFILE;
		return (NULL);
	}

	sprintf (g_mv, "%s.%s", g_vendor, g_model);
	strlwr(g_mv);

	monlen = strlen (g_mv);
	sprintf (tmp, "%s/%s", grafdir, moninfo);
	if (!(fp=fopen(tmp, "r"))) {
		graferrno = GEMONFILE;
		return (FAILURE);
	}

	while (fgets (tmp, 256, fp)) {
		tmp[monlen] = 0;
		if (strcmp (tmp, g_mv) == 0) {
			fclose (fp);
			strcpy(m_mv, &tmp[monlen+1]);
		} /* if (strcmp ... */
	} /* while */
	fclose (fp);
                

	if (sscanf (m_mv, "%[^.].%s", m_vendor, m_model) != 2) {
		graferrno = GEMONFORMAT;
		return (FAILURE);
	}

	sprintf (tmp, "%s/%s/%s/%s.mon", grafdir, mondir, m_vendor, m_model);

	p = Xstrdup (tmp);
	if (p==NULL)
		graferrno = GEALLOC;
	return (p);
}
	

grafParseMon (g)
grafData *g;
{
	FILE *fp;
	int t;
	char id[256], *filename;

	if ((filename = grafGetMonFilename (g)) == NULL)
		return (FAILURE);

	/* open file */
	if (!(fp=fopen(filename, "r"))) {
		graferrno = GEMONOPEN;
		return (FAILURE);
	}

	yyin = fp;

	while (t=yylex())
	{
		if (t != IDENT)
		{
			graferrno = GEMONPARSE;
			fclose (fp);
			return (FAILURE);
		}
		strcpy (id, yylval.string);

		t = yylex ();
		if (t != EQUAL)
		{
			graferrno = GEMONPARSE;
			fclose (fp);
			return (FAILURE);
		}

		MonFixId (id);

		t = yylex ();
		if (t == NUMBER)
		{
			if (!AddInt (g, id, yylval.number))
			{
				fclose (fp);
				return (FAILURE);
			}
		}
		else if (t == STRING)
		{
			if (!AddString (g, id, yylval.string))
			{
				fclose (fp);
				return (FAILURE);
			}
		}

		t = yylex ();
		if (t != SEMICOLON)
		{
			graferrno = GEMONPARSE;
			fclose (fp);
			return (FAILURE);
		}

	}

	fclose (fp);
	Xfree (filename);
	return (SUCCESS);
}

MonFixId (s)
char *s;
{
	char tmp[256];

	if (strncmp(s, MON_PREFIX, MON_PREFIX_LEN) != 0) {
		strcpy (tmp, s);
		sprintf (s, "%s%s", MON_PREFIX, tmp);
	}
}
