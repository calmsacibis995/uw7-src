#pragma ident	"@(#)dtm:olwsm/locale.c	1.61.1.1"

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <libgen.h>
#include "dirent.h"
#include "ctype.h"

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/ShellP.h>

#include <Xm/Xm.h>
#include <Xm/ScrolledW.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/MenuShell.h>

#include <MGizmo/Gizmo.h>
#include <WSMcomm.h>
#include <DesktopP.h>		/* for DtGetShellOfWidget */
#include <array.h>

#include "misc.h"
#include "node.h"
#include "list.h"
#include "exclusive.h"
#include "property.h"
#include "resource.h"
#include "wsm.h"
#include "error.h"

/*
 * Private definitons:
 */

typedef struct ResourceName {
	String			name;
	XrmQuark		quark;
} ResourceName;

typedef struct Labeled {
	String			name;
	String			label;
	Exclusive		exclusive;
} Labeled;

typedef struct LocaleListView {
	String			name;
	Widget			list;
	Widget			cap;
	Cardinal		current;
	ChangeBar*		changebar;
	void			(*change) ();
} LocaleListView;

typedef struct Locale {
	XrmDatabase		db;
	String			name;
	String			label;
	Cardinal		item;
} Locale;


typedef _OlArrayStruct(String, StringArray)	StringArray;
typedef _OlArrayStruct(Locale, LocaleArray)	LocaleArray;
	
/*
 * Private routines:
 */

static void		Exit( XtPointer );
static void		Import( XtPointer );
static void		Export( XtPointer );
static void		Create( Widget, XtPointer );
static ApplyReturn *	ApplyCB( Widget, XtPointer );
static void		ResetCB( Widget, XtPointer );
static void		FactoryCB( Widget, XtPointer );
static void		PopdownCB( Widget, XtPointer );
static String		SetCurrent(Cardinal /* index */, String, Boolean );
static void		FetchFileSearchPath( void );
static String		FindSubString( String, String );
static int		StringCompare( String *, String * );
static Boolean		AccumulateFileSearchPath( String );
static Boolean		AccumulateParentLocaleDirectories( String );
static Boolean		IsDir( String, String );
static void		AddDatabaseFileToLocaleList( String, LocaleArray * );
static String		GetResource( XrmDatabase, ResourceName * );
static String		_MultiCodesetStrchr( String, int, int );
static void		ParseNameAndLabel( String, String *, String * );
static Locale *		FindLocale( String );
static int		LocaleCompare( Locale *, Locale * );
static void		CreateLocaleListView( Locale *, Widget );
static void		LocaleSelectCB( Widget, XtPointer, XtPointer);
static void		Reset(Locale *, List * );
static Boolean		NewLocale(
				LocaleListView *, Locale *, int state_change
			);  /* int was Oldefine */
static void		CreateSettingsView(Locale *, Widget );
static void		FetchResourceAndParse(
				XrmDatabase, ResourceName *, Exclusive *
			);
static String		InputMethodPartner( Locale *, String, Cardinal );
static void		FreeExclusive( Exclusive * );
static Widget		CreateLabeledText( Labeled *, Widget, Widget );
static Display *	OpenDisplayForNewLocale( String	 );
static void		CloseDisplay( Display *);
static void		FillInLangSubs( Substitution, String );
static void		TouchChangeBarDB ( void );

#define LOGIN		".login"
#define PROFILE		".profile"
#define DONOTEDIT	"\t#!@ Do not edit this line !@"

#define EXCL(LAB, STR, CHANGE) { \
	False,								\
	(LAB),								\
	(STR),								\
	(ExclusiveItem *)0,						\
	(ExclusiveItem *)0,						\
	(List *)0,							\
	(void (*) (struct Exclusive * ))0,				\
	(ADDR)0,							\
	(Widget *)0,							\
	True,								\
	NULL,								\
	CHANGE,								\
}

/*
 * Private types:
 */

#define ABBREVIATED(name) { \
	name, NULL, EXCL("menu", "menu", TouchChangeBarDB) \
}

static Labeled	InputLang	= ABBREVIATED("inputLang");
static Labeled	DisplayLang	= ABBREVIATED("displayLang");
static Labeled	DateTimeFormat	= ABBREVIATED("timeFormat");
static Labeled	NumericFormat	= ABBREVIATED("numeric");
static Labeled	FontGroup	= ABBREVIATED("fontGroup");
static Labeled	InputMethod	= ABBREVIATED("inputMethod");

/*
 * Convenient macros:
 */

#define XNLLANGUAGE		0
#define INPUTLANG		1
#define DISPLAYLANG		2
#define DATETIMEFORMAT		3
#define NUMERICFORMAT		4
#define FONTGROUP		5
#define FONTGROUPDEF		6
#define FONTLIST		7
#define SANS_FAMILYFONTLIST	8
#define SERIF_FAMILYFONTLIST	9
#define MONO_FAMILYFONTLIST	10
#define INPUTMETHOD		11
#define IMSTATUS		12
#define FRONTENDSTRING		13
#define BASICLOCALE		14

#define CURRENT(x)		*(ADDR *)&(_current[x].value)
#ifdef GLOBAL
#undef GLOBAL
#endif
#define GLOBAL(x)	resource_value(&global_resources, _current[x].name)

#define ITEM(exclusive,addr)	ResourceItem(exclusive, addr)

#define MultiCodesetStrchr(s,c)		  _MultiCodesetStrchr(s,c,0)
#define MultiCodesetStrchrWithQuotes(s,c) _MultiCodesetStrchr(s,c,'"')

/*
 * Public data:
 */
static Arg		locale_args[] = {
	{XmNorientation,	XmVERTICAL},
	{XmNnumColumns,		1}
};


static HelpInfo LocaleHelp = {
	NULL, NULL, "DesktopMgr/locpref.hlp", NULL
};

Property		localeProperty = {
	"Set Locale",
	locale_args,
	XtNumber (locale_args),
	&LocaleHelp,
	'\0',
	Import,
	Export,
	Create,
	ApplyCB,
	ResetCB,
	FactoryCB,
	PopdownCB,
	0,
	0,
	0,
	0,
	NULL,
	NULL,
	False,
	NULL,
	420,
	400,
};

/*
 * Private data:
 */

