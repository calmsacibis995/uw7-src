#pragma ident	"@(#)dtm:h_file.c	1.56"

/******************************file*header********************************

    Description:
     This file contains the source code to read in and close a help file.
	It also contains code to parse a help file.
*/
                              /* #includes go here     */

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>

#include <MGizmo/Gizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Private Procedures
          2. Public  Procedures
*/


/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/* number of additional section record to allocate */
#define SECTION_STEP         16


/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
 * This function parses lines '^*name^value'.
 */
static int
Dm__GetOption(DmHelpFilePtr hfp, DmMapfilePtr mp)
{
	static int lineno = 0;
	char       *name;
	char       *value;

	if (Dm__GetNameValue(mp, &name, &value))
		return(1);

	if (!strcmp(name, "version")) {
		/*
		 * Only one version is supported in release 1.
		 * Error handling will be implemented when more than
		 * one version is supported in future releases.
		 * For now, a warning is displayed if version != 1.
		 */
		if (value) {
			hfp->version = atoi(value);
			if (hfp->version == 0) {
				/* reserved */
			} else if (hfp->version == 1) {
				/* standard format */
			} else {
				/* ignore for this release */
			}
			free(value);
		}
	} else if (!strcmp(name, "width")) {
		if (value) {
			hfp->width = atoi(value);
			if (hfp->width == 0)
				/* reset to default */
				hfp->width = HELP_WINDOW_NUM_COLS;
			free(value);
		}
	} else if (!strcmp(name, "title")) {
		/* note that allocated space for value is used as title here */
		if (hfp->title)
			free(hfp->title);
		if (value)
			hfp->title = value;
	} else
		Dm__VaPrintMsg(TXT_HELP_BAD_OPTION, name, lineno);
	++lineno;
	free(name);
	return(0);
}

/****************************procedure*header*****************************
 * Parses a section header and saves the body of a section in
 * sect.raw_data to be "cooked" by DmProcessSection().
 * has_header is False for section "Unknown".
 */
static int
Dm__GetSection(DmHelpFilePtr hfp, DmMapfilePtr mp, Boolean has_header)
{
	DmHelpSectPtr hsp;
	DmHelpSectRec sect;
	char *start;
	char *p;
	int  c;

	/* initialize keyword and defs ptr and count */
	memset(&sect, 0, sizeof(DmHelpSectRec));

	if (has_header) {
		char *name;

		start = MF_GETPTR(mp);

		/* The first line of a section header is of the following format:
		 * ^level^section_name=alias
		 *
		 * alias is optional.  If it is specified, it will be used to
		 * look up a link to a section; otherwise, section_name is used.
		 * If there is no alias, the '=' after section name is not required.
		 *
		 * In both cases, section_name is always used for display
		 * in the Table Of Contents.
		 *
		 * In addition, if level is 0, then don't display level of
		 * the section in Table Of Contents.  The section name is
		 * to be displayed as sort of the main topic heading.
		 */ 

		if (p = Dm__findchar(mp, '^')) {
			name = (char *)strndup(start, p - start);
			MF_NEXTC(mp); /* skip '^' */
			sect.level = atoi(name);

			start = ++p;

			/* find the end of line and parse the rest of
			 * the line for section name and alias.
			 */
			if ((p = Dm__findchar(mp, '\n')) != NULL) {
				char	*s;
				char	*t;

				s = (char *)strndup(start, p - start);
				if ((t = strchr(s, '=')) != NULL) {
					/* get section name and alias;
					 * there may be no alias.
					 */
					sect.name = (char *)strndup(s, t - s);
					sect.alias = strdup(++t);
				} else {
					sect.name = strdup(s);
					sect.alias = NULL;
				}
				free(s);

				/* section name must be specified for each section */
				if (sect.name[0] == '\0') {
					FREE(name);
					if (sect.name)
						FREE(sect.name);
					if (sect.alias)
						FREE(sect.alias);
					return(1);
				}
				MF_NEXTC(mp);  /* skip '\n' */
			} else {
				FREE(name);
				return(1);
			}
		}
	}
	else {
		/* assume section named 'Top' */
		sect.level = 1;
		sect.name  = strdup(DEFAULT_SECTION_NAME);
		sect.alias = NULL;
		sect.tag   = NULL;
	}

	/* add it to the section */
	if (hfp->count == hfp->num_sections) {

		hfp->count += SECTION_STEP;
		hsp = realloc(hfp->sections, hfp->count*sizeof(DmHelpSectRec));
		if (hsp == NULL) {
			hfp->count -= SECTION_STEP;
			if (sect.name)
				free(sect.name);
			if (sect.alias)
				free(sect.alias);
			return(1);
		}
		hfp->sections = hsp;
	}

	/* If there's no section header, skip this and go on to extract
	 * the section body.  If the first non-white space character is
	 * not a '^', we assume there's no section header.
	 */

	/* Find first non-white space character. */
	while (MF_PEEKC(mp) == ' ' || MF_PEEKC(mp) == '\t' ||
		  MF_PEEKC(mp) == '\n')
		MF_NEXTC(mp);

	if (MF_PEEKC(mp) != '^')
		goto extract;

	/* parse section header for section tag and local definitions and
	 * links, all of which are optional.
	 */
	while (MF_PEEKC(mp) == '^') {
		MF_NEXTC(mp); /* skip '^' */
		switch(c = MF_PEEKC(mp)) {
			case '$':
				/* section tag */
				MF_NEXTC(mp); /* skip '*' */
				start = MF_GETPTR(mp);
				p = Dm__findchar(mp, '\n');
				sect.tag = (char *)strndup(start, p - start);
				MF_NEXTC(mp); /* skip '\n' */
				break;
			case '%':
				/* keyword */
				MF_NEXTC(mp); /* skip '%' */
				DmGetKeyword(mp, &(sect.keywords));
				break;
			case '=':
				/* definition */
				MF_NEXTC(mp); /* skip '=' */
				DmGetDefinition(mp, &(sect.defs));
				break;
			default:
				/* ignore all other options and find end of line */
				(void)Dm__findchar(mp, '\n');
				MF_NEXTC(mp); /* skip '\n' */
				break;
		}
	}

extract:

	/* save the beginning of section */
	start = MF_GETPTR(mp);

	/* find the end of a section (clue is "\n^[0-9]") */
	while (p = Dm__strstr(mp, "\n^")) {
		MF_NEXTC(mp); /* skip '\n' */
		MF_NEXTC(mp); /* skip '^' */
		if (isdigit(MF_PEEKC(mp))) {
			mp->curptr--; /* move ptr back to '^' */
			break;
		}
	}

	if (!p) {
		if (MF_EOF(mp))
			p = MF_EOFPTR(mp);
	}

	/* initialize the section structure */
	sect.raw_data       = start;
	sect.raw_size       = p - start;
	sect.attrs          = 0;
	sect.cooked_data    = NULL;
	sect.notes_chged    = False;

	hfp->sections[hfp->num_sections] = sect;
	hfp->num_sections++;
	return(0);

} /* end of Dm__GetSection */

