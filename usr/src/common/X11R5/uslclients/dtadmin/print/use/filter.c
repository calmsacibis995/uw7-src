#ifndef NOIDENT
#ident	"@(#)dtadmin:print/use/filter.c	1.9"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <Intrinsic.h>
#include <StringDefs.h>
#include <Xol/OpenLook.h>

#include <filters.h>

#include "properties.h"
#include "error.h"

#define DEFAULTDIR_STR	"/usr/X/desktop/PrintMgr/Filters/%s"
#define DEFAULT_TYPES	"/usr/X/desktop/PrintMgr/Types"
#define TYPES_ALLOC	5
#define FILTER_ALLOC	5

extern ResourceRec	AppResources;

static Filter	*FindFilterOpts (char *name);
static _FILTER	*InstantiateFilter (char *, char *, char *, char *);
static int	Instantiate ( _FILTER **, TYPE *, TYPE *);
static int	SearchListType ( TYPE *, TYPE *);
static int	TypeMatch (TYPE *type1, TYPE *type2);
extern void	ReadInputTypes (ButtonItem **, unsigned *);

static char	*XwinHome;

static ButtonItem	DefaultTypes [] = {
    { (XtArgVal) "simple", (XtArgVal) TXT_simple, },	/* Simple */
    { (XtArgVal) 0, (XtArgVal) TXT_other, },		/* Other */
};

/* FindFilter
 *
 * Get the filter data for job based on content type and printer type.
 * Return a linked list of filters in properties->filter.
 */
Filter *
FindFilter (char *inputType, Printer *printer)
{
    Filter		*firstFilter;
    Filter		*filter;
    Filter		*prev;
    _FILTER		*pFilters;
    char		**printer_types;
    char		**pType;
    static Boolean	first = True;
    static FilterData	dfltData [] = { 0, "DfltOpt", "%s", 0, 's', };
    static Filter	dfltFilter = { 0, 0, 0, 1, &dfltData, };
    extern Widget	TopLevel;

    if (first)
    {
	first = False;

	dfltData [0].lbl = GetStr (TXT_miscOptions);
    }

    /* Find the specific filters that will be used for this input 
     * type and printer.  Note that this is assuming no modes, which
     * can lead to problems in complex cases, but these cases are rare
     * in practice and not worth the effort to deal with.  Still, we
     * have to try all printer types and all input types until we get
     * a combination that works.
     */
    printer_types = printer->config->printer_types;
    pFilters = (_FILTER *) 0;
    if (strcmp (*printer_types, NAME_UNKNOWN) != 0)
    {
	for (pType=printer_types; pType && *pType && !pFilters; pType++)
	{
	    pFilters = InstantiateFilter (inputType, *pType, *pType,
					  printer->name);
	}

	for (pType=printer->config->input_types;
	     pType && *pType && !pFilters;
	     pType++)
	{
	    /* Don't waste time with check we've already made. */
	    if (!printer_types || !searchlist(*pType, printer_types))
	    {
		/* Either we have one (or less) printer types and many
		 * input types, or we have one input type, ``simple''.
		 * This restriction is imposed by lpadmin on the printer
		 * database.  Regardless, using the first printer type is OK.
		 */
		pFilters = InstantiateFilter (inputType, *pType,
					      *printer_types, printer->name);
	    }
	}
    }

    /* For each filter in the pipeline, get the options that the filter
     * can accept.  If there is no valid pipeline, assume that this is a
     * "pseudo-filter"; that is, use the input type as the filter name in
     * order to get the options, but we assume that we will not be submitting
     * the request to lp.  If this assumption is wrong, lp will simply
     * reject the request and we post an error message.
     *
     * Also, always add the default filter to the end of the list.
     */
    firstFilter = prev = (Filter *) 0;
    for ( ; pFilters; pFilters=pFilters->next)
    {
	filter = FindFilterOpts (pFilters->name);
	if (filter)
	{
	    filter->next = &dfltFilter;
	    if (prev)
		prev->next = filter;
	    else
		firstFilter = filter;
	    prev = filter;
	}
    }

    if (!firstFilter)
	firstFilter = &dfltFilter;
    return (firstFilter);
}	/* End of FindFilter () */