static String		defaultLocaleName = "C";
static String		defaultLocale     = "\
	*xnlLanguage:    C \"Default (C locale)\"\n\
	*inputLang:      C \"Default (C locale)\"\n\
	*displayLang:    C \"Default (C locale)\"\n\
	*timeFormat:     C \"12/31/90 5:30 PM\"\n\
	*numeric:        C \"10,000\"\n\
	*fontGroup:      \n\
	*fontGroupDef:   \n\
	*fontList:       \n\
	*sansSerifFamilyFontList: \n\
	*serifFamilyFontList: \n\
	*monospacedFamilyFontList: \n\
	*inputMethod:    \n\
	*imStatus:       False\n\
	*frontEndString: \n\
	*basicLocale:    C \"C\"\n\
";

static ResourceName	resources[] = {
	{ "xnlLanguage",	0 },
	{ "inputLang",		0 },
	{ "displayLang",	0 },
	{ "timeFormat",		0 },
	{ "numeric",		0 },
	{ "fontGroup",		0 },
	{ "fontGroupDef",	0 },
	{ "fontList",		0 },
	{ "sansSerifFamilyFontList",	0 },
	{ "serifFamilyFontList",	0 },
	{ "monospacedFamilyFontList",	0 },
	{ "inputMethod",	0 },
	{ "imStatus",		0 },
	{ "frontEndString",	0 },
	{ "basicLocale",	0 },
};

static List		factory = LISTOF(Resource);

static Resource		_current[] = {
	{ "*xnlLanguage"    },
	{ "*inputLang"      },
	{ "*displayLang"    },
	{ "*timeFormat"     },
	{ "*numeric"        },
	{ "*fontGroup"      },
	{ "*fontGroupDef"   },
	{ "*fontList"	    },
	{ "*sansSerifFamilyFontList"},
	{ "*serifFamilyFontList"},
	{ "*monospacedFamilyFontList"},
	{ "*inputMethod"    },
	{ "*imStatus"       },
	{ "*frontEndString" },
	{ "*basicLocale"    },
};

static List		current		= LIST(Resource, _current);

static Widget		SettingsView	= 0;
static LocaleListView	XnlLanguage	= { "xnlLanguage" };

static String		FileSearchPath	= 0;

static Display *	SeparateDisplayForNewLocale	= 0;
static String		SeparateLocale			= 0;

String		CurrentLocale	= 0;
String		PrevLocale	= 0;

static StringArray	ParentLocaleDirectories;
static LocaleArray	locales;
static int		next_item_index = 0;
static int		SelectedItemNumber = 0;

/*
 * Import()
 */

static void 
Import (XtPointer closure)
{
	extern char *		GetResourceFilename();
	struct stat		status;
	DIR *			dirp;
	struct dirent *		direntp;
	Locale			locale;
	Locale *		p;
	String			name;
	String			dir;
	String			file;
	String			xnlLanguage;
	Cardinal		n;
	Resource		r;
	static SubstitutionRec	subs[] = {
		{ 'N' , OL_LOCALE_DEF },	/* MUST BE 0th ELEMENT */
		{ 'T' , "" },
		{ 'S' , "" },
		{ 'C' , "" },
		{ 'L' },
		{ 'l' },
		{ 't' },
		{ 'c' },
	};
/*	subs[4..7].substitution =   set below */


	/*
	 * Most of what we do in this routine is look for all the
	 * system and user defined locales.
	 */
	
	_OlArrayInitialize (&ParentLocaleDirectories, 10, 10, StringCompare);
	_OlArrayInitialize (&locales, 10, 10, LocaleCompare);

	/*
	 * Fetch the names of the parent directories of the locale
	 * definition files. This is tricky, because the convenient
	 * routine that knows how to parse XFILESEARCHPATH (or an
	 * implementation-specific default for it) automatically
	 * substitutes the current locale's name into the path--we
	 * don't want the directory for the current locale, we want
	 * the parent directory(s) for all possible locales. Thus we
	 * first discover XFILESEARCHPATH on our own and we don't use
	 * that convenient routine (OK, its name is XtResolvePathname).
	 */
	
	FetchFileSearchPath ();
	if (FileSearchPath) {
		/*
		 * We don't want to expand the standard substitutions,
		 * but instead want them to show up as themselves.
		 * XtFindFile will drop the % if it has no substitution,
		 * so we do the following. (Actually, the only one we
		 * care about is %L.)
		 *
		 * We also reconstruct the search path so it parallels the
		 * list of parent directories. This excludes path entries
		 * that, e.g., don't have a %L and would thus be locale
		 * insensitive. This is important to avoid locking in on
		 * the same locale definition file in a %L-less path, in
		 * the XtFindFile later on.
		 *
		 * See AccumulateParentLocaleDirectories() for details.
		 */
		static SubstitutionRec	same_subs[] = {
			{ 'N' , "%N" },
			{ 'T' , "%T" },
			{ 'S' , "%S" },
			{ 'C' , "%C" },
			{ 'L' , "%L" },
			{ 'l' , "%l" },
			{ 't' , "%t" },
			{ 'c' , "%c" },
		};
		String			fsp = FileSearchPath;
	
		FileSearchPath = 0;
		(void)XtFindFile (
			fsp,
			same_subs, XtNumber(same_subs),
			AccumulateParentLocaleDirectories
		);
		XtFree (fsp);
	}

	/*
	 * The user may have defined a private locale, so take a shot
	 * in the dark to see if we hit it. If we do, save the locale
	 * and later ignore any ``system'' locale of the same name.
	 *
	 * Note: If we were started with a valid locale (e.g. via
	 * the xnlLanguage option or resource, or via $LANG), then
	 * the locale definition file fetched below will be for that
	 * locale. That's mostly OK, the only bummer is that it may
	 * hide a locale definition file hidden in an odd place by
	 * the user.
	 */
	file = XtResolvePathname(
		DISPLAY,
		(String)0,		/* type */
		subs[0].substitution,	/* filename */
		(String)0,		/* suffix */
		(String)0,		/* use default path */
		(Substitution)0, (Cardinal)0,
		(XtFilePredicate)0	/* use default */
	);
	if (file) {
		AddDatabaseFileToLocaleList (file, &locales);
		XtFree (file);
	}

	/*
	 * Step through the content of each directory in the
	 * accumulated list of potential locale parent directories,
	 * to see which subdirectories (if any) harbor locale
	 * definition files.
	 */
	for (n = 0; n < _OlArraySize(&ParentLocaleDirectories); n++) {
		String	parent = _OlArrayElement(&ParentLocaleDirectories, n);

		if (!(dirp = opendir(parent))) {
			continue;
		}

		while ((direntp = readdir(dirp))) {
			/*
			 * Skip the obvious.
			 */
			if (
				MATCH(direntp->d_name, ".") ||
				MATCH(direntp->d_name, "..") ||
				!IsDir(parent, direntp->d_name)
			) {
				continue;
			}

			/*
			 * Found a file or sub-directory under the parent
			 * of a locale directory. Try this as a locale
			 * name. Skip it if no locale definition file is
			 * found.
			 */
			FillInLangSubs (&subs[4], direntp->d_name);
			file = XtFindFile(
				FileSearchPath,
				subs, XtNumber(subs),
				(XtFilePredicate)0 /* use default */
			);
			if (subs[5].substitution) {
				/* See "FillInLangSubs" (sorry!) */
				XtFree ((XtPointer)subs[5].substitution);
			}
			if (!file) {
				continue;
			}

			/*
			 * Found a correctly named locale definition file;
			 * fetch the content into an XrmDatabase and run a
			 * sanity check on it.
			 */
	
			AddDatabaseFileToLocaleList (file, &locales);
			XtFree (file);
		}
		closedir (dirp);
	}

	/*
	 * Make sure we have the "C" locale, and that it's first
	 * in the list. If we don't have it yet, use its hard-wired
	 * definition.
	 *
	 * All other locales are put in alphabetical location (using
	 * "C" lexical sort).
	 */

	/*
	 * If the .Xdefaults file doesn't exist then
	 * the user must have removed the file and we are in the process of
	 * reconstructing the file.  Therefore the value of the
	 * default locale should be taken from $LANG.
	 */
	name = NULL;
	if (stat(GetResourceFilename(), &status) != 0) {
		name = getenv("LANG");
	}
	if (name == NULL) {
		name = defaultLocaleName;
	}

	p = FindLocale(name);

	if (p) {
		locale = *p;
		n = p - &_OlArrayElement(&locales, 0);
		if (n != 0) {
			_OlArrayDelete (&locales, n);
			_OlArrayInsert (&locales, 0, locale);
		}
	}
	else {
		locale.db = XrmGetStringDatabase(defaultLocale);
		xnlLanguage = GetResource(locale.db, &resources[XNLLANGUAGE]);
		if (xnlLanguage) {
			ParseNameAndLabel (
				xnlLanguage, &locale.name, &locale.label
			);
			_OlArrayInsert (&locales, 0, locale);
		}
	}

	/*
	 * The above work gave us the "C" locale in "locale".
	 *
	 * First, we add the name of the default (factory) locale
	 * (duh, must be "C"!) to the global resources. This keeps
	 * all other specific and supplementary locale resources
	 * out of the global set, except those fetched from the
	 * user's resource set (after Import). Then, in Export,
	 * we look for missing specific and supplementary resources
	 * and fill them in from the locale (either the default
	 * locale or the locale specified by the user through his/her
	 * resources).
	 *
	 * Second, we construct the balance of the factory resources,
	 * for use if the user presses the Reset to Factory button.
	 */

	r.name = _current[XNLLANGUAGE].name;
	r.value = XtNewString(locale.name);
	list_append (&factory, &r, 1);

	/* for compatibility with Sun we need to write "basicLocale" also */
	r.name = _current[BASICLOCALE].name;
	r.value = XtNewString(locale.name);
	list_append (&factory, &r, 1);

	merge_resources (&global_resources, &factory);

	for (n = 0; n < XtNumber(resources); n++) {
		String			value;
		String			comma;
		String			name;
		String			label;

		if (n == XNLLANGUAGE) {
			continue;
		}

		r.name = _current[n].name;
		/*
		 * The value we fetch here may be multiple-choice,
		 * so parse out the first choice as the default
		 * (``factory'') value.
		 */
		if (!(value = GetResource(locale.db, &resources[n]))) {
			r.value = XtNewString("");
		}
		else {
			if (n == SANS_FAMILYFONTLIST ||
				n == SERIF_FAMILYFONTLIST ||
				n == MONO_FAMILYFONTLIST) {
				r.value = value;
				XtFree (name);
			}
			else {
				comma = MultiCodesetStrchrWithQuotes(
					value, ','
				);
				if (comma) {
					*comma = 0;
				}
				ParseNameAndLabel (value, &name, &label);

				/*
				 * The factory stuff never needs freeing.
				 */
				r.value = name;
				XtFree (label);
				if (comma) {
					*comma = ',';
				}
			}
		}

		/*
		 * WARNING: Make sure the factory list gets created
		 * in the same order as the current list, so that a
		 * given index fetches the same resource value from
		 * either.
		 */
		list_append (&factory, &r, 1);
	}

	return;
}