/****************************procedure*header*****************************
 * This function maps a helpfile and scan through the help file to
 * generate a list of sections.
 *
 * Note that mapped helpfiles are put into the cache, so that in the
 * future, if we have multiple windows viewing the same file, extra
 * processing is avoided.
 *
 * One possible enhancement to this is that we can scan the mapped file up
 * to the section that is needed. Then as sections are needed, then
 * the file is scanned further (something like "demand scanning").
 */
DmHelpFilePtr
DmOpenHelpFile(DmHelpAppPtr hap, char *filename)
{
     char c;
     char *fullpath;
     int len;
     int first = 1;
     DmHelpFilePtr hfp;
     DmMapfilePtr mp;

	if ((fullpath = DmGetFullPath(filename, hap->help_dir)) == NULL)
		return(NULL);
     len = strlen(fullpath) + 1;

	/* check the cache first */
	if (hfp = DtGetData(NULL, DM_CACHE_HELPFILE, (void *)fullpath, len)) {
		hfp->count++;
		free(fullpath);
		return(hfp);
	}

	/* create a new structure */
	if ((hfp = (DmHelpFilePtr)CALLOC(1, sizeof(DmHelpFileRec))) == NULL)
		return(NULL);

	/* map the file for reading */
	if (!(hfp->mp = Dm__mapfile(fullpath, PROT_READ, MAP_SHARED))) {
		free(hfp);
		if (fullpath)
			free(fullpath);
		return(NULL);
	}

	/* set the defaults */
	hfp->width = HELP_WINDOW_NUM_COLS;
	/* store full pathname */
	hfp->name  = fullpath;

	/*
	 * During the following loop, count is used to keep track
	 * of the # of allocated slots in hfp->sections.
	 * Count will be set to 1 near the bottom of this function.
	 */
	hfp->count = 0;

	/* scan the file for sections */
	mp = hfp->mp;
	while (MF_NOT_EOF(mp)) {
		if (MF_PEEKC(mp) == '#') {
			Dm__findchar(mp, '\n');
			MF_NEXTC(mp); /* skip #ident string */
		} else if (MF_PEEKC(mp) == '\n') {
			MF_NEXTC(mp); /* skip blank line */
		} else if (MF_PEEKC(mp) == '^') {
			MF_NEXTC(mp); /* skip '^' */
			switch(c = MF_PEEKC(mp)) {
			case '?':
			case ':':
				/* Skip icon label and one-line description;
				 * read by DmGetHDAppInfo() in DmInitHelpDesk()
				 * or when a DT_ADD_TO_HELPDESK is received.
				 */
				Dm__findchar(mp, '\n');
				MF_NEXTC(mp);
				break;
			case '*':
				/* global option */
				MF_NEXTC(mp);
				Dm__GetOption(hfp, mp);
				break;
			case '%':
				/* keyword */
				MF_NEXTC(mp);
				DmGetKeyword(mp, &(hfp->keywords));
				break;
			case '=':
				/* definition */
				MF_NEXTC(mp);
				DmGetDefinition(mp, &(hfp->defs));
				break;
			case '+':
				/* name of definition file */
				MF_NEXTC(mp);
				DmReadDefFile(mp, &(hfp->defs), hap->help_dir);
				break;
			default:
				if (isdigit(c)) {
					/* beginning of a section */
					if (Dm__GetSection(hfp, mp, True)) {
						DmCloseHelpFile(hfp);
						return(NULL);
					} else {
						first = 0;
					}
				}
				else {
					/* invalid format */
					Dm__VaPrintMsg(TXT_HELPFILE_SYNTAX_ERR, hfp->name,
						MF_LINENO(mp));
					DmCloseHelpFile(hfp);
					return(NULL);
				}
			}
		}
		else {
			if (first) {
				/* create a default section called 'Unknown' */
				if (Dm__GetSection(hfp, mp, False)) {
					DmCloseHelpFile(hfp);
					return(NULL);
				} else {
					first = 0;
				}
			}
			else {
				/* invalid format */
				Dm__VaPrintMsg(TXT_HELPFILE_SYNTAX_ERR, hfp->name,
					MF_LINENO(mp));
				DmCloseHelpFile(hfp);
				return(NULL);
			}
		}
	}

	/* compress the allocated space for sections array */
	if (hfp->count > (int)(hfp->num_sections)) {
		/* This realloc() call should not failed. */
		hfp->sections = realloc(hfp->sections, hfp->num_sections *
						sizeof(DmHelpSectRec));
	}

	hfp->count = 1;
	hfp->notesp = NULL;

	/* put the structure into cache */
	DtPutData(NULL, DM_CACHE_HELPFILE, hfp->name, len, hfp);

	return(hfp);

} /* end of DmOpenHelpFile */

