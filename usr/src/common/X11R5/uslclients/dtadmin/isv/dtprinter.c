#ifndef NOIDENT
#ident	"@(#)dtadmin:isv/dtprinter.c	1.1"
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
#define PRINTER_FILE	"desktop/PrintMgr/Printers"

enum {
    False, True,
};

typedef enum {
    Ok_RC, Usage_RC, Format_RC, Exists_RC, Access_RC, Mem_RC,
} RCodes;

typedef struct _Item {
    char		*tag;
    char		*desc;
    char		*terminfo;
    char		*interface;
    char		*contents;
    char		*stty;
    char		*modules;
    struct _Item	*next;
} Item;

typedef char	Boolean;

#define WHITESPACE	" \t\n"

typedef Boolean (*pfb)();

typedef struct {
    char	*key;
    pfb		proc;
} KeyWord;

char	*AppName;
char	*NoTag = "<?>";

char	*Usage_Error =
    "dtprinter:1" FS "Usage:\n"
    "    %s -a [-o] file [file ...]     # add items\n"
    "    %s -d name [name ...]          # delete items\n";
char	*Input_Access_Error =
    "dtprinter:2" FS "Unable to open '%s' for reading\n";
char	*Output_Access_Error =
    "dtprinter:3" FS "Unable to open '%s' for writing\n";
char	*Format_Error =
    "dtprinter:4" FS "Item '%s' has an invalid format\n";
char	*Label_Error =
    "dtprinter:5" FS "Name for item '%s' must be of the form "
    "'default string^catalog:index'\n";
char	*Exists_Error =
    "dtprinter:6" FS "Item '%s' already exists\n";
char	*Non_Exist_Error =
    "dtprinter:7" FS "Item '%s' does not exist\n";
char	*Mem_Error =
    "dtprinter:8" FS "No memory!\n";
char	*Keyword_Error =
    "dtprinter:9" FS "Keyword '%s' is unknown\n";
char	*Unrecognized_Line_Error =
    "dtprinter:10" FS "Line beginning with '%s' is invalid\n";
char	*Dup_Name_Error =
    "dtprinter:11" FS "Name line already found for entry '%s'\n";
char	*Dup_Terminfo_Error =
    "dtprinter:12" FS "Terminfo line already found for entry '%s'\n";
char	*Dup_Content_Error =
    "dtprinter:13" FS "Contents line already found for entry '%s'\n";
char	*Dup_Interface_Error =
    "dtprinter:14" FS "Interface line already found for entry '%s'\n";
char	*Dup_Entry_Error =
    "dtprinter:15" FS "Entry line already found for entry '%s'\n";
char	*Dup_Modules_Error =
    "dtprinter:16" FS "Modules line already found for entry '%s'\n";
char	*Dup_Stty_Error =
    "dtprinter:17" FS "Stty line already found for entry '%s'\n";
char	*Excess_Error =
    "dtprinter:18" FS "Extra characters found after value of '%s'";
char	*No_Tag_Error =
    "dtprinter:19" FS "The 'entry' line is required for all printers\n";
char	*No_Name_Error =
    "dtprinter:20" FS "A 'name' line is required for printer '%s'\n";

extern RCodes	Add (char *name, Boolean overwrite, Item **pList);
extern RCodes	Delete (char *name, Item **pList);
extern RCodes	CheckItems (FILE *infile, Item **pList, Boolean overwrite);
extern RCodes	AddItem (Item *item, Item **pList, Boolean overwrite);
extern void	Usage (char *name);
extern void	Error (char *msg, char *info);
extern char	*GetStr (char *msg);
extern Item	*ReadList (char *file);
extern void	WriteList (char *file, Item *item);
extern void	Save (FILE *outfile, Item *list);
extern Item	*Lookup (char *tag, Item **pList, Boolean remove);

static Boolean	GetName (Item *, char *);
static Boolean	GetTerminfo (Item *, char *);
static Boolean	GetContent (Item *, char *);
static Boolean	GetInterface (Item *, char *);
static Boolean	GetModules (Item *, char *);
static Boolean	GetStty (Item *, char *);
static Boolean	GetTag (Item *, char *);

static char	*GetList (char *);
static char	*GetWord (char *value, char *tag);
static pfb	KeySearch (char *);
static int	KeywordCmp (const void *, const void *);
static void	RemoveSpaces (char *);
static Boolean	ErrorChk (Item *);