/*
 * Export()
 */

static void
Export (XtPointer closure)
{
	Locale *		locale;
	Cardinal		n;
	
	/*
	 * Fetch the current locale, for use below.
	 */
	CurrentLocale = SetCurrent(XNLLANGUAGE, GLOBAL(XNLLANGUAGE), True);

	/*
	 * MORE: Watch out! User may have given screwy value for
	 * the locale.
	 */
	locale = FindLocale(CurrentLocale);
	/*
	 * Fetch the ``real'' current values from the RESOURCE_MANAGER
	 * database, but note that they may not be there.
	 */
	for (n = 1; n < XtNumber(resources); n++) {
		if (GLOBAL(n)) {
			/* MORE: Make sure the value is correct. */
			SetCurrent (n, GLOBAL(n), True);
		}
		else if (n == BASICLOCALE) {
			/* For compatibility with Sun, we need to write */
			/* "basicLocale". */
			SetCurrent(n, CURRENT(XNLLANGUAGE), True);
		}
		else {
			String			value;
			String			comma;
			String			name;

			/*
			 * The value we fetch here may be multiple-choice,
			 * so parse out the first choice as the default
			 * value (this is an arbitrary choice).
			 */
			if (locale &&
			    !(value = GetResource(locale->db,&resources[n]))) {
				SetCurrent (n, "", True);
			}
			else {
				if (n == SANS_FAMILYFONTLIST || 
					n == SERIF_FAMILYFONTLIST || 
					n == MONO_FAMILYFONTLIST ) {
					SetCurrent(n, value, True);
				}
				else {
					comma = MultiCodesetStrchrWithQuotes(
						value, ','
					);
					if (comma) {
						*comma = 0;
					}

					ParseNameAndLabel (
						value, &name, (String *)0
					);
					SetCurrent (n, name, False);
					if (comma) {
						*comma = ',';
					}
				}
			}
		}
	}

	merge_resources(&global_resources, &current);

	return;
}

/*
 * Create()
 */

static void
Create (Widget work, XtPointer closure)
{
	Locale *		locale;


	/*
	 * Find the current locale in the list of locales.
	 */
	locale = FindLocale(CURRENT(XNLLANGUAGE));
	if (!locale) {
		locale = &_OlArrayElement(&locales, 0);
	}

	/*
	 * Keep the list from being made wider than it needs to be.
	 */
	
	CreateLocaleListView (locale, work);
	CreateSettingsView (locale, work);

	return;
}