/* FindFilterOpts
 *
 * Find the filter options structure associated with a particular filter.
 * Note that the options file is not read at this time; it is just checked
 * for existence.  These records are never freed, so check the list of
 * known filters first.
 */
static Filter *
FindFilterOpts (char *name)
{
    Filter		*filter;
    char		filterFile [256];
    struct stat		statbuf;
    static Filter	*filterList;

    /* Check if we already know about this filter. */
    for (filter=filterList; filter; filter=filter->listNext)
    {
	if (strcmp (filter->name, name) == 0)
	    return (filter);
    }

    /* Didn't find it.  Check the file system to see if the
     * options file was created.  While it's convenient, check
     * that the file is a regular file and contains something.
     * Defer checking if the file is actually readable.
     */
    if (AppResources.filterDir [0] != '/')
    {
	char	*dir;

	if (!XwinHome)
	{
	    XwinHome = getenv ("XWINHOME");
	    if (!XwinHome)
		XwinHome = "/usr/X";
	}

	dir = XtMalloc (strlen (XwinHome) + 1 +
			strlen (AppResources.filterDir) + 1);
	sprintf (dir, "%s/%s", XwinHome, AppResources.filterDir);
	AppResources.filterDir = strdup (dir);
    }

    sprintf (filterFile, "%s/%s", AppResources.filterDir, name);
    if (stat (filterFile, &statbuf) != 0 || !S_ISREG (statbuf.st_mode) ||
	statbuf.st_size == 0)
    {
	sprintf (filterFile, DEFAULTDIR_STR, name);
	if (stat (filterFile, &statbuf) != 0 || !S_ISREG (statbuf.st_mode) ||
	    statbuf.st_size == 0)
	    return ((Filter *) 0);
    }

    /* Create a record for the filter and add it to the list. */
    filter = (Filter *) XtMalloc (sizeof (Filter));
    filter->listNext = filterList;
    filterList = filter;
    filter->name = strdup (name);
    filter->cnt = 0;
    filter->data = (FilterData (*)[]) 0;
    filter->next = (Filter *) 0;

    return (filter);
}	/* End of FindFilterOpts () */

/* ReadFilterOpts
 *
 * Read the options from the description file.
 */