/****************************procedure*header*****************************
	Closes a help file.
 */
void
DmCloseHelpFile(register DmHelpFilePtr hfp)
{
	/* decrement usage count, and remove from cache if count reaches 0 */
	if (--(hfp->count) == 0) {

		/* remove it from cache */
		DtDelData(NULL, DM_CACHE_HELPFILE, (void *)(hfp->name),
			  strlen(hfp->name) + 1);

		/* unmap file */
		if (hfp->mp)
			Dm__unmapfile(hfp->mp);

		/* free section resources */
		DmFreeAllHelpSections(hfp->sections, hfp->num_sections);

		/* free file resources */
		DtFreePropertyList(&(hfp->keywords));
		DtFreePropertyList(&(hfp->defs));

		if (hfp->title)
			free(hfp->title);
		if (hfp->name)
			free(hfp->name);
		if (hfp->notesp) {
			DmHelpNotesPtr	np;
			DmHelpNotesPtr tnp;

			np = hfp->notesp;
			while (np) {
				if (np->sect_name)
					free(np->sect_name);
				if (np->sect_tag)
					free(np->sect_tag);
				if (np->notes_file)
					free(np->notes_file);
				tnp = np->next;
				free(np);
				np = tnp;
			}
		}
		free(hfp);
	}
} /* end of DmCloseHelpFile */

/****************************procedure*header*****************************
 * This function returns a pointer to a section given a section tag, name
 * or section alias.
 */
DmHelpSectPtr
DmGetSection(DmHelpFilePtr hfp, char *tag)
{
	register DmHelpSectPtr hsp;
	register int i;

	for (hsp=hfp->sections, i=hfp->num_sections; i; i--, hsp++)
		if ((hsp->tag && !strcmp(hsp->tag, tag))
			|| (hsp->name && !strcmp(hsp->name, tag))
			|| (hsp->alias && !strcmp(hsp->alias, tag)))
			return(hsp);

	return(NULL);
} /* end of DmGetSection */