static void
CallbackPopdown(Widget w, XtPointer clientData, XtPointer callData)
{
	XtPopdown(DtGetShellOfWidget(w));
	((void(*)())clientData)();
}

static Notice	notice = {
	TXT_PREFERENCES_FOLDER,
	TXT_fixedString_change_locale,
	CallbackPopdown
};

/*
 * ApplyCB()
 */

static ApplyReturn *
ApplyCB (Widget w, XtPointer closure)
{
	static ApplyReturn	ret;
	Locale **		p;
	String			value;
	int				i;

	ret.reason = APPLY_OK;

	SetChangeBarState (
		XnlLanguage.changebar, 0, WSM_NONE, False, XnlLanguage.change
	);
	
	XtVaGetValues(
		XnlLanguage.list, 
		XmNuserData,	&p,
		NULL
	);
	
	PrevLocale = strdup(CurrentLocale);
	CurrentLocale = SetCurrent(
		XNLLANGUAGE, (ADDR)(p[SelectedItemNumber]->name), True
	);

	if (strcmp(PrevLocale, CurrentLocale) == 0) {
		return &ret;
	}

	ret.reason = APPLY_NOTICE;
	ret.u.notice = &notice;
	localeProperty.exit = Exit;

#define _APPLY(abbrev,index) \
	if (abbrev.exclusive.current_item->addr) { \
		SetCurrent (						\
			index, (ADDR)abbrev.exclusive.current_item->addr, True\
		);							\
	}								\
	else {								\
		SetCurrent (index, "", True);				\
	}

	_APPLY (DisplayLang,	DISPLAYLANG);
	_APPLY (InputLang,	INPUTLANG);
	_APPLY (NumericFormat,	NUMERICFORMAT);
	_APPLY (DateTimeFormat, DATETIMEFORMAT);
	_APPLY (FontGroup,	FONTGROUP);
	_APPLY (InputMethod,	INPUTMETHOD);

	if(
		FontGroup.exclusive.items &&
		(value = GetResource(
			p[SelectedItemNumber]->db,
			&resources[FONTGROUPDEF]
		))
	) {
		SetCurrent (FONTGROUPDEF, value, True);
	}
	else {
		SetCurrent (FONTGROUPDEF, "", True);
	}
	
	if( InputMethod.exclusive.items) {
		SetCurrent (
			IMSTATUS,
			InputMethodPartner(
				p[SelectedItemNumber],
				CURRENT(INPUTMETHOD), IMSTATUS
			),
			False
		);
		SetCurrent (
			FRONTENDSTRING,
			InputMethodPartner(
				p[SelectedItemNumber],
				CURRENT(INPUTMETHOD), FRONTENDSTRING
			),
			False
		);
	}
	
	/* for compatibility with Sun, we need to write "basicLocale" also */

	SetCurrent(BASICLOCALE, CURRENT(XNLLANGUAGE), True);

	/* Update the family font list */

	for (i=SANS_FAMILYFONTLIST; i<=MONO_FAMILYFONTLIST; i++){
		value = GetResource( p[SelectedItemNumber]->db,&resources[i]);
		if (value) {
			SetCurrent (i, value, True);
		}
		else {
			SetCurrent (i, "", True);
		}
	}
	return &ret;

#undef	_APPLY
}

/*
 * ResetCB()
 */

static void
ResetCB(Widget w, XtPointer closure)
{
	Reset (FindLocale(CURRENT(XNLLANGUAGE)), &current);
	return;
}

/*
 * FactoryCB()
 */

static void
FactoryCB(Widget w, XtPointer closure)
{
	Reset (FindLocale(defaultLocaleName), &factory);
	return;
}

/*
 * PopdownCB()
 */

static void
PopdownCB(Widget w, XtPointer closure)
{
	FreeExclusive (&DisplayLang.exclusive);
	FreeExclusive (&InputLang.exclusive);
	FreeExclusive (&NumericFormat.exclusive);
	FreeExclusive (&DateTimeFormat.exclusive);
	FreeExclusive (&FontGroup.exclusive);
	FreeExclusive (&InputMethod.exclusive);

	/*
	 * Do this to (1) clean up stuff we may not need for a long
	 * time and (2) to force a re-open when we need it again.
	 *
	 * CAUTION:
	 * (2) is necessary for now, since the dynamic resource handling
	 * in Xol doesn't support multiple displays.
	 */
	if (SeparateDisplayForNewLocale) {
		CloseDisplay (SeparateDisplayForNewLocale);
	}
	
	/*
	 * Remove reference to the view widget.  It is
	 * destroyed when it is Popped down.
	 */
	SettingsView = 0;

	return;
}

/*
 * SetCurrent()
 */

static String
SetCurrent(Cardinal index, String value, Boolean copy)
{
	if (CURRENT(index)) {
		XtFree (CURRENT(index));
	}
	return CURRENT(index) = (copy? XtNewString(value) : value);
}

/*
 * FetchFileSearchPath()
 */

static void
FetchFileSearchPath(void)
{
	static String		language_spec_format = "%l_%t.%c";
	Display *		display;
	Cardinal		language_spec_format_length;
	String			p;
	extern char *		getenv();


	/*
	 * We need: The given or default value for XFILESEARCHPATH.
	 * We can't use XtResolvePathname(), because it provides
	 * a language substitution for %L and %l that we can't
	 * override. We need to call XtFindFile() directly, and
	 * this requires knowing the correct path.
	 *
	 * Problem: We don't know the correct path to pass to
	 * XtFindFile(). If XFILESEARCHPATH is defined in the
	 * environment, great; but if it isn't, the Intrinsics
	 * have an implementation-specific default that isn't
	 * publically known.
	 *
	 * Solution: We deduce the default value by probing the
	 * Intrinsics with repeated calls to XtResolvePathname().
	 */

	if (FileSearchPath) {
		return;
	}
	else if ((FileSearchPath = getenv("XFILESEARCHPATH"))) {
		FileSearchPath = XtNewString(FileSearchPath);
		return;
	}

	/*
	 * Reopen the current display under a new application
	 * context, but provide a bogus language specification
	 * through the argv/argc parameters. Because of the bogus
	 * values for the language and the application class, the
	 * resource database creation overhead should be minimal.
	 */
	display = OpenDisplayForNewLocale(language_spec_format);
	XtResolvePathname (
		display,
		"%T", "%N", "%S",	/* type, filename, suffix */
		(String)0,		/* use default path */
		(Substitution)0, (Cardinal)0,
		AccumulateFileSearchPath
	);
	CloseDisplay (display);

	p = FileSearchPath;
	language_spec_format_length = strlen(language_spec_format);
	while ((p = FindSubString(p, language_spec_format))) {
		/*
		 * Each time we find %l_%t.%c (language_spec_format)
		 * in the path, we replace it--in place--with %L.
		 * Since the %L is shorter than %l_%t.%c, we simply
		 * overlay %L on top of %l and collapse the rest of
		 * the path over the _%t.%c.
		 */
		sprintf (p, "%s%s", "%L", p + language_spec_format_length);
		p += 2;	/* skip the %L */
	}

	return;
}