static KeyWord	keywords [] = {
    { "contents", GetContent, },
    { "entry", GetTag },
    { "interface", GetInterface, },
    { "modules", GetModules, },
    { "name", GetName, },
    { "stty", GetStty, },
    { "terminfo", GetTerminfo, },
};

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
    char	*file;
    char	*home;
    RCodes	exitrc;
    RCodes	rc;
    extern char	*optarg;
    extern int	optind;

    AppName = argv [0];

    if (!(home = getenv ("XWINHOME")))
	home = "/usr/X";

    if (!(file = malloc (strlen (home) + strlen (PRINTER_FILE) + 2)))
    {
	Error (Mem_Error, "");
	exit (Mem_RC);
    }

    sprintf (file, "%s/%s", home, PRINTER_FILE);

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
 *
 * The file consists of zero or more printer descriptions separated by blank
 * lines or comments.  Each description is one or more lines of the form:
 *
 *		keyword: value
 *
 * Keywords are:
 *	entry -- entry name
 *	name -- name of printer to appear in list
 *	terminfo -- comma or space separated list of terminfo names
 *	contents -- comma or space separated list of content types
 *	interface -- path of interface program.  If the path is not absolute,
 *			it is relative to the model directory of lp.
 *	stty -- list of stty args to be used by default for serial printers.
 *	modules -- list of streams modules needed for the printer.
 *
 * String values (e.g. name) are not enclosed in quotes.  Leading and
 * trailing white space is stripped automatically.  Keywords in the input
 * file are converted to lower case before comparison.  The file may include
 * comments; comments are introduced by a # and extend to the end of line.
 * A keyword can be listed only once per printer.
 */
RCodes
CheckItems (FILE *infile, Item **pList, Boolean overwrite)
{
    Item	*newItem;
    char	*keyword;
    char	*value;
    Boolean	(*proc)();
    RCodes	rc;
    char	buf [256];

    rc = Ok_RC;
    newItem = (Item *) 0;
    while (fgets (buf, 256, infile))
    {
	/* remove comments and leading white space and trailing newline */
	keyword = strchr (buf, '#');
	if (keyword)
	    *keyword = 0;
	keyword = buf + strlen (buf) - 1;
	if (keyword >= buf && *keyword == '\n')
	    *keyword = 0;
	keyword = buf + strspn (buf, WHITESPACE);

	/* process the keyword */
	value = strchr (keyword, ':');
	if (value)
	{
	    *value++ = 0;
	    proc = KeySearch (keyword);

	    /* If proc is NULL, there is an error in the input file. */
	    if (proc)
	    {
		if (!newItem)
		{
		    newItem = (Item *) calloc (1, sizeof(*newItem));
		    newItem->tag = NoTag;
		}

		if (!(*proc) (newItem, value + strspn (value, WHITESPACE)))
		    rc = Format_RC;
	    }
	    else
	    {
		Error (Keyword_Error, keyword);
		rc = Format_RC;
	    }
	}
	else
	{
	    /* Line is either blank or is invalid.  Check previous printer
	     * for errors.
	     */
	    if (buf [0])
	    {
		Error (Unrecognized_Line_Error, keyword);
		rc = Format_RC;
	    }

	    if (newItem)
	    {
		if (ErrorChk (newItem))
		    AddItem (newItem, pList, overwrite);
		else
		{
		    /* Error found--remove the item */
		    free (newItem);
		    rc = Format_RC;
		}
	    }

	    newItem = (Item *) 0;
	}
    }

    /* End of file.  Check the printer for errors. */
    if (newItem)
    {
	if (ErrorChk (newItem))
	    AddItem (newItem, pList, overwrite);
	else
	{
	    /* Error found--remove the item */
	    free (newItem);
	    rc = Format_RC;
	}
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

    return (Ok_RC);
}	/* End of Delete () */

/* AddItem
 *
 * Add a item to the list of printers, checking for duplicates.
 */
RCodes
AddItem (Item *item, Item **pList, Boolean overwrite)
{
    Item	*oItem;

    oItem = Lookup (item->tag, pList, False);
    if (oItem)
    {
	if (!overwrite)
	{
	    Error (Exists_Error, item->tag);
	    return (Exists_RC);
	}
	else
	{
	    free (oItem->desc);
	    if (oItem->terminfo)
		free (oItem->terminfo);
	    if (oItem->interface)
		free (oItem->interface);
	    if (oItem->contents)
		free (oItem->contents);
	    if (oItem->stty)
		free (oItem->stty);
	    if (oItem->modules)
		free (oItem->modules);

	    item->next = oItem->next;
	    *oItem = *item;
	}
    }
    else
    {
	item->next = *pList;
	*pList = item;
    }
}	/* End of AddItem () */

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

    fprintf (outfile, "entry: %s\nname: %s\n", list->tag, list->desc);
    if (list->terminfo)
	fprintf (outfile, "terminfo: %s\n", list->terminfo);
    if (list->interface)
	fprintf (outfile, "interface: %s\n", list->interface);
    if (list->contents)
	fprintf (outfile, "contents: %s\n", list->contents);
    if (list->stty)
	fprintf (outfile, "stty: %s\n", list->stty);
    if (list->modules)
	fprintf (outfile, "modules: %s\n", list->modules);
    fprintf (outfile, "\n");
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

/* GetName
 *
 * Get the printer name to appear in the list
 */
static Boolean
GetName (Item *item, char *value)
{
    char	*desc;
    char	*caret;
    char	*colon;
    char	*ptr;
    long	val;

    if (item->desc)
    {
	Error (Dup_Name_Error, item->tag);
	return (False);
    }

    caret = strchr (value, '^');
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
	    Error (Label_Error, item->tag);
	    return (False);
	}
    }

    item->desc = strdup (value);
    return (True);
}	/* End of GetName () */

