#pragma ident	"@(#)dtm:h_sect.c	1.15.1.1"

/******************************file*header********************************

    Description:
     This file contains the source code for "processing" a section
	to be displayed by the hypertext widget and to free resources
	allocated for a section. 
*/
                              /* #includes go here     */

#include <X11/Intrinsic.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Private Procedures
          2. Public  Procedures
*/
                         /* private procedures         */

static void DmFreeHelpSection(DmHelpSectPtr hsp);

                         /* public procedures         */

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

#define NEXTC(P,END)	P = (P < (END)) ? (P+1) : (P)

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
 * This function processes a section from raw data to the format that is
 * readable by the hypertext widget.
 */
int
DmProcessHelpSection(DmHelpSectPtr hsp)
{
	DmMapfileRec map; /* dummy map structure */

	/* simply return if section is already cooked */
	if (hsp->cooked_data)
		return;

	/*
	 * create a dummy map structure, so we can use the generic routines.
	 */
	map.curptr = hsp->raw_data;
	map.endptr = map.curptr + hsp->raw_size;

	/* If displaying Table of Contents, need to parse section header,
	 * For other sections, this is done in Dm__GetSection() in h_file.c.
	 */
	if (strcmp(hsp->name, TABLE_OF_CONTENTS) == 0) {
		while (map.curptr < map.endptr) {
			if (*map.curptr == '^') {
				NEXTC(map.curptr, map.endptr);
				switch(*map.curptr) {
				case '%':
					/* keyword */
					NEXTC(map.curptr, map.endptr);
					DmGetKeyword(&map, &(hsp->keywords));
					break;
				case '=':
					/* definition */
					NEXTC(map.curptr, map.endptr);
					DmGetDefinition(&map, &(hsp->defs));
					break;
				default:
					/* bad format */
					Dm__VaPrintMsg(TXT_HELP_BAD_SECTION, hsp->name);
					map.curptr = Dm__findchar((DmMapfilePtr)&map, '\n');
					NEXTC(map.curptr, map.endptr); /* skip '\n' */
				}
			}
			else
				break;
		}
	}

	if (map.curptr < map.endptr) {
		hsp->cooked_size = map.endptr - map.curptr;
		hsp->cooked_data = (char *)strndup(map.curptr,
			hsp->cooked_size);
	}
	else {
		/* no text in this section */
		hsp->cooked_size = 0;
		hsp->cooked_data = NULL;
	}

	return(0);
} /* end of DmProcessHelpSection */

/****************************procedure*header*****************************
 * This function frees all the resources associated with a section.
 */
static void
DmFreeHelpSection(DmHelpSectPtr hsp)
{
	DtFreePropertyList(&(hsp->keywords));
	DtFreePropertyList(&(hsp->defs));

	if (hsp->name)
		free(hsp->name);

	if (hsp->alias)
		free(hsp->alias);

	if (hsp->tag)
		free(hsp->tag);

	if (hsp->cooked_data)
		free(hsp->cooked_data);
	hsp->name = NULL;
	hsp->alias = NULL;
	hsp->tag = NULL;
	hsp->cooked_data = NULL;
} /* end of DmFreeHelpSection */

/****************************procedure*header*****************************
 * This function frees all the resources associated with sections in a file.
 */
void
DmFreeAllHelpSections(register DmHelpSectPtr hsp, register int count)
{
	register DmHelpSectPtr thsp;

	thsp = hsp;
	for (;count; count--) {
		DmFreeHelpSection(thsp++);
	}
	free(hsp);

} /* end of DmFreeAllHelpSections */