/*
 * FindSubString()
 */

static String
FindSubString(String src, String substring)
{
	Cardinal		n	= strlen(substring);

	while (*src) {
		if (strncmp(src, substring, n) == 0) {
			return src;
		}
		src++;
	}
	return 0;
}

/*
 * StringCompare()
 */

static int
StringCompare(String *a, String *b)
{
	return strcmp(*a, *b);
}

/*
 * AccumulateFileSearchPath()
 */

static Boolean
AccumulateFileSearchPath(String filename)
{
	String			newpath;
	Cardinal		curlen;


	if (!FileSearchPath) {
		FileSearchPath = XtNewString(filename);
	}
	else {
		curlen = strlen(FileSearchPath);
		FileSearchPath = XtRealloc(
			FileSearchPath, curlen + 1 + strlen(filename) + 1
		);
		sprintf (FileSearchPath + curlen, ":%s", filename);
	}
		
	return False;	/* keep the ``search'' going */
}

/*
 * AccumulateParentLocaleDirectories()
 */

static Boolean
AccumulateParentLocaleDirectories(String filename)
{
	String			locale;
		
	if ((locale = FindSubString(filename, "%L"))) {
		*locale = 0;
		/*
		 * Can't use _OlArrayUniqueAppend below, because we need
		 * to malloc a copy of filename once we know we should add
		 * it to the list.
		 */
		if (
			IsDir((String)0, filename) && 
			(_OlArrayFind(&ParentLocaleDirectories, filename)
			== _OL_NULL_ARRAY_INDEX)
		) {
			String f = XtNewString(filename);
		
			_OlArrayAppend (&ParentLocaleDirectories, f);
			*locale = '%';
			AccumulateFileSearchPath (filename);
		} 
		else {
			*locale = '%';
		}
	}

	return False;	/* keep the ``search'' going */
}

/*
 * IsDir()
 */

static Boolean
IsDir(String parent, String file)
{
	char			buffer[128];
	String			name = 0;
	struct stat		status;
	Boolean			ret;

	if (parent) {
		Cardinal len = strlen(parent) + 1 + strlen(file) + 1;
		name = len <= sizeof(buffer)? buffer : XtMalloc(len);
		sprintf (name, "%s/%s", parent, file);
	}
	else {
		name = file;
	}

	ret = (
		access(name, (R_OK|X_OK)) == 0 &&
		stat(name, &status) == 0 &&
		(status.st_mode & S_IFDIR) == S_IFDIR
	);

	if (name != file && name != buffer) {
		XtFree (name);
	}
		
	return ret;
}

/*
 * AddDatabaseFileToLocaleList()
 */

static void
AddDatabaseFileToLocaleList(String file, LocaleArray *locales)
{
	String			xnlLanguage;
	Locale			locale;

	/*
	 * A valid locale definition file looks like an XrmDatabase.
	 */
	locale.db = XrmGetFileDatabase(file);
	if (!locale.db) {
		return;
	}

	/*
	 * Found a real database; skip it if it doesn't
	 * contain at least a basicLocal resource.
	 */
	xnlLanguage = GetResource(locale.db, &resources[XNLLANGUAGE]);
	if (!xnlLanguage) {
		return;
	}

	/*
	 * We're on a roll! Found a real locale database;
	 * skip it if it identifies the same locale we
	 * already know about.
	 */
	ParseNameAndLabel (xnlLanguage, &locale.name, &locale.label);
	if (FindLocale(locale.name)) {
		XtFree (locale.name);
		XtFree (locale.label);
	} 
	else {
		_OlArrayOrderedInsert (locales, locale);
	}

	return;
}

/*
 * GetResource()
 */

static String
GetResource(XrmDatabase db, ResourceName * pname)
{
	static XrmQuark		QString = 0;
	static XrmQuark		class[2] = { 0 , 0 };
	static XrmQuark		name[2] = { 0 , 0 };
	XrmQuark		type;
	XrmValue		value;

	if (!pname->quark) {
		pname->quark = XrmStringToQuark(pname->name);
	}
	if (!QString) {
		QString = XrmStringToQuark("String");
	}
	if (class[0] == 0) {
		class[0] = XrmUniqueQuark();
	}

	name[0] = pname->quark;
	XrmQGetResource (db, name, class, &type, &value);

	if (type != QString) {
		return 0;	/* Huh? */
	}
	else {
		return (String)value.addr;
	}
}

/*
 * _MultiCodesetStrchr()
 */

static String
_MultiCodesetStrchr(String str, int c, int quote)
{
	String			p;
	Boolean			within_quotes	= False;

	for (p = str; *p; p++) {
		if (*p == '\\') {
			if (!*++p) {
				break;
			}
			else {
				continue;
			}
		}
		else if (*p == quote) {
			within_quotes = !within_quotes;
		}
		else if (!within_quotes && *p == c) {
			return p;
		}
	}
	return (c? 0 : p);
}

/*
 * ParseNameAndLabel()
 */

