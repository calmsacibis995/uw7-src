#ifndef NOIDENT
#ident	"@(#)dtadmin:print/use/utils.h	1.5.1.1"
#endif

#ifndef UTILS_H
#define UTILS_H

#define	APP_NAME	"PrtMgr"

typedef struct {
    XtArgVal	lbl;
    XtArgVal	mnem;
    XtArgVal	sensitive;
    XtArgVal	selectProc;
    XtArgVal	dflt;
    XtArgVal	userData;
    XtArgVal	subMenu;
} MenuItem;

typedef struct {
    XtArgVal	userData;
    XtArgVal	lbl;
} ButtonItem;

typedef struct {
    char	*title;
    char	*file;
    char	*section;
} HelpText;

typedef struct {
    Widget	btn;
    Cardinal	setIndx;
} BtnChoice;

typedef struct {
    Widget	txt;
    char	*setText;
} TxtChoice;

typedef Widget StaticTxt;

typedef struct {
    Widget	menuBtn;
    Widget	caption;
    Widget	preview;
    BtnChoice	buttons;
    ButtonItem	*items;
} AbbrevChoice;

typedef struct {
    Widget      caption;
    Widget      preview; 
    BtnChoice   buttons;
    ButtonItem  *items;
} ListChoice;

typedef struct {
    TxtChoice	value;
    BtnChoice	units;
} ScaledChoice;

extern char	*AppName;
extern char	*AppTitle;

extern void	FooterMsg (Widget, char *);
extern void	DisplayHelp (Widget, HelpText *);

extern void	SetLabels (MenuItem *, int);
extern void	SetButtonLbls (ButtonItem *, int);
extern void	SetHelpLabels (HelpText *);
extern void	MakeButtons (Widget, char *, ButtonItem *, Cardinal,
			     BtnChoice *);
extern void	MakeCheck (Widget, char *, BtnChoice *);
extern void	ResetCheck (BtnChoice *, Boolean);
extern void	ApplyCheck (BtnChoice *, Boolean *);
extern void	MakeAbbrevMenu (Widget, char *, AbbrevChoice *, ButtonItem *,
				int, int);
extern void	ApplyAbbrevMenu (AbbrevChoice *, Cardinal *);
extern void	ResetAbbrevMenu (AbbrevChoice *, Cardinal);
extern void	MakeText (Widget, char *, TxtChoice *, int);
extern void	MakeStaticText (Widget, char *, StaticTxt *, int);
extern void	ApplyText (TxtChoice *, char **);
extern char	*GetText (TxtChoice *);
extern void	MakeScaled (Widget, char *, ScaledChoice *);
extern void	ApplyScaled (ScaledChoice *, float *, char *);
extern void	ResetScaled (ScaledChoice *, float, char);
extern Boolean	CheckScaled (ScaledChoice *);
extern void	GetScaled (char *, SCALED *);

#endif /* UTILS_H */
