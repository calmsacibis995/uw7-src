#ifndef NOIDENT
#ident	"@(#)dtadmin:isv/dtisv.c	1.1"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <sys/types.h>
#include <unistd.h>

#define FS	"\001"
#define TYPES_FILE	"desktop/PrintMgr/Types"
#define MODEM_FILE	"desktop/DialupMgr/Modems"

enum {
    False, True,
};

typedef enum {
    Ok_RC, Usage_RC, Format_RC, Exists_RC, Access_RC, Mem_RC,
} RCodes;

typedef struct _Item {
    char		*tag;
    char		*lbl;
    char		*name;
    struct _Item	*next;
} Item;

typedef char	Boolean;

char	*AppName;

char	*Input_Access_Error =
    "dtisv:1" FS "Unable to open '%s' for reading\n";
char	*Output_Access_Error =
    "dtisv:2" FS "Unable to open '%s' for writing\n";
char	*Format_Error =
    "dtisv:3" FS "Item '%s' has an invalid format\n";
char	*Label_Error =
    "dtisv:4" FS "Label for item '%s' must be of the form "
    "'default string^catalog:index'\n";
char	*Exists_Error =
    "dtisv:5" FS "Item '%s' already exists\n";
char	*Non_Exist_Error =
    "dtisv:6" FS "Item '%s' does not exist\n";
char	*Mem_Error =
    "dtisv:7" FS "No memory!\n";
char	*Pgm_Name_Error =
    "dtisv:8" FS "Program name must be dttypes or dtmodem\n";
char	*Usage_Error =
    "dtisv:9" FS "Usage:\n"
    "    %s -a [-o] file [file ...]     # add items\n"
    "    %s -d name [name ...]          # delete items\n";

extern RCodes	Add (char *name, Boolean overwrite, Item **pList);
extern RCodes	Delete (char *name, Item **pList);
extern RCodes	CheckItems (FILE *infile, Item **pList, Boolean overwrite);
extern void	Usage (char *name);
extern void	Error (char *msg, char *info);
extern char	*GetStr (char *msg);
extern Item	*ReadList (char *file);
extern void	WriteList (char *file, Item *item);
extern void	Save (FILE *outfile, Item *list);
extern Item	*Lookup (char *tag, Item **pList, Boolean remove);

/* main
 *
 * Options are:
 *	-a	Add items (mutually exclusive with -d)
 *	-d	Delete items (mutually exclusive with -a)
 *	-o	Overwrite existing item (ignored with -d)
 *
 * Options must include one of -a or -d.
 */
main (int argc, char **argv)
{
    int		c;
    Boolean	addflg;
    Boolean	delflg;
    Boolean	overwriteflg;
    Item	*list;
    char	*pgmName;
    char	*file;
    char	*path;
    char	*home;
    RCodes	exitrc;
    RCodes	rc;
    extern char	*optarg;
    extern int	optind;

    /* This application can be called by either of two names to set up
     * input types for printing or modems for dialup.  Use argv [0] to
     * determine which case we're in.
     */
    AppName = argv [0];
    pgmName = strrchr (AppName, '/');
    pgmName = (!pgmName) ? AppName : pgmName + 1;

    if (!(home = getenv ("XWINHOME")))
	home = "/usr/X";

    if (strcmp (pgmName, "dttypes") == 0)
	path = TYPES_FILE;
    else if (strcmp (pgmName, "dtmodem") == 0)
	path = MODEM_FILE;
    else
    {
	Error (Pgm_Name_Error, "");
	exit (Usage_RC);
    }

    if (!(file = malloc (strlen (home) + strlen (path) + 2)))
    {
	Error (Mem_Error, "");
	exit (Mem_RC);
    }

    sprintf (file, "%s/%s", home, path);

    /* Parse the options. */
    addflg = delflg = overwriteflg = False;
    while ((c = getopt (argc, argv, "ado")) != EOF)
    {
	switch (c) {
	case 'a':
	    if (delflg)
		Usage (argv [0]);
	    addflg = True;
	    break;

	case 'd':
	    if (addflg)
		Usage (argv [0]);
	    delflg = True;
	    break;

	case 'o':
	    overwriteflg = True;;
	    break;

	default:
	    Usage (argv [0]);
	    break;
	}
    }

    if (optind == argc || (!addflg && !delflg))
	Usage (argv [0]);

    /* Read the existing list */
    list = ReadList (file);

    exitrc = 0;
    for ( ; optind<argc; optind++)
    {
	if (addflg)
	    rc = Add (argv [optind], overwriteflg, &list);
	else
	    rc = Delete (argv [optind], &list);

	if (exitrc < rc)
	    exitrc = rc;
    }

    /* Rewrite the list file */
    WriteList (file, list);

    exit (exitrc);
}	/* End of main () */

/* Add
 *
 * Add new list items.
 */
RCodes
Add (char *name, Boolean overwrite, Item **pList)
{
    FILE	*infile;
    FILE	*outfile;
    char	*filter;
    char	*slash;
    RCodes	rc;
    char	buf [256];

    /* Open the list file for reading. */
    if (!(infile = fopen (name, "r")))
    {
	Error (Input_Access_Error, name);
	return (Access_RC);
    }

    rc = CheckItems (infile, pList, overwrite);

    fclose (infile);
    return (rc);
}	/* End of Add () */

