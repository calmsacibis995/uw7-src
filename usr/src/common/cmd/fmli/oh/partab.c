/*		copyright	"%c%" 	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */

#ident	"@(#)fmli:oh/partab.c	1.7.3.3"

/* Note: this file created with tabstops set to 4.
 *
 * Definition of the Object Parts Table (OPT).
 *
 */

#include <stdio.h>
#include "inc.types.h"		/* abs s14 */
#include "wish.h"
#include "but.h"
#include "typetab.h"
#include "ifuncdefs.h"
#include "optabdefs.h"
#include "partabdefs.h"


/*** NOTE: the ordering of the objects in this table must be the same
 *** as the order in the object operations table (In optab.c), as this table is
 *** used as an index into that table.
 ***/

/* the Object Part Table */
struct opt_entry Partab[MAX_TYPES] =
{
  { "DIRECTORY",":153","File folder",	CL_DIR,  "?", "?", "?", "?", "?", 0, 2},
  { "ASCII",    ":154","Standard file",CL_DOC,	 "?", "?", "?", "?", "?", 2, 1},
  { "MENU",     ":117","Menu",CL_DYN | CL_FMLI,"?", "?", "?", "?", "?", 3, 1},
  { "FORM",     ":107","Form",CL_FMLI, "?", "?", "?", "?", "?", 4, 1},
  { "TEXT",     ":113","Text",CL_FMLI, "?", "?", "?", "?", "?", 5, 1},
  { "EXECUTABLE",":155","Executable",	CL_FMLI, "?", "?", "?", "?", "?", 7, 1},
  { "TRANSFER", ":156","Foreign file",	CL_OEU,  "?", "?", "?", "?", "?", 6, 1},
  { "UNKNOWN",  ":86","Data file",	NOCLASS, "?", "?", "?", "?", "?", 7, 1},
  { "",         NULL,"",  NOCLASS, NULL, NULL, NULL, NULL, NULL, 0, 0}
};

/* the "magic" numbers in the "%.ns" below (2nd field) are based on 
 * a max file name size of 255.
 */
/* the Object Part Name display format table */
struct one_part Parts[MAXPARTS] = 
{
        {"1",	"%.255s", 	PRT_DIR},	/* 0  DIRECTORY */
	{"2",	"%.249s/.pref",	PRT_FILE|PRT_OPT}, /* 1            */
	{"1",	"%.255s", 	PRT_FILE},	/* 2  ASCII     */
	{"1",   "Menu.%.250s", 	PRT_FILE},	/* 3  MENU      */
	{"1",   "Form.%.250s", 	PRT_FILE},	/* 4  FORM      */
	{"1",   "Text.%.250s", 	PRT_FILE},	/* 5  TEXT      */
	{"1",	"%.255s",	PRT_FILE|PRT_BIN}, /* 6  TRANSFER  */
	{"1",	"%.255s", 	PRT_FILE|PRT_BIN}, /* 7  UNKNOWN/EXEC*/
	{"",	"",		0}
};