void
ReadFilterOpts (Filter *filter)
{
    FILE	*file;
    FilterData	*option;
    int		allocated = 0;
    int		cnt;
    char	*label;
    char	*catalog;
    char	*pattern;
    char	*caret;
    char	*percent;
    char	*nextPercent;
    char	buf [1024];

    sprintf (buf, "%s/%s", AppResources.filterDir, filter->name);
    if (!(file = fopen (buf, "r")))
    {
	sprintf (buf, DEFAULTDIR_STR, filter->name);
	if (!(file = fopen (buf, "r")))
	{
	    /* To prevent trying to read the options again, set data to
	     * non-zero.  Since the count is zero, we won't try to access it.
	     */
	    filter->data = (FilterData (*)[]) 1;
	    filter->cnt = 0;
	    return;
	}
    }

    filter->cnt = 0;
    while (fgets (buf, 1024, file))
    {
	if (filter->cnt >= allocated)
	{
	    allocated += FILTER_ALLOC;
	    filter->data = (FilterData (*)[])
		XtRealloc ((char *)filter->data, allocated*sizeof(FilterData));
	}

	option = *filter->data + filter->cnt++;

	/* First entry is a tag; ignore it.  Second entry is the label.
	 * Third is option pattern.  Forth (optional) string is help file.
	 */
	if (!strtok (buf, "\t\n") || !(label = strtok (NULL, "\t\n")) ||
	    !(pattern = strtok (NULL, "\t\n")))
	{
	    filter->cnt--;
	    continue;
	}

	option->origPattern = strdup (pattern);
	option->help = strtok (NULL, "\t\n");
	if (option->help)
	    option->help = strdup (option->help);

	caret = strchr (label, '^');
	if (caret)
	{
	    *caret = 0;
	    option->lbl = gettxt (caret+1, label);
	    if (option->lbl == label)
		option->lbl = strdup (label);
	}
	else
	    option->lbl = strdup (label);

	/* Determine the type of controls needed from the pattern string. */
	for (percent = strchr (pattern, '%'); 
	     percent && percent [1] == '%';
	     percent = strchr (percent + 2, '%'))
	    ;	/* do nothing */

	if (!percent)
	{
	    /* A simple option--use a checkbox (boolean).  Make sure %% are
	     * converted to % in the pattern.
	     */
	    option->pattern = XtMalloc (strlen (pattern) + 1);
	    sprintf (option->pattern, pattern);
	    option->type = 'b';
	}
	else
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
		    option->type = percent [1];
		    percent [1] = 's';
		    if (nextPercent)
		    {
			int	len = percent - pattern + 2;

			option->pattern = XtMalloc (len + 1);
			strncpy (option->pattern, pattern, len);
			option->pattern [len] = 0;
			pattern = percent + 2;
			percent = nextPercent;
		    }
		    else
		    {
			option->pattern = strdup (pattern);
			pattern = 0;
		    }
		    break;

		default:
		    /* Unknown control.  Ignore the option. */
		    filter->cnt--;
		    if (nextPercent)
		    {
			pattern = percent + 2;
			percent = nextPercent;
		    }
		    else
			pattern = 0;
		    break;
		}

		/* If there are still more options, assign space. */
		if (pattern && *pattern)
		{
		    /* Arbitrarily allow no more than 5 substitutions in
		     * a pattern.  If more are needed (it's a strange filter
		     * if they are), then users will have to enter them via
		     * miscellaneous options.
		     */
		    if (++cnt >= 5)
		    {
			/* Remove the options derived from this pattern. */
			filter->cnt -= cnt;
			while (--cnt > 0)
			    XtFree ((option--)->pattern);
			XtFree (option->pattern);
			XtFree (option->origPattern);
			XtFree (option->help);
			XtFree (option->lbl);
			break;  /* ignore remainder of pattern */
		    }

		    if (filter->cnt >= allocated)
		    {
			allocated += FILTER_ALLOC;
			filter->data = (FilterData (*)[])
			    XtRealloc ((char *) filter->data,
				       allocated * sizeof (FilterData));
		    }

		    option = *filter->data + filter->cnt++;
		    option->help = option [-1].help;
		    option->lbl = "";
		    option->origPattern = 0;
		}
		else
		    cnt = 0;
	    } while (pattern && *pattern);
	}
    }

    fclose (file);
}	/* End of ReadFilterOpts () */

/* ReadInputTypes
 *
 * Read the input types allowed in the menu.
 */