static void
ParseNameAndLabel(String value, String *pname, String *plabel)
{
	String		end;
	String		quote;
	Cardinal	len;

	/*
	 * Syntax is
	 *
	 *	name "xxx...xx"
	 *
	 * where the stuff in double quotes is the label. Note,
	 * though, that the ``stuff'' is in an arbitrary encoding!
	 */

	/*
	 * Strip leading spaces. Here's some induction for you,
	 * in an argument why we needn't worry about non-C codesets
	 * here: If very first character matches a space, then we
	 * can increment the character pointer by one byte and
	 * loop again. Next byte and subsequent bytes can be compared
	 * to whitespace and can be skipped if they match.
	 */
	if (value) {
		while (isspace(*value)) {
			value++;
		}
	}
	if (!value || !*value) {
		*pname = XtMalloc(1);
		(*pname)[0] = 0;
		if (plabel) {
			*plabel = XtMalloc(1);
			(*plabel)[0] = 0;
		}
		return;
	}

	/* Find the 1st double quote: */
	if ((quote = MultiCodesetStrchr(value, '"'))) {
		/* Find the 2nd double quote.  */
		end = MultiCodesetStrchr(quote+1, '"');
		/*
		 * If just one double quote was found, treat this as if
		 * a quote was found at the end.
		 */
		if (!end) {
			end = MultiCodesetStrchr(quote+1, 0);
		}

		/*
		 * Label starts just past the first quote
		 * and ends just before the last quote.
		 */
		len = end - (quote+1);
		if (plabel) {
			String	gt;

			gt = memcpy(XtMalloc(len+1), quote+1, len);
			gt[len] = 0;
			*plabel = XtNewString(GetGizmoText(gt));
			XtFree(gt);
		}
	} 
	else {
		/*
		 * No quote, therefore the label is the same as
		 * the name.
		 *
		 * Set things up so it looks as if a quote was
		 * found at the very end, so we can reuse the code
		 * that follows.
		 */
		if (plabel) {
			*plabel = 0; /* see below */
		}
		quote = MultiCodesetStrchr(value, 0);
	}

	/*
	 * Name ends at space(s) before the 1st quote (or before
	 * trailing space(s) if no quote.)
	 */
	while (isspace(quote[-1]) && quote > value) {
		quote--;
	}
	len = quote - value;
	*pname = memcpy(XtMalloc(len+1), value, len);
	(*pname)[len] = 0;
	if (plabel && !*plabel) {
		*plabel = XtNewString(*pname);
	}

	return;
}

/*
 * FindLocale()
 */

static Locale *
FindLocale(String locale)
{
	Cardinal		n;

	/*
	 * Can't use _OlArrayFind, because the "natural" compare function
	 * compares labels.
	 */
	for (n = 0; n < _OlArraySize(&locales); n++) {
		if (MATCH(locale, _OlArrayElement(&locales, n).name)) {
			return &_OlArrayElement(&locales, n);
		}
	}
	return 0;
}

/*
 * LocaleCompare()
 */

static int
LocaleCompare(Locale *pa, Locale *pb)
{
	return strcmp(pa->label, pb->label);
}

/*
 * CreateLocaleListView()
 */

static void
CreateLocaleListView( Locale *locale, Widget parent)
{
	Widget			label;
	Widget			sw;
	LocaleListView *	pv	= &XnlLanguage;
	Locale *		p;
	Display *		dpy = XtDisplayOfObject(parent);
	Cardinal		nitems;
	Cardinal		selectedCnt = 0;
	XmString *		itemsNew;
	XmString *		selectNew;
	int			i;
	int			elements;
	Locale **		items;
	
	elements = _OlArraySize(&locales);		
	
	items = (Locale **) calloc(_OlArraySize(&locales), sizeof(Locale *));
	itemsNew = (XmString *) malloc(elements * sizeof(XmString));
	selectNew = (XmString *) malloc(elements * sizeof(XmString));
	
	for (nitems = 0; nitems < _OlArraySize(&locales); nitems++) {
		Locale *	p = &_OlArrayElement(&locales, nitems);
		
		itemsNew[nitems] = XmStringCreateLocalized(p->label);
		items[nitems] = p;
		
		if (p == locale) {
			selectNew[selectedCnt] = XmStringCreateLocalized(
				p->label
			);
			++selectedCnt;
			pv->current = nitems;
		}
		p->item = nitems;
	}
	
	label = CreateCaption(
		pv->name, pGGT(TXT_fixedString_basicSet), parent
	);
	XtVaSetValues(
		label,
		XmNleftOffset,		20,
		NULL
	);
	
	pv->cap = XtParent(label);
	XtVaSetValues(
		pv->cap,
		XmNleftAttachment,	XmATTACH_FORM,
		NULL
	);

	sw = XtVaCreateManagedWidget(
		"sw", xmScrolledWindowWidgetClass, pv->cap, NULL
	);
	AddToCaption(sw, label);
	
	pv->list = XtVaCreateManagedWidget(
		"list", xmListWidgetClass, sw,
		XmNitems,		itemsNew,
		XmNitemCount,		nitems,
		XmNselectionPolicy,	XmSINGLE_SELECT,
		XmNselectedItems,	selectNew,
		XmNselectedItemCount,	selectedCnt,
		XmNuserData,		items,
		XmNvisibleItemCount, 	3,
		NULL
	);
	XmListSetKbdItemPos(pv->list, pv->current+1);

	pv->changebar = (ChangeBar *) calloc(1, sizeof(ChangeBar));
	pv->change = TouchChangeBarDB;
	CreateChangeBar(pv->cap, pv->changebar);
	XtAddCallback(
		pv->list, XmNsingleSelectionCallback,
		LocaleSelectCB, (XtPointer) pv
	);
	
	/* Free memory */
	for(i = 0; i < nitems; i++) {
		XmStringFree(itemsNew[i]);
	}
	free(itemsNew);
	for(i = 0; i < selectedCnt; i++) {
		XmStringFree(selectNew[i]);
	}
	free(selectNew);
	
	return;
}

/*
 * LocaleSelectCB()
 */

static void
LocaleSelectCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	LocaleListView *	pv = (LocaleListView *)client_data;
	Locale *		locale;
	
	SelectedItemNumber = (
		((XmListCallbackStruct *)call_data)->item_position - 1
	);
	
	/* Get locale from selected item position */
	locale = &_OlArrayElement(&locales, SelectedItemNumber);

	NewLocale (pv, locale, WSM_NORMAL);

	return;
}

/*
 * Reset()
 */

static void
Reset(Locale *p, List *resource_list)
{
	Locale *	current = FindLocale(CURRENT(XNLLANGUAGE));
	Locale *	deflt = FindLocale(defaultLocaleName);

	if (!p) {
		return;
	}

	/*
	 * Attempt to switch to this locale; if the attempt ``failed''
	 * (i.e. we're already looking at this locale), then reset
	 * the specific and supplemental settings. (If the attempt
	 * ``succeeded'' then it already switched to default specific
	 * and supplemental settings.)
	 */

	if (p == deflt && p != current) {
		localeProperty.changed = True;
		NewLocale(&XnlLanguage, p, WSM_NORMAL);
	}
	else {
		localeProperty.changed = False;
		NewLocale(&XnlLanguage, p, WSM_NONE);
	}
	
	TouchPropertySheets();

	XmListSetKbdItemPos(XnlLanguage.list, XnlLanguage.current+1);

	return;
}

/*
 * NewLocale()
 */

