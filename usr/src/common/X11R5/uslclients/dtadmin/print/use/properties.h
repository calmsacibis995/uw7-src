#ifndef NOIDENT
#ident	"@(#)dtadmin:print/use/properties.h	1.12.1.2"
#endif

#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <lp.h>
#include <printers.h>

#include "utils.h"
#include "lpsys.h"

enum {
    Job_Submitted, Job_Canceled, Job_Error, No_Printer, Internal_Error,
};

enum {
    NoBanner, Length, Width, Cpi, Lpi, Locale, Unknown,
};

typedef struct {
    XtArgVal	lbl;
    XtArgVal	glyph;
    XtArgVal	x;
    XtArgVal	y;
    XtArgVal	width;
    XtArgVal	height;
    XtArgVal	selected;
    XtArgVal	properties;
    XtArgVal	managed;
} IconItem;

typedef struct {
    char	*printer;
    char	*dfltPrinter;
    char	*procIcon;
    char	*icon;
    Boolean	displayQueue;
    char	*contentType;
    char	*filterDir;
    char	*typesFile;
    int		timeout;
} ResourceRec, *ResourcesPtr;

typedef struct {
    char	*lbl;
    char	*help;
    char	*pattern;
    char	*origPattern;
    char	type;
} FilterData; 

typedef struct _Filter {
    char		*name;
    struct _Filter	*next;
    struct _Filter	*listNext;
    int			cnt;
    FilterData		(*data)[];
} Filter;

typedef union {
    TxtChoice		txtCtrl;
    BtnChoice		chkCtrl;
} FilterCtrl;

typedef struct {
    Filter	*filter;
    char	*inputType;
    char	*opts;
    char	*modes;
    Cardinal	locale;
    Cardinal	charSet;
    SCALED	pgLen;
    SCALED	pgWid;
    SCALED	cpi;
    SCALED	lpi;
} FilterOpts;

typedef struct {
    Widget		popup;
    Widget		optionsPopup;
    Widget		optionsCtrlArea;
    Widget		lca;
    struct _Properties	*posted;
    struct _Properties	*optionsOwner;
    Boolean		poppedUp;
    Boolean		popdownOK;
    Widget		footer;
    TxtChoice		copyCtrl;
    BtnChoice		mailCtrl;
    AbbrevChoice	charSetCtrl;
    ListChoice		localeCtrl;
    BtnChoice		bannerCtrl;
    TxtChoice		titleCtrl;
    AbbrevChoice	inTypeCtrl;
    TxtChoice		otherTypeCtrl;
    ScaledChoice	pgLenCtrl;
    ScaledChoice	pgWidCtrl;
    ScaledChoice	cpiCtrl;
    ScaledChoice	lpiCtrl;
    FilterCtrl		*filterCtrls;
    char		**filterOptions;
    Cardinal		numOptions;
    FilterOpts		workFilter;
    StaticTxt		idCtrl;
    StaticTxt		userCtrl;
    StaticTxt		sizeCtrl;
    StaticTxt		dateCtrl;
    StaticTxt		stateCtrl;
} PropPg;

typedef struct _Defaults {
    char		*inputType;
    char		*opts;
    char		*modes;
    char		*charSet;
    struct _Defaults	*next;
} Defaults;

typedef struct _Printer {
    char		*name;
    PRINTER		*config;
    ButtonItem		*charSetItems;
    int			numCharSets;
    ButtonItem		*localeItems;
    int			numLocales;
    Widget		iconbox;
    Widget		prtreqMenu;
    XtIntervalId	timeout;
    Defaults		*dflts;
    struct _Printer	*next;
    PropPg		page;
} Printer;

typedef struct _Properties {
    char	*id;
    Printer	*printer;
    PropPg	*page;
    REQUEST	*request;
    char	**files;
    char	*copies;
    Cardinal	mail;
    Cardinal	banner;
    Cardinal	inType;
    char	*otherType;
    char	*title;
    FilterOpts	originalFilter;
    FilterOpts	appliedFilter;
    char	*miscOpts;
    int		rank;
    char	*user;
    long	size;
    long	date;
    short	state;
} Properties;

extern XtAppContext	AppContext;
extern Widget		TopLevel;
extern String		MenuFields [];
extern int		NumMenuFields;

extern void	OpenPrinter (char *name);
extern void	PostProperties(Widget, Properties *);
extern void	InitProperties (Widget, Properties *, PrintJob *);
extern void	FreeProperties (Properties *);

extern void	BringDownPopup (Widget popup);
extern int	Lookup (char *);
extern void	Die (Widget, XtPointer, XtPointer);

#endif /* PROPERTIES_H */