extern void
ReadInputTypes (ButtonItem **pItems, unsigned *pCnt)
{
    FILE		*file;
    ButtonItem		*item;
    int			allocated = 0;
    char		*label;
    char		*inType;
    char		*caret;
    char		buf [1024];
    static Boolean	first = True;

    if (first)
    {
	first = False;

	SetButtonLbls (DefaultTypes, XtNumber (DefaultTypes));
    }

    if (AppResources.typesFile [0] != '/')
    {
	char	*dir;

	if (!XwinHome)
	{
	    XwinHome = getenv ("XWINHOME");
	    if (!XwinHome)
		XwinHome = "/usr/X";
	}

	dir = XtMalloc (strlen (XwinHome) + 1 +
			strlen (AppResources.typesFile) + 1);
	sprintf (dir, "%s/%s", XwinHome, AppResources.typesFile);
	AppResources.typesFile = strdup (dir);
    }

    if (!(file = fopen (AppResources.typesFile, "r")))
    {
	if (!(file = fopen (DEFAULT_TYPES, "r")))
	{
	    *pItems = DefaultTypes;
	    *pCnt = XtNumber (DefaultTypes);
	    return;
	}
    }

    *pCnt = 0;
    while (fgets (buf, 1024, file))
    {
	if (*pCnt >= allocated)
	{
	    allocated += TYPES_ALLOC;
	    *pItems = (ButtonItem *)
		XtRealloc ((char *) *pItems, allocated * sizeof (ButtonItem));
	}

	item = *pItems + (*pCnt)++;

	/* First field is a tag; ignore it.  Second field is an optionally
	 * internationalized label.  A '^' separates the default string from
	 * the file:index.  Third is the input type used by lp.
	 */
	if (!strtok (buf, "\t\n") || !(label = strtok (NULL, "\t\n")) ||
	    !(inType = strtok (NULL, "\t\n")))
	{
	    (*pCnt)--;
	    continue;
	}

	caret = strchr (label, '^');
	if (caret)
	{
	    *caret = 0;
	    item->lbl = (XtArgVal) gettxt (caret+1, label);
	    if ((char *) item->lbl == label)
		item->lbl = (XtArgVal) strdup (label);
	}
	else
	    item->lbl = (XtArgVal) strdup (label);

	item->userData = (XtArgVal) strdup (inType);
    }

    fclose (file);

    /* Add an entry for "other" to the end of the list. */
    if (*pCnt >= allocated)
    {
	allocated++;
	*pItems = (ButtonItem *)
	    XtRealloc ((char *) *pItems, allocated * sizeof (ButtonItem));
    }
    
    item = *pItems + (*pCnt)++;
    *item = DefaultTypes [XtNumber (DefaultTypes) - 1];
}	/* End of ReadInputTypes () */

/* The functions below this point have been hacked shamelessly from the
 * lp source lib/filters/insfilter.c
 */

static struct S {
    TYPE	input_type;
    TYPE	output_type;
    TYPE	printer_type;
    char	*printer;
} S;

/*
 * InstantiateFilter()
 */

static _FILTER *
InstantiateFilter (char *input_type, char *output_type,
		   char *printer_type, char *printer)
{
    _FILTER	*pipeline;

    S.input_type.name = input_type;
    S.input_type.info = isterminfo (input_type);
    S.output_type.name = output_type;
    S.output_type.info = isterminfo (output_type);
    S.printer_type.name = printer_type;
    S.printer_type.info = isterminfo (printer_type);
    S.printer = printer;

    /*
     * If the filters have't been loaded yet, do so now.
     * We'll load the standard table, but the caller can override
     * this by first calling "loadfilters()" with the appropriate
     * filter table name.
     */
    if (!filters && loadfilters ((char *) 0) == -1)
	return ((_FILTER *) 0);

    /*
     * Preview the list of filters, to rule out those that
     * can't possibly work.
     */
    {
	register _FILTER	*pf;

	for (pf = filters; pf->name; pf++)
	{
	    pf->mark = FL_CLEAR;

	    if (printer && !searchlist(printer, pf->printers))
		pf->mark = FL_SKIP;
	    else
		if (printer_type &&
		    !SearchListType (&(S.printer_type), pf->printer_types))
		    pf->mark = FL_SKIP;

	}
    }

    /*
     * Find a pipeline that will convert the input-type to the
     * output-type and map the parameters as well.
     */
    if (!Instantiate (&pipeline, &S.input_type, &S.output_type))
	return ((_FILTER *) 0);

    return (pipeline);
}	/* End of InstantiateFilter () */

static int
TypeMatch (TYPE *type1, TYPE *type2)
{
    if (STREQU (type1->name, NAME_ANY) || STREQU(type2->name, NAME_ANY) ||
	STREQU (type1->name, type2->name) ||
	(STREQU (type1->name, NAME_TERMINFO) && type2->info) ||
	(STREQU (type2->name, NAME_TERMINFO) && type1->info))
	return (1);
    else
	return (0);
}	/* End of TypeMatch () */

/**
 ** SearchListType() - SEARCH (TYPE *) LIST FOR ITEM
 **/