static Boolean
NewLocale(LocaleListView *pv, Locale *p, int change_state)
{
	Cardinal		new = p->item;
	Cardinal		current = pv->current;
	
	if (current == new) {
		return False;
	}
	
	SetChangeBarState ( pv->changebar, 0, change_state, True, pv->change);
	
	XmListSelectItem(pv->list, XmStringCreateLocalized(p->label), False);
	
	pv->current = new;

	/*
	 * Get rid of the old settings view and create a new one.
	 */
	CreateSettingsView (p, (Widget)0);

	return True;
}

/*
 * CreateSettingsView()
 */

static void
CreateSettingsView(Locale *locale, Widget parent)
{
	LocaleListView *	pv = &XnlLanguage;
	Display *		dpy;
	XFontStruct *		fs;
	Widget			label;
	Widget			w;
	XmString		string;
	
	/*
	 * On being called the first time, we save the view's widget ID.
	 * On being called again, we destroy the previous view and
	 * recreate it, using the same parent as before.
	 */
	if (SettingsView) {
		/*
		 * The XtUnmanageChild() keeps the old view from
		 * interfering with the geometry of the new view,
		 * XtDestroyWidget() just marks the widget for
		 * destruction, but does not remove it from a
		 * composite's list until we return to the main loop.
		 * Many composites (our own included, alas) don't ignore
		 * widgets being-destroyed when computing layout.
		 */
		parent = XtParent(SettingsView);
		XtUnmanageChild (SettingsView);
		XtDestroyWidget (SettingsView);
	}

	dpy = XtDisplayOfObject(parent);
	
	/*
	 * Populate the menus with the data from this locale.
	 */

#define _FETCH(abbrev,index) \
	FetchResourceAndParse (						\
		locale->db, &resources[index], &abbrev.exclusive	\
	);								\
	if (abbrev.exclusive.items) {					\
		SetCurrent (						\
			index, (ADDR)abbrev.exclusive.current_item->addr, True\
		); \
	}

	_FETCH (DisplayLang,	DISPLAYLANG);
	_FETCH (InputLang,	INPUTLANG);
	_FETCH (NumericFormat,	NUMERICFORMAT);
	_FETCH (DateTimeFormat,	DATETIMEFORMAT);
	_FETCH (FontGroup,	FONTGROUP);
	_FETCH (InputMethod,	INPUTMETHOD);
#undef	_FETCH

	/*
	 * The pane that holds the view starts out unmanaged, to
	 * avoid silly base-window bounce as widgets get added.
	 */
	SettingsView = XtVaCreateManagedWidget(
		"settingsView",
		xmFormWidgetClass,
		parent,
		XmNshadowThickness, (XtArgVal)0,
		XmNfractionBase, 20,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopOffset, 10,
		XmNtopWidget, pv->cap,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL
	);
	
	label = XtVaCreateManagedWidget(
		"specificSettings",
		xmLabelWidgetClass,
		SettingsView,
		XmNlabelString,
		XmStringCreateLocalized(pGGT(TXT_fixedString_specSetting)),
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 2,
		NULL
	);

	DisplayLang.label	= pGGT(TXT_fixedString_dispLang);
	InputLang.label		= pGGT(TXT_fixedString_inputLang);
	NumericFormat.label	= pGGT(TXT_fixedString_numFormat);
	DateTimeFormat.label	= pGGT(TXT_fixedString_dateTime);
	
	w = CreateLabeledText(&DisplayLang, SettingsView, label);
	w = CreateLabeledText(&InputLang, SettingsView, w);
	w = CreateLabeledText(&NumericFormat, SettingsView, w);
	w = CreateLabeledText(&DateTimeFormat, SettingsView, w);

	if (FontGroup.exclusive.items || InputMethod.exclusive.items) {
		string = XmStringCreateLocalized(
			pGGT(TXT_fixedString_suppSetting)
		);
	}
	else {
		string = XmStringCreateLocalized(" ");
	}
	label = XtVaCreateManagedWidget(
		"supplementarySettings", xmLabelWidgetClass,
		SettingsView, 
		XmNlabelString, string,
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, w,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 2,
		NULL
	);
	FontGroup.label = pGGT(TXT_fixedString_fontGroup);
	w = CreateLabeledText (&FontGroup, SettingsView, label);
	InputMethod.label = pGGT(TXT_fixedString_inputMethod);
	CreateLabeledText (&InputMethod, SettingsView, w);

	/*
	 * Now let the user see the view.
	 */
	
	XtManageChild(SettingsView);

	return;
}

/*
 * FetchResourceAndParse()
 */

static void
FetchResourceAndParse(XrmDatabase db, ResourceName *resource, Exclusive *pe)
{
	String			value;
	String			end;
	String			comma;
	String			name;
	String			label;
	ExclusiveItem		item;

	FreeExclusive (pe);

	pe->current_item = pe->default_item = 0;

	if (!(value = GetResource(db, resource)) || !*value) {
		return;
	}

	pe->items = alloc_List(sizeof(ExclusiveItem));

	do {
		comma = MultiCodesetStrchrWithQuotes(value, ',');
		if (comma) {
			*comma = 0;
		}
		ParseNameAndLabel (value, &name, &label);

		item.name = (XtArgVal)label;
		item.addr = (XtArgVal)name;
		list_append (pe->items, &item, 1);

		if (comma) {
			*comma = ',';
			value = comma + 1;
		}
	} while (comma);

	/*
	 * No particular rule for which is the current and the
	 * default, so make them the first item in the exclusive.
	 */
	pe->current_item =
	pe->default_item = (ExclusiveItem *)pe->items->entry;

	return;
}

/*
 * InputMethodPartner()
 */

static String
InputMethodPartner(Locale *locale, String im, Cardinal partner)
{
	String			im_xrm;
	String			im_partner_xrm;
	String			im_comma;
	String			im_partner_comma;
	String			ret	= 0;

	im_xrm = GetResource(locale->db, &resources[INPUTMETHOD]);
	im_partner_xrm = GetResource(locale->db, &resources[partner]);
	if (!im_xrm || !im_partner_xrm) {
		return 0;
	}

	do {
		String			name;

#define NULL_COMMA(comma,value) \
		if ((comma = MultiCodesetStrchrWithQuotes(value, ',')))	{ \
			*comma = 0; \
		}
#define	SKIP_COMMA(comma,value) \
		if (comma) {						\
			*comma = ',';					\
			value = comma + 1;				\
		}

		NULL_COMMA (im_comma, im_xrm);
		NULL_COMMA (im_partner_comma, im_partner_xrm);

		ParseNameAndLabel (im_xrm, &name, (String *)0);
		if (MATCH(name, im)) {
			ret = XtNewString(im_partner_xrm);
		}
		XtFree (name);

		SKIP_COMMA (im_comma, im_xrm);
		SKIP_COMMA (im_partner_comma, im_partner_xrm);

#undef	NULL_COMMA
#undef	SKIP_COMMA
	} while (im_comma && !ret);

	return ret;
}

