#ifndef NOIDENT
#ident	"@(#)dtadmin:print/use/prtmgr.c	1.27"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <Intrinsic.h>
#include <StringDefs.h>
#include <Xol/OpenLook.h>

#include <X11/Shell.h>
#include <X11/Xatom.h>

#include <Desktop.h>
#include <DtI.h>

#include "properties.h"
#include "error.h"

#define SET_ALLOC_SIZE	10
#define XNLLANGUAGE "*xnlLanguage:"


extern Printer	*GetPrinter (char *name);
static void	GetLocales (PRINTER *config, ButtonItem **pItems,
			     int *pNumItems);
static void	GetCharSets (PRINTER *config, ButtonItem **pItems,
			     int *pNumItems);
static void	InsertDefaults (char *inputType, char *opts, char *modes,
				Cardinal charSet, Printer *printer);
static void	GetDefaults (Printer *printer);
extern Boolean	SetDefaults (FilterOpts *filterOpts, Printer *printer);
extern void	Die (Widget widget, XtPointer client_data,
		     XtPointer call_data);
extern int	FindCharSet (Printer *, char *);
extern void	ReadInputTypes (ButtonItem **, unsigned *);

static Boolean	DropNotify (Widget w, Window win, Position x, Position y, 
			    Atom selection, Time timestamp, 
			    OlDnDDropSiteID drop_site_id, 
			    OlDnDTriggerOperation op, 
			    Boolean send_done, Boolean forwarded,
			    XtPointer closure);
static void	SelectionCB(Widget w, XtPointer client_data,
			    XtPointer call_data);

extern ButtonItem	*InputTypes;
extern unsigned		NumInputTypes;

XtAppContext	AppContext;
Widget		TopLevel;
static char	*PrtDir;

static XrmOptionDescRec	Options [] =
{
	{ "-p", ".printer", XrmoptionSepArg, 0 },
	{ "-o", ".displayQueue", XrmoptionNoArg, (XtPointer) "True" },
	{ "-T", ".contentType", XrmoptionSepArg, 0 },
};

ResourceRec	AppResources;
XtResource	Resources [] = {
    { "printer", "Printer", XtRString, sizeof (String),
	  XtOffset (ResourcesPtr, printer), XtRString, "", },
    { "defaultPrinter", "DefaultPrinter", XtRString, sizeof (String),
	  XtOffset (ResourcesPtr, dfltPrinter), XtRString, NULL, },
    { "contentType", "ContentType", XtRString, sizeof (String),
	  XtOffset (ResourcesPtr, contentType), XtRString, "simple", },
    { "displayQueue", "DisplayQueue", XtRBoolean, sizeof (Boolean),
	  XtOffset (ResourcesPtr, displayQueue),
	  XtRImmediate, (XtPointer)False, },
    { "jobIcon", "JobIcon", XtRString, sizeof (String),
	  XtOffset (ResourcesPtr, icon), XtRString,
	  "prtreq.icon", },
    { "procIcon", "ProcIcon", XtRString, sizeof (String),
	  XtOffset (ResourcesPtr, procIcon), XtRString,
	  "printer48.icon", },
    { "filterDir", "FilterDir", XtRString, sizeof (String),
	  XtOffset (ResourcesPtr, filterDir), XtRString,
	  "desktop/PrintMgr/Filters", },
    { "typesFile", "TypesFile", XtRString, sizeof (String),
	  XtOffset (ResourcesPtr, typesFile), XtRString,
	  "desktop/PrintMgr/Types", },
    { "checkQInterval", "CheckQInterval", XtRInt, sizeof (int),
	  XtOffset (ResourcesPtr, timeout), XtRImmediate,
	  (XtPointer) 60, },
};

/* The printer use command has several "modes" of operation that are
 * switchable via command line options.
 *
 *	(none) -- default is to assume that a file has been dropped on the
 *		  icon, and we should print it.
 *	-o     -- open window and display print queue.
 *	-p name -- printer name
 */