/* GetTerminfo
 *
 * Get terminfo list--a comma or space separated list of terminfo names
 */
static Boolean
GetTerminfo (Item *item, char *value)
{
    if (item->terminfo)
    {
	Error (Dup_Terminfo_Error, item->tag);
	return (False);
    }

    item->terminfo = GetList (value);
    return (True);
} /* End of GetTerminfo () */

/* GetContent
 *
 * Get Content type list--a comma or space separated list of input types.
 * Allow a null list to be created by specifying the "content:" keyword with
 * no list.
 */
static Boolean
GetContent (Item *item, char *value)
{
    if (item->contents)
    {
	Error (Dup_Content_Error, item->tag);
	return (False);
    }

    item->contents = GetList (value);
    return (True);
} /* End of GetContent () */

/* GetInterface
 *
 * Get the interface program name.  Paths are relative to the lp model
 * directory.  Characters after the path name are ignored.
 */
static Boolean
GetInterface (Item *item, char *value)
{
    if (item->interface)
    {
	Error (Dup_Interface_Error, item->tag);
	return (False);
    }
    
    item->interface = GetWord (value, item->tag);
    return (True);
} /* End of GetInterface () */

/* GetTag
 *
 * Get the interface program name.  Paths are relative to the lp model
 * directory.  Characters after the path name are ignored.
 */
static Boolean
GetTag (Item *item, char *value)
{
    if (item->tag != NoTag)
    {
	Error (Dup_Entry_Error, item->tag);
	return (False);
    }
    
    item->tag = GetWord (value, item->tag);
    if (!*item->tag)
    {
	free (item->tag);
	item->tag = NoTag;
    }

    return (True);
} /* End of GetTag () */

/* GetModules
 *
 * Get modules list--a comma or space separated list of streams modules names.
 */
static Boolean
GetModules (Item *item, char *value)
{
    if (item->modules)
    {
	Error (Dup_Modules_Error, item->tag);
	return (False);
    }

    item->modules = GetList (value);
    return (True);
} /* End of GetModules () */

/* GetStty
 *
 * Get stty setting--this is a string of space separated values (but not made
 * into a list).  No error checking is done here.
 */
static Boolean
GetStty (Item *item, char *value)
{
    if (item->stty)
    {
	Error (Dup_Stty_Error, item->tag);
	return (False);
    }

    item->stty = strdup (value);
    return (True);
} /* End of GetStty () */

/* RemoveSpaces
 *
 * Remove trailing white space from a string.
 */
static void
RemoveSpaces (char *str)
{
    int		strLen;
    int		spaceCnt;

    /* Leading space is already assumed to be gone. */
    strLen = strcspn (str, WHITESPACE);
    spaceCnt = 0;
    while (str [strLen])
    {
	spaceCnt = strspn (str + strLen, WHITESPACE);
	strLen += spaceCnt;
	strLen += strcspn (str + strLen, WHITESPACE);
    }
    str [strLen - spaceCnt] = 0;
} /* End of RemoveSpaces () */

/* GetList
 *
 * This should be a comma or space separated list.  However, there's not much
 * we can check on that, so just allow anything.
 */
static char *
GetList (char *str)
{
    return (strdup (str));
} /* End of GetList () */

/* GetWord
 *
 * Get a single word value.  Make sure there is nothing left on the line.
 */
static char *
GetWord (char *value, char *tag)
{
    unsigned	ilen;

    /* Look for characters after the end of the interface program. */
    ilen = strcspn (value, WHITESPACE);
    if (ilen < strlen (value))
    {
	Error (Excess_Error, tag);
	value [ilen] = 0;
    }

    return (strdup (value));
} /* End of GetWord () */

/* ErrorChk
 *
 * Check an printer item for missing required fields.  Return True if Ok.
 */
static Boolean
ErrorChk (Item *item)
{
    if (item->tag == NoTag)
    {
	Error (No_Tag_Error, item->tag);
	return False;
    }

    if (!item->desc)
    {
	Error (No_Name_Error, item->tag);
	return False;
    }

    return True;
} /* End of ErrorChk () */

/* Comparison function for keyword list */
static int
KeywordCmp (const void *k1, const void *k2)
{
    return (strcmp (((KeyWord *)k1)->key, ((KeyWord *)k2)->key));
} /* End of KeywordCmp () */

/* KeySearch
 *
 * Use a binary search to find a keyword.  Return the procedure used to
 * process that word.
 */
static pfb
KeySearch (char *word)
{
    char	*pc;
    KeyWord	*keyWord;
    KeyWord	searchWord;

    /* Convert to lower case. */
    for (pc = word; *pc; pc++)
	*pc = tolower (*pc);

    searchWord.key = word;
    keyWord = (KeyWord *) bsearch (&searchWord, keywords,
				   sizeof (keywords) / sizeof (*keywords),
				   sizeof (searchWord), KeywordCmp);
    return (keyWord ? keyWord->proc : (pfb) 0);
} /* End of KeySearch () */