static int
SearchListType (TYPE *itemp, TYPE *list)
{
    if (!list || !list->name)
	return (0);

    /*
     * This is a linear search--we believe that the lists
     * will be short.
     */
    while (list->name)
    {
	if (TypeMatch (itemp, list))
	    return (1);
	list++;
    }
    return (0);
}	/* End of SearchListType () */

/**
 ** Instantiate() - CREATE FILTER-PIPELINE KNOWING INPUT/OUTPUT TYPES
 **/

/*
 *	The "Instantiate()" routine is the meat of the "InstantiateFilter()"
 *	algorithm. It is given an input-type and output-type and finds a
 *	filter-pipline that will convert the input-type into the
 *	output-type. Unlike in lp, other criteria are NOT used to select
 *	the pipeline.  This might result in the "wrong" pipeline being
 *	selected, and therefore, the wrong filter options presented to the
 *	user.  However, given most reasonable filter tables, this works
 *	just fine.
 *
 *	The filter-pipeline is built up and returned in "pipeline".
 *	Conceptually this is just a list of filters. What is used in
 *	the routine, though, is a pair of linked lists, one list forming
 *	the ``right-half'' of the pipeline, the	other forming the
 *	``left-half''. The pipeline is then the two lists taken together.
 *
 *	The "Instantiate()" routine looks for a single filter that matches
 *	the input-type and output-type. If one is found, it is added to
 *	the end of the ``left-half'' list (it could be added to the
 *	beginning of the ``right-half'' list with no problem). The two
 *	lists are linked together to form one linked list.
 *
 *	If a single filter is not found, "Instantiate()" examines all
 *	pairs of filters where one in the pair can accept the input-type
 *	and the other can produce the output-type. For each of these, it
 *	calls itself again to find a filter that can join the pair
 *	together--one that accepts as input the output-type of the first
 *	in the pair, and produces as output the input-type of the second
 *	in the pair.  This joining filter may be a single filter or may
 *	be a filter-pipeline. "Instantiate()" checks for the trivial case
 *	where the input-type is the output-type; with trivial cases it
 *	links the two lists without adding a filter.
 */

/**
 ** Instantiate()
 **/

/*
 * A PIPELIST is what is passed to each recursive call to "Instantiate()".
 * It contains a pointer to the end of the ``left-list'', a pointer to the
 * head of the ``right-list'', and a pointer to the head of the left-list.
 * The latter is passed to "verify". The end of the right-list (and thus
 * the end of the entire list when left and right are joined) is the
 * filter with a null ``next'' pointer.
 */
typedef struct PIPELIST {
    _FILTER	*lhead;
    _FILTER	*ltail;
    _FILTER	*rhead;
} PIPELIST;

static int	RecursiveInstantiate (PIPELIST *, TYPE *, TYPE *);

static int	peg;

static int
Instantiate (_FILTER **pline, TYPE *input, TYPE *output)
{
    PIPELIST	p;
    int		ret;

    peg = 0;
    p.lhead = p.ltail = p.rhead = 0;
    ret = RecursiveInstantiate (&p, input, output);
    *pline = p.lhead;
    return (ret);
}	/* End of Instantiate () */

#define	ENTER()		int our_tag = ++peg;

#define	LEAVE(Y)	if (!Y) { \
				register _FILTER *f; \
				for (f = filters; f->name; f++) \
					CLEAR(f); \
				return(0); \
			} else return(1)

#define MARK(F,M)	(((F)->mark |= M), (F)->level = our_tag)

#define CLEAR(F)	if ((F)->level == our_tag) \
				(F)->level = 0, (F)->mark = FL_CLEAR

#define CHECK(F,M)	(((F)->mark & M) && (F)->level == our_tag)

#define	USED(F)		((F)->mark)