/* CheckItems
 *
 * Read the options from the description file, check them for correct format,
 * and update the list.
 */
RCodes
CheckItems (FILE *infile, Item **pList, Boolean overwrite)
{
    RCodes	rc;
    Item	*item;
    char	*tag;
    char	*label;
    char	*name;
    char	*caret;
    char	*colon;
    char	*ptr;
    long	val;
    char	buf [1024];

    rc = Ok_RC;
    while (fgets (buf, 1024, infile))
    {
	if (buf [0] == '\n')
	{
	    /* Blank line--skip it */
	    continue;
	}

	/* First entry is a tag.  Second entry is the label.
	 * Third is the internal name of the item.
	 */
	if (!(tag = strtok(buf, "\t\n")) || !(label = strtok(NULL, "\t\n")) ||
	    !(name = strtok(NULL, "\t\n")))
	{
	    Error (Format_Error, tag);
	    if (Format_RC > rc)
		rc = Format_RC;
	    continue;
	}

	caret = strchr (label, '^');
	if (caret)
	{
	    /* label contains both a default string and an internationalized
	     * string.  Check the latter for format (but don't check if the
	     * help file exists.
	     */
	    colon = strchr (caret+1, ':');

	    if (!colon || !*(colon+1) ||
		(val = strtol (colon+1, &ptr, 10)) < 0 || *ptr)
	    {
		Error (Label_Error, tag);
		if (Format_RC > rc)
		    rc = Format_RC;
		continue;
	    }
	}

	/* Check if the item already exists */
	item = Lookup (tag, pList, False);
	if (item)
	{
	    if (!overwrite)
	    {
		Error (Exists_Error, tag);
		if (Exists_RC > rc)
		    rc = Exists_RC;
		continue;
	    }
	    free (item->tag);
	    free (item->lbl);
	    free (item->name);
	}
	else
	{
	    /* Make a new entry at the front of the list. */
	    item = (Item *) malloc (sizeof (Item));
	    if (!item)
	    {
		Error (Mem_Error, "");
		exit (Mem_RC);
	    }
	    item->next = *pList;
	    *pList = item;
	}

	item->tag = strdup (tag);
	item->lbl = strdup (label);
	item->name = strdup (name);
    }

    return (rc);
}	/* End of CheckItems () */

/* Delete
 *
 * Delete a filter options file.
 */
RCodes
Delete (char *name, Item **pList)
{
    Item	*item;

    item = Lookup (name, pList, True);
    if (!item)
    {
	Error (Non_Exist_Error, name);
	return (Access_RC);
    }

    free (item->tag);
    free (item->lbl);
    free (item->name);
    free (item);

    return (Ok_RC);
}	/* End of Delete () */

/* Error
 *
 * Write an error message.  All strings are assumed to have a single %s
 * in them for additional information.
 */
void
Error (char *msg, char *info)
{
    if (!info)
	info = "";

    fprintf (stderr, "%s:  ", AppName);
    fprintf (stderr, GetStr (msg), info);
}	/* End of Error () */

/* Usage
 *
 * Display a usage message and die.
 */
void
Usage (char *name)
{
    fprintf (stderr, GetStr (Usage_Error), name, name);
    exit (Usage_RC);
}	/* End of Usage () */

/* GetStr
 *
 * Get an internationalized string.  The string is of the form
 * "file:index\001Default string".
 */
char *
GetStr (char *msg)
{
    char	*sep;
    char	*str;

    sep = strchr (msg, '\001');
    *sep = 0;
    str = gettxt (msg, sep + 1);

    *sep = '\001';
    return (str);
}	/* End of GetStr () */

/* ReadList
 *
 * Read the existing file.  If there are duplicates in the file (shouldn't
 * happen), the last entry wins.
 */
Item *
ReadList (char *file)
{
    Item	*list;
    FILE	*infile;

    /* Open the file for reading.  It is not a crisis if the file doesn't
     * exist.  Assume that the file does not exist on any open error.
     */
    if (!(infile = fopen (file, "r")))
	return ((Item *) 0);

    list = (Item *) 0;
    (void) CheckItems (infile, &list, True);
    fclose (infile);
    return (list);
}	/* End of ReadList () */

/* WriteList
 *
 * ReWrite the list to the output file.  Write the list in back-to-front
 * order.
 */
void
WriteList (char *file, Item *list)
{
    FILE	*outfile;

    if (!(outfile = fopen (file, "w")))
    {
	Error (Output_Access_Error, file);
	exit (Access_RC);
    }

    if (list)
	Save (outfile, list);

    fclose (outfile);
}	/* End of WriteList () */

/* Save
 *
 * Recursively write a list to a file.
 */
void
Save (FILE *outfile, Item *list)
{
    if (list->next)
	Save (outfile, list->next);

    fprintf (outfile, "%s\t%s\t%s\n", list->tag, list->lbl, list->name);
}	/* End of Save () */

/* Lookup
 *
 * Search the list for a item with a given tag.  Optionally unlink this item
 * from the list.
 */
Item *
Lookup (char *tag, Item **pList, Boolean remove)
{
    Item	*item;

    if (!*pList)
	return ((Item *) 0);

    if (strcmp (tag, (*pList)->tag) == 0)
    {
	item = *pList;
	if (remove)
	    *pList = (*pList)->next;
    }
    else
	item = Lookup (tag, &((*pList)->next), remove);

    return (item);
}	/* End of Lookup () */