main (int argc, char **argv)
{
    Printer	*printer;
    Window	owner;
    char	**files;
    Properties	*properties;
    int		cnt;
    static char	*noPrinterMsg;

#ifdef MEMUTIL
    InitializeMemutil ();
#endif	/* MEMUTIL */

    AppName = APP_NAME;
    AppTitle = GetStr (TXT_appName);

    OlToolkitInitialize (&argc, argv, NULL);
    TopLevel = XtAppInitialize(
			&AppContext,		/* app_context_return	*/
			AppName,		/* application_class	*/
			Options,		/* options		*/
			XtNumber (Options),	/* num_options		*/
			&argc,			/* argc_in_out		*/
			argv,			/* argv_in_out		*/
			(String *) NULL,	/* fallback_resources	*/
			(ArgList) NULL,		/* args			*/
			(Cardinal) 0		/* num_args		*/
    );
    DtInitialize (TopLevel);

    XtGetApplicationResources (TopLevel, &AppResources,
			       Resources, XtNumber (Resources), NULL, 0);

    if (AppResources.displayQueue)
    {
	/* Display the printer queue.  Check if another copy of this
	 * application is already running.  If so, send a message to
	 * it to do the work; this version should die.
	 */
	XtVaSetValues (TopLevel,
		XtNmappedWhenManaged,		(XtArgVal) False,
		XtNwidth,			(XtArgVal) 1,
		XtNheight,			(XtArgVal) 1,
		0);

	XtRealizeWidget (TopLevel);

	OlDnDRegisterDDI(TopLevel, OlDnDSitePreviewNone, DropNotify,
			 (OlDnDPMNotifyProc) 0, True, (XtPointer) 0);

	owner = DtSetAppId (XtDisplay (TopLevel), XtWindow (TopLevel),
			    "PrtMgr");
	if (owner != None)
	{
	    static char		*printerList [] = { 0, NULL };

	    printerList [0] = AppResources.printer;

	    if (DtNewDnDTransaction(TopLevel, printerList,
				    DT_B_SEND_EVENT | DT_B_STATIC_LIST,
				    0, 0, CurrentTime, owner, DT_MOVE_OP,
				    NULL, (XtCallbackProc) Die, 0))
	    {
		/* Another PrtMgr is running, and we successfully sent a
		 * message to it.  Wait for death.
		 */
		XtAppMainLoop (AppContext);
	    }
	}

	OpenPrinter (AppResources.printer);
    }
    else
    {
	/* Normal print job.  Create a properties structure and build the
	 * files list.
	 */
	noPrinterMsg = GetStr (TXT_no_Printer2);
	printer = GetPrinter (AppResources.printer);
	if (!printer)
	    ErrorConfirm (TopLevel, noPrinterMsg, Die, (XtPointer) No_Printer);
	else
	{
	    ReadInputTypes (&InputTypes, &NumInputTypes);
	    properties = (Properties *) XtCalloc (1, sizeof (*properties));
	    properties->printer = printer;
	    InitProperties (TopLevel, properties, (PrintJob *) 0);

	    cnt = (argc == 1) ? 1 : argc - 1;
	    files = properties->files =
		(char **) XtMalloc ((cnt+1) * sizeof (char *));
	    if (argc == 1)
		*files++ = strdup ("");
	    else
	    {
		while (--argc)
		{
		    if (strcmp (*++argv, "-") == 0)
			*files++ = strdup ("");
		    else
			*files++ = strdup (*argv);
		}
	    }
	    *files = (char *) 0;

	    /* Set the default title to the file to print */
	    XtFree (properties->title);
	    if (files > properties->files + 1)
	    {
		char	buf [128];

		sprintf (buf, GetStr (TXT_multiFiles),
			 (**properties->files) ? *properties->files :
			     GetStr (TXT_stdinName));
		properties->title = strdup (buf);
	    }
	    else
	    {
		properties->title =
		    strdup ((**properties->files) ? *properties->files :
			    GetStr (TXT_stdinName));
	    }
	    PostProperties (TopLevel, properties);
	}
    }

    XtAppMainLoop (AppContext);
}	/* End of main () */