static int
RecursiveInstantiate (PIPELIST *pp, TYPE *inputp, TYPE *outputp)
{
    register _FILTER	*prev_lhead;
    register _FILTER	*prev_ltail;

    /*
     * Must be first ``statement'' after declarations.
     */
    ENTER ();

    /*
     * We're done when we've added filters on the left and right
     * that let us connect the left and right directly; i.e. when
     * the output of the left is the same type as the input of the
     * right. HOWEVER, there must be at least one filter involved,
     * to allow the filter feature to be used for handling modes,
     * pages, copies, etc. not just FILTERING data.
     */
    if (TypeMatch (inputp, outputp) && pp->lhead)
    {
	/*
	 * Getting here means that we must have a left and right
	 * pipeline. Why? For "pp->lhead" to be non-zero it
	 * must have been set below. The first place below
	 * doesn't set the right pipeline, but it also doesn't
	 * get us here (at least not directly). The only
	 * place we can get to here again is the second place
	 * "pp->phead" is set, and THAT sets the right pipeline.
	 */
	pp->ltail->next = pp->rhead;
	LEAVE (1);
    }

    /*
     * Each time we search the list of filters, we examine
     * them in the order given and stop searching when a filter
     * that meets the needs is found. If the list is ordered with
     * fast filters before slow filters, then fast filters will
     * be chosen over otherwise-equal filters.
     */

    /*
     * See if there's a single filter that will work.
     * Just in case we can't find one, mark those that
     * will work as left- or right-filters, to save time
     * later.
     *
     * Also, record exactly *which* input/output
     * type would be needed if the filter was used.
     * This record will be complete (both input and output
     * recorded) IF the single filter works. Otherwise,
     * only the input, for the left possible filters,
     * and the output, for the right possible filters,
     * will be recorded. Thus, we'll have to record the
     * missing types later.
     */
    {
	register _FILTER	*pf;

	for (pf = filters; pf->name; pf++)
	{
	    if (USED (pf))
		continue;

	    if (SearchListType (inputp, pf->input_types))
	    {
		MARK (pf, FL_LEFT);
		pf->inputp = inputp;
	    }
	    if (SearchListType (outputp, pf->output_types))
	    {
		MARK (pf, FL_RIGHT);
		pf->outputp = outputp;
	    }

	    if (CHECK (pf, FL_LEFT) && CHECK (pf, FL_RIGHT))
	    {
		prev_lhead = pp->lhead;
		prev_ltail = pp->ltail;

		if (!pp->lhead)
		    pp->lhead = pf;
		else
		    pp->ltail->next = pf;
		(pp->ltail = pf)->next = pp->rhead;

		LEAVE (1);
	    }
	}
    }

    /*
     * Try all DISJOINT pairs of left- and right-filters; recursively
     * call this function to find a filter that will connect
     * them (it might be a ``null'' filter).
     */
    {
	register _FILTER	*pfl;
	register _FILTER	*pfr;
	register TYPE		*llist;
	register TYPE		*rlist;

	for (pfl = filters; pfl->name; pfl++)
	{
	    if (!CHECK (pfl, FL_LEFT))
		continue;

	    for (pfr = filters; pfr->name; pfr++)
	    {
		if (pfr == pfl || !CHECK (pfr, FL_RIGHT))
		    continue;

		prev_lhead = pp->lhead;
		prev_ltail = pp->ltail;

		if (!pp->lhead)
		    pp->lhead = pfl;
		else
		    pp->ltail->next = pfl;
		(pp->ltail = pfl)->next = 0;

		pfr->next = pp->rhead;
		pp->rhead = pfr;

		/*
		 * Try all the possible output types of
		 * the left filter with all the possible
		 * input types of the right filter. If
		 * we find a combo. that works, record
		 * the output and input types for the
		 * respective filters.
		 */
		for (llist = pfl->output_types; llist->name; llist++)
		    for (rlist = pfr->input_types; rlist->name; rlist++)
			if (RecursiveInstantiate (pp, llist, rlist))
			{
			    pfl->outputp = llist;
			    pfr->inputp = rlist;
			    LEAVE (1);
			}
		pp->rhead = pfr->next;
		if ((pp->ltail = prev_ltail))
		    pp->ltail->next = 0;
		pp->lhead = prev_lhead;
	    }
	}
    }

    LEAVE (0);
}	/* End of RecursiveInstantiate () */