/*
 * FreeExclusive()
 */

static void
FreeExclusive(Exclusive *exclusive)
{
	ExclusiveItem *		p;
	list_ITERATOR		I;

	if (exclusive->items) {
		I = list_iterator(exclusive->items);
		while ((p = (ExclusiveItem *)list_next(&I))) {
			if (p->name) {
				XtFree ((String)p->name);
			}
			if (p->addr) {
				XtFree ((String)p->addr);
			}
		}
		free_List (exclusive->items);
		exclusive->items = 0;
	}

	return;
}

/*
 * CreateLabeledText()
 */

static Widget
CreateLabeledText(Labeled *pa, Widget parent, Widget topWid)
{
	Widget		w = NULL;
	XmString	string1;
	XmString	string2;

	if (pa->exclusive.items) {
		pa->exclusive.addr = (ADDR)pa;
		pa->exclusive.f = NULL;

		string1 = XmStringCreateLocalized((String)pa->label);
		string2 = XmStringCreateLocalized(
			(String)pa->exclusive.current_item->name
		);
	}
	else {
		string1 = XmStringCreateLocalized(" ");
		string2 = XmStringCreateLocalized(" ");
	}
	w = XtVaCreateManagedWidget(
		"localeText",
		xmLabelWidgetClass,
		parent,
		XmNlabelString, string1,
		XmNalignment, XmALIGNMENT_END,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, topWid,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 10,	/* Center of form */
		NULL
	);
	XtVaCreateManagedWidget(
		"localeText",
		xmLabelWidgetClass,
		parent,
		XmNlabelString, string2,
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 11,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, topWid,
		NULL
	);
	
	return w;
}

/*
 * OpenDisplayForNewLocale()
 */

static String
LanguageProc(Display *display, String language, XtPointer client_data)
{
	return language;
}

static Display *
OpenDisplayForNewLocale(String locale)
{
	int			argc;
	String			name;
	String			class;
	String			argv[8];
	XtAppContext		app = XtDisplayToApplicationContext(DISPLAY);
#if	defined(XtSpecificationRelease) && XtSpecificationRelease >= 5
	XtLanguageProc		proc;
#endif


	if (CurrentLocale && MATCH(CurrentLocale, locale)) {
		return DISPLAY;
	}

	if (SeparateDisplayForNewLocale) {
		if (MATCH(SeparateLocale, locale)) {
			return SeparateDisplayForNewLocale;
		}
		CloseDisplay (SeparateDisplayForNewLocale);
	}

	proc = XtSetLanguageProc((XtAppContext)0, LanguageProc, (XtPointer)0);
	/*
	 * DON'T pass display string to XtOpenDisplay() as the second parameter.
	 * instead, put the display string in the arg list. This will force
	 * Xt to parse the arg list first, and so will set the language
	 * correctly.
	 */
	XtGetApplicationNameAndClass (DISPLAY, &name, &class);
	argc = (int)XtNumber(argv)-1;
	argv[0] = name;
	argv[1] = "-xnllanguage";
	argv[2] = locale;
	argv[3] = "-xrm";
	argv[4] = "*customization:%C";
	argv[5] = "-display";
	argv[6] = DisplayString(DISPLAY);
	argv[7] = 0;
	SeparateDisplayForNewLocale = XtOpenDisplay(
		app, NULL,
		name, class,
		(XrmOptionDescRec *)0, (Cardinal)0,
		&argc, argv
	);
	SeparateLocale = XtNewString(locale);

	XtSetLanguageProc(NULL, NULL, NULL);
	return SeparateDisplayForNewLocale;
}

/*
 * CloseDisplay()
 */

static void
CloseDisplay(Display *display)
{
	if (display != DISPLAY) {
		XtCloseDisplay (display);
		if (display == SeparateDisplayForNewLocale) {
			SeparateDisplayForNewLocale = 0;
			XtFree (SeparateLocale);
		}
	}

	return;
}

/*
 * FillInLangSubs()
 *
 * Stolen from Xt/Intrinsic.c
 */

static void
FillInLangSubs(Substitution subs, String string)
{
	Cardinal		len;
	String			p1;
	String			p2;
	String			p3;
	String			ch;
	String *		rest;

	if (!string || !string[0]) {
		subs[0].substitution =
		subs[1].substitution =
		subs[2].substitution =
		subs[3].substitution = 0;
		return;
	}

	len = strlen(string) + 1;
	subs[0].substitution = string;
	p1 = subs[1].substitution = XtMalloc(3*len);
	p2 = subs[2].substitution = subs[1].substitution + len;
	p3 = subs[3].substitution = subs[2].substitution + len;

	/*
	 * Everything up to the first "_" goes into p1.
	 * From "_" to "." in p2.  The rest in p3.
	 * If no delimiters, all goes into p1.
	 * We assume p1, p2, and p3 are large enough.
	 */
	*p1 = *p2 = *p3 = 0;

	ch = strchr(string, '_');
	if (ch) {
		len = ch - string;
		(void) strncpy(p1, string, len);
		p1[len] = 0;
		string = ch + 1;
		rest = &p2;
	} 
	else {
		rest = &p1;
	}

	/*
	 * "rest" points to where we put the first part.
	 */
	ch = strchr(string, '.');
	if (ch) {
		len = ch - string;
		strncpy(*rest, string, len);
		(*rest)[len] = '\0';
		(void) strcpy(p3, ch+1);
	} 
	else {
		(void) strcpy(*rest, string);
	}

	return;
}

/*
 *   TouchChangeBarDB-
 */

static void
TouchChangeBarDB()
{
	int j = 0;
	
	localeProperty.changed = True;
	TouchPropertySheets();
	
	return;
}

/*
 * Exit()
 */

static void 
Exit (XtPointer closure)
{
	struct passwd *		pw = getpwuid(geteuid());
	extern void		Dm__UpdateUserFile();
		
	merge_resources (&global_resources, &current);

	/* Set "setenv LANG %s" in the user's .login file. */
	Dm__UpdateUserFile(
		pw->pw_dir, CurrentLocale, LOGIN,
		"setenv LANG ", "[^	]+", DONOTEDIT
	);
	/* Set "LANG=%s" in the user's .profile file. */
	Dm__UpdateUserFile(
		pw->pw_dir, CurrentLocale, PROFILE,
		"LANG=", "[^ ]+", " export LANG" DONOTEDIT
	);
}