/* GetPrinter
 *
 * Get the printer structure by name.  If the printer structure doesn't
 * exist, create it.
 */
Printer *
GetPrinter (char *name)
{
    Printer		*prt;
    PRINTER		*config;
    static Printer	*prtList;
    static ButtonItem *SlocaleItems;
    static int 		SnumLocales;
    static Boolean 	first = True;
    

    /* If no name was given, use the default printer. */
    if (!name || !*name)
    {
	name = AppResources.dfltPrinter;
	if (!name || !*name)
	    return ((Printer *) 0);
    }

    for (prt=prtList; prt; prt=prt->next)
    {
	if (strcoll (prt->name, name) == 0)
	    return (prt);
    }

    config = getprinter (name);
    if (!config)
	return ((Printer *) 0);

    /* Printer not found.  Make it. */
    prt = (Printer *) XtCalloc (1, sizeof (*prt));
    prt->name = strdup (name);
    prt->config = (PRINTER *) XtMalloc (sizeof (*config));
    *prt->config = *config;

    if (first)
    {
        first = False;
        GetLocales (config, &SlocaleItems, &SnumLocales);
    }
    prt->localeItems = SlocaleItems;
    prt->numLocales = SnumLocales;
    GetCharSets (config, &prt->charSetItems, &prt->numCharSets);
    GetDefaults (prt);

    prt->iconbox = (Widget) 0;
    prt->timeout = (XtIntervalId) 0;

    prt->next = prtList;
    prtList = prt;
    return (prt);
}	/* End of GetPrinter () */

/* GetCharSets
 *
 * Get the character sets supported by the printer.  Sadly, getprinter
 * does not get the list for us; rather, it gets only the list of aliases
 * defined by the administrator.  Soooo, ask terminfo for the programmable
 * character sets.
 */
static void
GetCharSets (PRINTER *config, ButtonItem **pItems, int *pNumItems)
{
    register		cs;
    register char	*name;
    register char	**list;
    char		*csnm;
    ButtonItem		*items;
    int			cnt;
    int			allocated;
    static Boolean	first = True;
    static char		*defaultLbl;
    extern char		*tparm();

    if (first)
    {
	first = False;

	defaultLbl = GetStr (TXT_default);
    }

    allocated = cnt = 0;
    items = (ButtonItem *) 0;

    /* Get the terminfo list for each printer type */
    if (config->printer_types &&
	strcmp (*(config->printer_types), "unknown") &&
	!config->daisy)
    {
	for (list=config->printer_types; *list; list++)
	{
	    if (tidbit (*list, "csnm", &csnm) != -1 && csnm && *csnm)
	    {
		for (cs=0; cs<=63; cs++)
		{
		    /* HACK -- for reasons unknown, postscript printers and
		     * some others, csnm contains just a Ctrl-D to allow
		     * just about all character sets.  In this case, use
		     * a null list.
		     */
		    if ((name = tparm (csnm, cs)) && *name && *name != '\004')
		    {
			if (cnt >= allocated)
			{
			    allocated += SET_ALLOC_SIZE;
			    items = (ButtonItem *)
				XtRealloc ((char *) items,
					   allocated * sizeof (*items));
			    if (cnt == 0)
				items [cnt++].lbl = (XtArgVal) defaultLbl;
			}
			items [cnt++].lbl = (XtArgVal) strdup (name);
		    }
		    else
			/* Assume that a break in the numbers means
			 * we're done.
			 */
			break;
		}
	    }
	}
    }

    /* Add the alias list.  Use only the alias name. */
    if (config->char_sets)
    {
	for (list=config->char_sets; *list; list++)
	{
	    char	*pEq;

	    if (cnt >= allocated)
	    {
		allocated += SET_ALLOC_SIZE;
		items = (ButtonItem *) XtRealloc ((char *) items,
						  allocated * sizeof (*items));
		if (cnt == 0)
		    items [cnt++].lbl = (XtArgVal) defaultLbl;
	    }

	    if (pEq = strchr (*list, '='))
	    {
		int	len = pEq - *list;
		char	*label;

		label = XtMalloc (len + 1);
		items [cnt++].lbl = (XtArgVal) label;
		strncpy (label, *list, len);
	    }
	    else
		items [cnt++].lbl = (XtArgVal) strdup (*list);
	}
    }

    *pItems = items;
    *pNumItems = cnt;
}	/* End of GetCharSets () */

