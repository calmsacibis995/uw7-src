#ifndef NOIDENT
#ident	"@(#)dtadmin:isv/dtfilter.c	1.1"
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
#define DEFAULTDIR_STR	"%s/desktop/PrintMgr/Filters/%s"

enum {
    False, True,
};

typedef enum {
    Ok_RC, Usage_RC, Format_RC, Exists_RC, Access_RC,
} RCodes;

typedef char	Boolean;

char	*AppName;
char	*Home;

char	*Input_Access_Error =
    "dtfilter:1" FS "Unable to open '%s' for reading\n";
char	*Output_Access_Error =
    "dtfilter:2" FS "Unable to open '%s' for writing\n";
char	*Format_Error =
    "dtfilter:3" FS "Option '%s' has an invalid format\n";
char	*Label_Error =
    "dtfilter:4" FS "Label for option '%s' must be of the form "
    "'default string^catalog:index'\n";
char	*Substitution_Error =
    "dtfilter:5" FS "Unknown substitution type in option '%s'\n";
char	*Substition_Cnt_Error =
    "dtfilter:6" FS "Option '%s' has too many substitutions\n";
char	*Exists_Error =
    "dtfilter:7" FS "Options for filter '%s' already exists\n";
char	*Delete_Error =
    "dtfilter:8" FS "Can not delete filter '%s'\n";
char	*Non_Exist_Error =
    "dtfilter:9" FS "Filter '%s' does not exist\n";
char	*Usage_Error =
    "dtfilter:10" FS "Usage:\n"
    "    %s -a [-o] file [file ...]     # add filter options\n"
    "    %s -d name [name ...]          # delete filter options\n";

extern RCodes	Add (char *name, Boolean overwrite);
extern RCodes	Delete (char *name);
extern RCodes	CheckOptions (FILE *infile, FILE *outfile);
extern void	Usage (char *name);
extern void	Error (char *msg, char *info);
extern char	*GetStr (char *msg);

/* main
 *
 * Options are:
 *	-a	Add filter options (mutually exclusive with -d)
 *	-d	Delete filter options (mutually exclusive with -a)
 *	-o	Overwrite existing filter file (ignored if -a not specified)
 *
 * Options must include one of -a or -d.
 */
main (int argc, char **argv)
{
    int		c;
    Boolean	addflg;
    Boolean	delflg;
    Boolean	overwriteflg;
    RCodes	exitrc;
    RCodes	rc;
    extern char	*optarg;
    extern int	optind;

    AppName = argv [0];

    if (!(Home = getenv ("XWINHOME")))
	Home = "/usr/X";

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

    exitrc = 0;
    for ( ; optind<argc; optind++)
    {
	if (addflg)
	    rc = Add (argv [optind], overwriteflg);
	else
	    rc = Delete (argv [optind]);

	if (exitrc < rc)
	    exitrc = rc;
    }

    exit (exitrc);
}	/* End of main () */

/* Add
 *
 * Add new filter options.
 */
RCodes
Add (char *name, Boolean overwrite)
{
    FILE	*infile;
    FILE	*outfile;
    char	*filter;
    char	*slash;
    RCodes	rc;
    char	buf [256];

    /* Find the base name of the filter. */
    slash = strrchr (name, '/');
    if (!slash)
	filter = name;
    else
	filter = slash + 1;
    
    /* Open the input file for reading and the filter file for writing. */
    if (!(infile = fopen (name, "r")))
    {
	Error (Input_Access_Error, name);
	return (Access_RC);
    }

    sprintf (buf, DEFAULTDIR_STR, Home, filter);
    if (access (buf, F_OK) == 0 && !overwrite)
    {
	Error (Exists_Error, filter);
	return (Exists_RC);
    }

    if (!(outfile = fopen (buf, "w")))
    {
	Error (Output_Access_Error, buf);
	fclose (infile);
	return (Access_RC);
    }

    rc = CheckOptions (infile, outfile);

    fclose (infile);
    fclose (outfile);

    return (rc);
}	/* End of Add () */

/* CheckOptions
 *
 * Read the options from the description file, check them for correct format,
 * and write them to the output file.
 */
RCodes
CheckOptions (FILE *infile, FILE *outfile)
{
    int		cnt;
    RCodes	rc;
    char	*tag;
    char	*label;
    char	*catalog;
    char	*pattern;
    char	*origpattern;
    char	*help;
    char	*caret;
    char	*colon;
    char	*percent;
    char	*nextPercent;
    long	val;
    char	*ptr;
    char	buf [1024];

    rc = Ok_RC;
    while (fgets (buf, 1024, infile))
    {
	if (buf [0] == '\n')
	{
	    /* Ignore blank lines */
	    continue;
	}

	/* First entry is a tag.  Second entry is the label.
	 * Third is option pattern.  Forth (optional) string is help file.
	 */
	if (!(tag = strtok(buf, "\t\n")) || !(label = strtok(NULL, "\t\n")) ||
	    !(pattern = strtok(NULL, "\t\n")))
	{
	    Error (Format_Error, tag);
	    rc = Format_RC;
	    continue;
	}

	help = strtok (NULL, "\t\n");
	origpattern = pattern;

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
		rc = Format_RC;
		continue;
	    }
	}

	/* Determine the type of controls needed from the pattern string. */
	for (percent = strchr (pattern, '%'); 
	     percent && percent [1] == '%';
	     percent = strchr (percent + 2, '%'))
	    ;	/* do nothing */

	if (percent)
	{
	    /* complex options */
	    cnt = 0;
	    do
	    {
		/* Look for more options on this line. */
		for (nextPercent = strchr (percent + 2, '%'); 
		     nextPercent && nextPercent [1] == '%';
		     nextPercent = strchr (nextPercent + 2, '%'))
		    ;	/* do nothing */

		switch (*(percent + 1)) {
		case 'd':
		case 's':
		case 'c':
		case 'u':
		case 'f':
		    if (nextPercent)
		    {
			pattern = percent + 2;
			percent = nextPercent;
		    }
		    else
			pattern = 0;
		    break;

		default:
		    /* Unknown control.  Ignore the option. */
		    Error (Substitution_Error, tag);
		    rc = Format_RC;
		    break;
		}

		if (*pattern)
		{
		    /* Arbitrarily allow no more than 5 substitutions in
		     * a pattern.  If more are needed (it's a strange filter
		     * if they are), then users will have to enter them via
		     * miscellaneous options.
		     */
		    if (++cnt >= 5)
		    {
			Error (Substition_Cnt_Error, tag);
			rc = Format_RC;
			break;  /* ignore remainder of pattern */
		    }
		}
		else
		    cnt = 0;
	    } while (*pattern);
	}

	/* Write the option to the output file. */
	fprintf (outfile, "%s\t%s\t%s\t%s\n", tag, label, origpattern, help);
    }

    return (rc);
}	/* End of CheckOptions () */

/* Delete
 *
 * Delete a filter options file.
 */
RCodes
Delete (char *name)
{
    char	filter [256];

    sprintf (filter, DEFAULTDIR_STR, Home, name);

    if (access (filter, F_OK))
    {
	Error (Non_Exist_Error, name);
	return (Access_RC);
    }

    if (unlink (filter))
    {
	Error (Delete_Error, name);
	return (Access_RC);
    }

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