int
cmplocale(ButtonItem *x, ButtonItem *y)
{
    return strcoll((char *)x->lbl, (char*)y->lbl);
}

static void
GetLocales (PRINTER *config, ButtonItem **pItems, int *pNumItems)
{
    int                 cnt;
    int			indx;
    int			itemCnt;
    ButtonItem          *items;
    static Boolean      first = True;
    static char         *cLocaleLbl;
    static char		*xhome;
    static char 	LocalePath[BUFSIZ];
    register char	**locale_list;
    char		*ConvertLocales(char *, char*);
    
    if (first) {
	first = False;
	cLocaleLbl = GetStr (TXT_cLocale);

	xhome = getenv("XWINHOME");
	if (!xhome)
	    xhome = "/usr/X";
	(void)sprintf(LocalePath, "%s/lib/locale", xhome);
    }

    if (locale_list = get_locales ()) {
        /* Count items in locale_list, including the null item at the end */
	for (cnt=0; locale_list[cnt++]; )
	    ; 

	items = (ButtonItem *) XtMalloc (cnt * sizeof (*items));

	/* Always place the C locale at the top of the list. */
	items[0].lbl = (XtArgVal) cLocaleLbl;
	items[0].userData = (XtArgVal) "C";
	itemCnt = 1;
	for (indx=0; indx<cnt-1; indx++) {

	    /* Ignore directories "PC*", "POSIX", and "88591" */
	    if ((strcmp (locale_list[indx], "C") != 0) &&
		(strcmp (locale_list[indx], "88591") != 0) &&
	        (strncmp (locale_list[indx], "P", 1) != 0)) {

	          items[itemCnt].userData = (XtArgVal) locale_list[indx];

	          /*  Call ConvertLocales() to interpret cryptic locale
		      names.  e.g.  Convert "fr_CA.850" to "French PC 850"
		   */
		  items[itemCnt].lbl = 
		   (XtArgVal) ConvertLocales(LocalePath,locale_list[indx]);
		  itemCnt++;

	    }

	}
	/* If the only item in the list is for C locale, return an empty list.
	 * This forces the locale control to be made insensitive.
	 */
	if (itemCnt > 1) {
	    qsort((void*)items, itemCnt, sizeof(ButtonItem),
		  (int (*)()) cmplocale);
	    *pItems = items;
	    *pNumItems = itemCnt;
	    return;
	}
    }

    *pItems = NULL;
    *pNumItems = 0;

}	/* End of GetLocales () */

/* InsertDefaults
 *
 * Add a defaults record for a printer.  Overwrite an existing entry for the
 * same input type.
 */
static void
InsertDefaults (char *inputType, char *opts, char *modes, Cardinal charSet,
		Printer *printer)
{
    Defaults	*dflts;

    for (dflts=printer->dflts; dflts; dflts=dflts->next)
    {
	if (strcmp (inputType, dflts->inputType) == 0)
	    break;
    }

    if (!dflts)
    {
	dflts = (Defaults *) XtMalloc (sizeof (Defaults));
	dflts->next = printer->dflts;
	printer->dflts = dflts;
	dflts->inputType = strdup (inputType);
    }
    else
    {
	XtFree (dflts->modes);
	XtFree (dflts->opts);
	XtFree (dflts->charSet);
    }

    dflts->modes = strdup (modes);
    dflts->opts = strdup (opts);
    dflts->charSet = strdup ((char *) printer->charSetItems [charSet].lbl);
}	/* End of InsertDefaults () */

/* SetDefaults
 *
 * Set default values for filter and interface options for a given printer
 * and input type.  If the input type/printer combination already has defaults,
 * replace with the new values.  Write all default values for this printer
 * to the file $HOME/.printers/{printer name}.  Return True if the write
 * was successful.
 */
Boolean
SetDefaults (FilterOpts *filterOpts, Printer *printer)
{
    char		path [128];
    FILE		*file;
    Defaults		*dflts;
    struct stat		statbuf;

    InsertDefaults (filterOpts->inputType, filterOpts->opts, filterOpts->modes,
		    filterOpts->charSet, printer);

    /* Check if the printer directory exists.  If not, try to create it. */
    if (stat (PrtDir, &statbuf) != 0)
    {
	if (errno != ENOENT ||
	     mkdir (PrtDir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
		return (False);
    }
    else
    {
	if (!S_ISDIR (statbuf.st_mode))
	    return (False);
    }

    /* Open the defaults file and write it out. */
    sprintf (path, "%s/%s", PrtDir, printer->name);
    if (!(file = fopen (path, "w")))
	return (False);

    for (dflts=printer->dflts; dflts; dflts=dflts->next)
    {
	if (fprintf (file, "%s\t%s\t%s\t%s\n", dflts->inputType, dflts->opts,
		     dflts->modes, dflts->charSet) < 0)
	{
	    fclose (file);
	    return (False);
	}
    }

    fclose (file);
    return (True);
}	/* End of SetDefaults () */

/* GetDefaults
 *
 * Read filter and interface option defaults from a file.  If the defaults
 * file doesn't exist or is unreadable for any reason, assume there are no
 * defaults.  We assume that this program wrote this file, and therefore,
 * we will assume that it is in a friendly format (ie, don't bother to
 * remove leading spaces, because there aren't any).  If this assumption
 * is not true, the defaults will simply be ignored.
 */
static void
GetDefaults (Printer *printer)
{
    char	*home;
    char	*inputType;
    char	*opts;
    char	*modes;
    char	*charSet;
    char	*endField;
    FILE	*file;
    char	buf [1024];

    if (!PrtDir)
    {
	home = getenv ("HOME");
	if (!home)
	    home = "";
	PrtDir = XtMalloc (strlen (home) + 9 + 1);
	sprintf (PrtDir, "%s/.printer", home);
    }

    printer->dflts = (Defaults *) 0;

    sprintf (buf, "%s/%s", PrtDir, printer->name);
    if (!(file = fopen (buf, "r")))
	return;

    while (fgets (buf, 1024, file))
    {
	inputType = buf;
	endField = strchr (buf, '\t');
	if (!endField || endField == inputType)
	    continue;

	*endField++ = 0;
	opts = endField;
	endField = strchr (opts, '\t');
	if (!endField)
	    continue;
	if (endField == opts)
	    opts = "";

	*endField++ = 0;
	modes = endField;
	endField = strchr (modes, '\t');
	if (!endField)
	    continue;
	if (endField == modes)
	    modes = "";

	*endField++ = 0;
	charSet = endField;
	endField = strrchr (charSet, '\n');
	if (endField)
	    *endField = 0;
	if (strlen (charSet) == 0)
	    charSet = "";

	InsertDefaults (inputType, opts, modes, FindCharSet (printer, charSet),
			printer);
    }

    fclose (file);
}	/* End of GetDefaults () */

/* Die
 *
 * Self-explanatory.  client_data is the return code.
 */
void
Die (Widget widget, XtPointer client_data, XtPointer call_data)
{
    exit ((int) client_data);
}	/* End of Die () */

/* SelectionCB
 *
 * Called when the file list is ready after a drag and drop operation.  In
 * this case, file names are actually printers.  Open the print queue for
 * each.  client_data is OlDnDTriggerOperation information; call_data is
 * DtDnDInfoPtr drop information.
 */
static void
SelectionCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    OlDnDTriggerOperation	op  = (OlDnDTriggerOperation) client_data;
    DtDnDInfoPtr		dip = (DtDnDInfoPtr) call_data;
    int				i;

    if (dip->error)
	return;
    else
    {
	for (i = 0; i < dip->nitems; i++)
	{
	    char	*name;

	    /* Use only the basename. */
	    name = strrchr (dip->files [i], '/');
	    if (!name)
		name = dip->files [i];
	    else
		name++;
	    OpenPrinter (name);
	}
    }
}	/* End of SelectionCB () */

/* DropNotify
 *
 * Called by a pseudo-drop event on the toplevel window.  Get printer names.
 */
static Boolean
DropNotify (Widget w, Window win, Position x, Position y, Atom selection,
            Time timestamp, OlDnDDropSiteID drop_site_id,
            OlDnDTriggerOperation op, Boolean send_done, Boolean forwarded, 
            XtPointer closure)
{
    DtGetFileNames (w, selection, timestamp, send_done,
		    SelectionCB, (XtPointer) op);

    return(True);
}	/* End of DropNotify () */

static char *
ConvertLocales(char *locale_def, char *raw_locale)
{
    int			i;
    char		*lineptr;
    char		labelbuf[BUFSIZ], line[BUFSIZ];
    char		tmp_locale[BUFSIZ], locale_buf[BUFSIZ];
    static char		localefile[BUFSIZ];
    FILE		*fp;
    Boolean		found = FALSE;
    static String       LocaleLabel;
    char		*char_set;

    /*  Open the ol_locale_def file */
    strcpy(tmp_locale, raw_locale);

    if ((char_set = strchr(tmp_locale,'.')) != NULL) {
          *char_set = 0;
          char_set++;
    }

    sprintf(localefile,"%s/%s/ol_locale_def", locale_def, tmp_locale);
    if ((fp = fopen (localefile, "r")) != NULL) {
          while (fgets (line, BUFSIZ, fp)) {
                  lineptr = line;
                  if (strncmp (XNLLANGUAGE, lineptr, 13) == 0) {
                        while (*lineptr != '\"' && *lineptr != '\n')
                            lineptr++;
                        if (*lineptr == '\"') {
                            lineptr++;
                            while (*lineptr == ' ') lineptr++;
                            i=0;
                            while (*lineptr != '\"' && *lineptr != '\n')
                                 labelbuf[i++]=*lineptr++;
                            labelbuf[i] = '\0';
                            LocaleLabel= (String)strdup((char*)GetGizmoText(labelbuf));
                            found = TRUE;
                            break;
                        } /* If first quote is found */
                  } /* If XNLLANGUAGE */
          } /* While fgets */

          fclose (fp);

          if (found) {
	          /* Create the Character Set extension.  
		     e.g.  PC 850
	 	   */
	          if (char_set != NULL)
		       sprintf(locale_buf,"%s (%s)", LocaleLabel, char_set);
		  else
		       sprintf(locale_buf,"%s", LocaleLabel);
		  LocaleLabel = strdup (locale_buf);
		  return(LocaleLabel);
	  }
    }
    if (char_set != NULL)
         sprintf(locale_buf,"%s (%s)", tmp_locale, char_set);
    else
         sprintf(locale_buf,"%s", tmp_locale);
    LocaleLabel = strdup (locale_buf);
    return(LocaleLabel);
}

