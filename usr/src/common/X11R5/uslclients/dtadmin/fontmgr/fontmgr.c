#ident	"@(#)dtadmin:fontmgr/fontmgr.c	1.73.1.2"
/*
 * Module:     dtadmin:fontmgr   Graphical Administration of Fonts
 * File:       fontmgr.c
 */

/*
*******************************************************************************
*   Include_files.
*******************************************************************************
*/

#include <stdio.h>
#include <signal.h>
#include <locale.h>
#include <Intrinsic.h>
#include <StringDefs.h>
#include <Shell.h>
#include <OpenLook.h>

#include <OlDnDVCX.h>

#include <Gizmos.h>
#include <BaseWGizmo.h>
#include <PopupGizmo.h>
#include <MenuGizmo.h>
#include <NumericGiz.h>
#include <STextGizmo.h>
#include <LabelGizmo.h>
#include <SpaceGizmo.h>
#include <X11/StringDefs.h>
#include <ChoiceGizm.h>

#include <RubberTile.h>
#include <ControlAre.h>
#include <Panes.h>
#include <Caption.h>
#include <StaticText.h>
#include <TextEdit.h>
#include <FList.h>
#include <ScrolledWi.h>
#include <Footer.h>
#include <Form.h>
#include <IntegerFie.h>

#include <Desktop.h>

#include <fontxlfd.h>
#include <fontmgr.h>

/*
*******************************************************************************
*   Application callback declaration (callbacks should be in a separate file)
*******************************************************************************
*/
Boolean CheckRegistryEncodingIso8859(char *lowername);
extern void callRegisterHelp(Widget, char *, char *);
extern void ErrorCancelCB();
extern Boolean ParseXLFD(view_type *view,String, xlfd_type **);
extern Boolean _IsXLFDFontName();
extern void CreateShowDPI();
extern void HelpCB();
extern void IntegrityCB();
extern int FontIsPartOfFamily();
extern int GetFontWeightSlant();
void OutlinePSCB();
void FamilySelectCB();
void StyleSelectCB();
void PSSelectCB();
void DisplayBitmapDeleteCB();
void DisplayOutlineDeleteCB();
void AddCB();
static Boolean CheckResource(char *filename, String res, Boolean value_needed);
static void GetCurrentLocale();
static char *SetFontDefaults();
extern void FreeFontsInFamily();
static void GetScreenResolutions();
static void ApplyDefaultFontCB();
static void ApplyToAllCB();
static void ApplyToXtermCB();
static void ApplyFooterPropCB();
static void ApplySamplePropCB();
void CreateFontDB();
void ResetFont();
void DnDPopupAddWindow();
void MainHelpTOCCB();
void MainHelpAboutCB();
static void ExitCB();
static Boolean	DropNotify OL_ARGS((Widget, Window, Position,
					  Position, Atom, Time,
					  OlDnDDropSiteID,
					  OlDnDTriggerOperation, Boolean,
					  Boolean, XtPointer));
static void	SelectionCB OL_ARGS((Widget, XtPointer, Atom *, Atom *,
					XtPointer, unsigned long *, int *));


#define CHARACTER_SPACING ", c"
#define MONO_SPACING  ", m"
#define POINT_SIZE_IN_RANGE(P) ((atoi(P->size) <= SAVE_MAX_SIZE) )
#define MAXWIDTH	480
#define MAXHEIGHT	161
#define LOCAL 0
#define REMOTE 1
#define DROP_RESOURCE "fontmgr"
#define ALL_FONT_RESOURCE_NAME "font"
#define SANS_SERIF_FONTLIST_RESOURCE_NAME1  "*sansSerifFamilyFontList"
#define SANS_SERIF_FONTLIST_RESOURCE_NAME  "sansSerifFamilyFontList"
#ifdef xxx
#define FONTLIST_RESOURCE_NAME  "fontList"
#endif
#define PLAIN_FONT "=PLAIN_FONT_TAG"
#define BOLD_FONT "=BOLD_FONT_TAG"
#define ITALIC_FONT "=ITALIC_FONT_TAG"
#define ITALIC_BOLD_FONT "=ITALIC_BOLD_FONT_TAG"
#define XTERM_FONT_RESOURCE_NAME "xterm*Font"
#define OLDEFAULT_FIXED_FONT_RESOURCE_NAME "olDefaultFixedFont"
#define ALL 1
#define MONOSPACED 2
#define ICON_FILE "font48.icon"
#define ClientClass          "fontmgr"

#define SAVE_MAX_SIZE 20
Widget       app_shellW;		  /* application shell widget       */
Widget base_shell;
static char *locale_encoding;
Boolean ISO8859_locale = FALSE;
Boolean install_allowed = FALSE;
Boolean ApplyAllowed=True;
static char *locale= NULL;
char *xwin_home;
char *home;
char font_value[1200];
_OlArrayType(FamilyArray) family_data;
view_type view_data = { &family_data };
view_type *view = &view_data;
static Xterm_set;

static Setting ps_gizmo_setting = {0, 0, 0, (XtPointer) DEFAULT_POINT_SIZE };
NumericGizmo ps_gizmo =
{0, 0, 0, MIN_PS_VALUE, MAX_PS_VALUE, &ps_gizmo_setting, 4};

static MenuItems delete_menu_item[] = {
{ TRUE, TXT_BITMAP_DDD, ACCEL_FILE_DELETE_BITMAP , 0, DisplayBitmapDeleteCB},
{ TRUE, TXT_OUTLINE_DDD,ACCEL_FILE_DELETE_OUTLINE,0, DisplayOutlineDeleteCB},
{ NULL }
};

static MenuGizmo delete_menu = {0, "d_menu", NULL, delete_menu_item};

#define ADD_BUT 0
#define DELETE_BUT 1
#define INTEGRITY_BUT 0
static MenuItems file_menu_item[] = {
{ TRUE, TXT_INTEGRITY, ACCEL_FILE_INTEGRITY, 0, IntegrityCB, 0},
{ TRUE, TXT_SHOW_DPI, ACCEL_FILE_SHOW, 0, CreateShowDPI, (char *) &view_data},
{ TRUE, TXT_EXIT,      ACCEL_FILE_EXIT     , 0, ExitCB, 0},
{ NULL }
};


#define RESTORE_DEFAULT 2
#define APPLY_WINDOWS  3
#define APPLY_XTERM 4

static MenuItems edit_menu_item[] = {  
{ TRUE, TXT_ADD_DDD,   ACCEL_FILE_ADD      , 0, AddCB, 0},
{ TRUE, TXT_DELETE,    ACCEL_FILE_DELETE   , (char *) &delete_menu, 0},
{ TRUE, TXT_APPLY_DEFAULT_FONT, ACCEL_EDIT_DEFAULT,0,
      ApplyDefaultFontCB, (char *) &view_data},
{ TRUE, TXT_APPLY_FONT_ALL,ACCEL_EDIT_ALL,0,
      ApplyToAllCB, (char *) &view_data},
{ TRUE, TXT_APPLY_FONT_XTERM,ACCEL_EDIT_TERMINAL,0,
      ApplyToXtermCB, (char *) &view_data},
{ NULL }
};

static HelpInfo help_apply_to_xterm = { 0, "", HELP_PATH, TXT_APPLY_XTERM_HELP_SECTION };
static HelpInfo help_apply_to_windows = { 0, "", HELP_PATH, TXT_APPLY_WINDOWS_HELP_SECTION };
static HelpInfo help_restore_default = { 0, "", HELP_PATH, TXT_RESTORE_DEFAULT_HELP_SECTION };
static HelpInfo help_wrong_locale = { 0, "", HELP_PATH, TXT_HELP_WRONG_LOCALE };
static HelpInfo help_wrong_charset = { 0, "", HELP_PATH, TXT_HELP_WRONG_CHARSET };
static HelpInfo help_pointsize_too_large = { 0, "", HELP_PATH, TXT_HELP_POINTSIZE_TOO_LARGE };
static HelpInfo help_must_be_monospaced = { 0, "", HELP_PATH, TXT_HELP_MUST_BE_MONOSPACED };
static HelpInfo help_about = { 0, "", HELP_PATH, TXT_MAIN_HELP_SECTION };
static HelpInfo help_TOC = { 0, "", HELP_PATH, "TOC" };
static HelpInfo help_desk = { 0, "", HELP_PATH, "HelpDesk" };
static HelpInfo help_main_menu = { 0, "", HELP_PATH, TXT_HELP_MAIN_MENU };

enum sample_constant { E_SHORT_SAMPLE, E_FULL_SAMPLE };
static MenuItems view_menu_item[] = {
{ TRUE, TXT_SHORT_SAMPLE, ACCEL_VIEW_DISPLAY_PHASE, 0,
      ApplySamplePropCB, (char *)E_SHORT_SAMPLE},
{ TRUE, TXT_FULL_SAMPLE, ACCEL_VIEW_DISPLAY_CHAR ,0,
      ApplySamplePropCB, (char *)E_FULL_SAMPLE},
{ TRUE,TXT_SHORT_NAME,ACCEL_VIEW_FONT_SHORT,0,
        ApplyFooterPropCB,(char*)FALSE},
{ TRUE, TXT_XLFD_NAME,ACCEL_VIEW_FONT_XLFD ,0,
        ApplyFooterPropCB, (char*)TRUE  },
{ NULL }
};

static MenuItems help_menu_item[] = {  
{ TRUE, TXT_HELP_ABOUT,ACCEL_HELP_FONT ,0, HelpCB, (char *) &help_about },
{ TRUE, TXT_HELP_TOC,  ACCEL_HELP_TABLE,0, HelpCB, (char *) &help_TOC },
{ TRUE, TXT_HELP_DESK, ACCEL_HELP_DESK ,0, HelpCB, (char *) &help_desk },
{ NULL }
};

static MenuGizmo file_menu = {0, "file_menu", NULL, file_menu_item};
static MenuGizmo view_menu = {0, NULL, NULL, view_menu_item};
static MenuGizmo edit_menu = {0, NULL, NULL, edit_menu_item};
static MenuGizmo help_menu = {0, "help_menu", NULL, help_menu_item};

#define EDIT_BUT 2
static MenuItems main_menu_item[] = {  
{ TRUE, TXT_FILE, ACCEL_BASE_FILE, (Gizmo) &file_menu},
{ TRUE, TXT_EDIT, ACCEL_BASE_EDIT, (Gizmo) &edit_menu},
{ TRUE, TXT_VIEW, ACCEL_BASE_VIEW, (Gizmo) &view_menu},
{ TRUE, TXT_HELP, ACCEL_BASE_HELP, (Gizmo) &help_menu},
{ NULL }
};
static MenuGizmo menu_bar = { &help_main_menu, "menu_bar", NULL,
    main_menu_item, 0, 0, 0, 0, 0, OL_NO_ITEM };

static MenuItems font_save_item[] = {
{ True, "1", "1" },
{ 0 }
};
static MenuGizmo   font_save_menu    = 
{ NULL, NULL, "_X_", font_save_item, NULL, NULL, EXC };

static Setting font_save_setting;
static ChoiceGizmo font_save_choice   = 
   { NULL, NULL, " ",  &font_save_menu, &font_save_setting };
static GizmoRec view_list[] = {
      { ChoiceGizmoClass,  &font_save_choice }
};

BaseWindowGizmo base = {0, "base", TXT_WINDOW_TITLE, (Gizmo)&menu_bar,
	view_list,
	XtNumber(view_list),
	TXT_ICON_TITLE, /* icon title */
	ICON_FILE,
	" ", " ", 100
    };


static void
ApplyFooterPropCB(Widget w,
		  XtPointer client_data,
		  XtPointer call_data)
{
    extern Boolean show_xlfd;

    show_xlfd = (Boolean) client_data;
    DisplayFont(view);

} /* end of ApplyFooterPropCB */


static void
ApplySamplePropCB(Widget w,
		  XtPointer client_data,
		  XtPointer call_data)
{
    if (E_SHORT_SAMPLE == (int)client_data)
	XtVaSetValues(view->sample_text,
		      XtNcursorPosition, 0,
		      XtNselectStart, 0,
		      XtNselectEnd, 0,
		      XtNsource, GetGizmoText(TXT_SAMPLE_TEXT), 0);
    else
	XtVaSetValues(view->sample_text,
		      XtNcursorPosition, 0,
		      XtNselectStart, 0,
		      XtNselectEnd, 0,
		      XtNsource, GetGizmoText(TXT_CHAR_SET), 0);

} /* end of ApplySamplePropCB */


static void
ApplyDefaultFontCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	char buf[MAX_PATH_STRING];

    /*
     * do dynamic update
     */
	GetCurrentLocale();
	if (ApplyAllowed == False) {
        PopupErrorMsg(TXT_BAD_LOCALE_4_APPLY,
			ErrorCancelCB,
			NULL,
			APPLY_FONT_TITLE,
			&help_restore_default,
			TXT_RESTORE_DEFAULT_HELP_SECTION);
			return;
		};
    font_save_choice.name = ALL_FONT_RESOURCE_NAME;
    font_save_menu.items[0].set = TRUE;
    /*font_save_menu.items[0].mod.resource_value = "olDefaultFont";*/
    font_save_menu.items[0].mod.resource_value = "-*-helvetica-medium-r-normal--*-120-*-*-p-*-iso8859-1";
    ManipulateGizmo(ChoiceGizmoClass, &font_save_choice, SetGizmoValue);

    font_save_choice.name = OLDEFAULT_FIXED_FONT_RESOURCE_NAME;
    font_save_menu.items[0].set = TRUE;
    font_save_menu.items[0].mod.resource_value = NULL;
    ManipulateGizmo(ChoiceGizmoClass, &font_save_choice, SetGizmoValue);

    font_save_choice.name = XTERM_FONT_RESOURCE_NAME;
    font_save_menu.items[0].set = TRUE;
    /*font_save_menu.items[0].mod.resource_value = NlucidaTypewriter;*/
    font_save_menu.items[0].mod.resource_value = olDefaultFixedFont;
    ManipulateGizmo(ChoiceGizmoClass, &font_save_choice, SetGizmoValue);

	/* get value for Motif default fonts in the 
		/usr/X/lib/locale/xxx/ol_locale_def file and use
		the value for *fontList and *sansSerifFamilyFontList */

	
    sprintf(buf, "%s/%s/%s/%s", xwin_home, "lib/locale", locale, "ol_locale_def");
	if ((CheckResource(buf, SANS_SERIF_FONTLIST_RESOURCE_NAME1, True)) == True) {
		
#ifdef DEBUG
	fprintf(stderr,"restoring sansSerifFamilyFontList to ol_locale_def value=%s\n", font_value);
#endif
    	font_save_choice.name = SANS_SERIF_FONTLIST_RESOURCE_NAME;
    	font_save_menu.items[0].set = TRUE;
    	font_save_menu.items[0].mod.resource_value =  font_value;
    	ManipulateGizmo(ChoiceGizmoClass, &font_save_choice, SetGizmoValue);
	} else {
		/* no resource set in ol_locale_def then just remove
		the resource from the .Xdefaults file */

#ifdef DEBUG
	fprintf(stderr,"restoring sansSerifFamilyFontList to NULL\n");
#endif
    	font_save_choice.name = SANS_SERIF_FONTLIST_RESOURCE_NAME;
    	font_save_menu.items[0].set = TRUE;
    	font_save_menu.items[0].mod.resource_value =  NULL;
    	ManipulateGizmo(ChoiceGizmoClass, &font_save_choice, SetGizmoValue);
	}


    Xterm_set = TRUE;
    InformUser(TXT_APPLIED_DEFAULT_FONT_TO_ALL);

} /* end of ApplyDefaultFontCB */


void
ApplyToAllCB(w, client_data, call_data)
    Widget w;
    XtPointer client_data;
    XtPointer call_data;
{
    view_type *view = (view_type *) client_data;
    xlfd_type *xlfd_info;

    Boolean result;
    char apply_xlfd_name[256];

	/* ISO8859-1 locales must has registry & encoding set to
		ISO8859-1 */

	GetCurrentLocale();
	if (ApplyAllowed == False) {
		PopupErrorMsg(TXT_BAD_LOCALE_4_APPLY,
			ErrorCancelCB,
			NULL,
			APPLY_FONT_TITLE,
			&help_wrong_locale,
			TXT_HELP_WRONG_LOCALE);
		return;
		};


	if (CheckRegistryEncodingIso8859(view->cur_xlfd)== False) {
		PopupErrorMsg(TXT_BAD_ISO8859_FONT,
			ErrorCancelCB,
			NULL,
			APPLY_FONT_TITLE,
			&help_wrong_charset,
			TXT_HELP_WRONG_CHARSET);
		return;
		};

    ParseXLFD(view, view->cur_xlfd, &xlfd_info);
    if (!POINT_SIZE_IN_RANGE(xlfd_info)) {
		PopupErrorMsg(TXT_POINT_SIZE_NOT_IN_RANGE,
			ErrorCancelCB,
			NULL,
			APPLY_FONT_TITLE,
			&help_pointsize_too_large,
			TXT_HELP_POINTSIZE_TOO_LARGE);
		return;
    };
    result = GetBitmapFontName(view->cur_xlfd, &apply_xlfd_name[0]);
    if (result == True) {
		strcpy(view->cur_xlfd, &apply_xlfd_name[0]);
	
    }
    SetMotifFontResources(view);

    font_save_choice.name = ALL_FONT_RESOURCE_NAME;
    font_save_menu.items[0].set = TRUE;
    font_save_menu.items[0].mod.resource_value = view->cur_xlfd;

    ManipulateGizmo(ChoiceGizmoClass, &font_save_choice, SetGizmoValue);

    /*
     * set xterm font resource
     */
    if (!Xterm_set) {
    
	font_save_choice.name = XTERM_FONT_RESOURCE_NAME;
	font_save_menu.items[0].set = TRUE;
	font_save_menu.items[0].mod.resource_value = olDefaultFixedFont;
	ManipulateGizmo(ChoiceGizmoClass, &font_save_choice, SetGizmoValue);
	Xterm_set = TRUE;
    }
    InformUser(TXT_APPLIED_FONT_TO_ALL);

} /* end of ApplyToAllCB */


void
ApplyToXtermCB(w, client_data, call_data)
    Widget w;
    XtPointer client_data;
    XtPointer call_data;
{
    view_type *view = (view_type *) client_data;
    xlfd_type *xlfd_info;
    char apply_xlfd_name[256];

    ParseXLFD(view, view->cur_xlfd, &xlfd_info);

	/* ISO8859-1 locales must has registry & encoding set to
		ISO8859-1 */

	GetCurrentLocale();
	if (ApplyAllowed == False) {
        PopupErrorMsg(TXT_BAD_LOCALE_4_APPLY,
			ErrorCancelCB,
			NULL,
			APPLY_FONT_TITLE,
			&help_wrong_locale,
			TXT_HELP_WRONG_LOCALE);
			return;
		};
	if (CheckRegistryEncodingIso8859(view->cur_xlfd) == False) {
		PopupErrorMsg(TXT_BAD_ISO8859_FONT,
			ErrorCancelCB,
			NULL,
			APPLY_FONT_TITLE,
			&help_wrong_charset,
			TXT_HELP_WRONG_CHARSET);
			return;
		};

    if (!POINT_SIZE_IN_RANGE(xlfd_info)) {
		PopupErrorMsg(TXT_POINT_SIZE_NOT_IN_RANGE, 
			ErrorCancelCB,
			NULL,
			APPLY_FONT_TITLE,
			&help_pointsize_too_large,
			TXT_HELP_POINTSIZE_TOO_LARGE);
		return;
    };
/*
        spacing  set in font_arena.c to "" or ", cell-spaced" or
                ", mono-spaced"
		and it is a bitmap font
 */


    if ((xlfd_info->spacing != NULL) && 
    ((strncmp(xlfd_info->spacing, CHARACTER_SPACING, 3) == 0) ||
    (strncmp(xlfd_info->spacing, MONO_SPACING, 3) == 0)) ) { 
		/* skip to setting new resources */
		;
	} else  {
		PopupErrorMsg(TXT_BAD_XTERM_FONT, 
			ErrorCancelCB,
			NULL,
			APPLY_FONT_TITLE,
			&help_must_be_monospaced,
			TXT_HELP_MUST_BE_MONOSPACED);
			return;
	} 

	/* do not allow scalable fonts to be applied to xterm - this
	causes problems in xterm with the space character and the
	cursor. backspacing etc. does not work properly */
    	if (xlfd_info->bitmap == False) {
		PopupErrorMsg(TXT_BAD_XTERM_FONT2, 
		ErrorCancelCB,
		NULL,
		APPLY_FONT_TITLE,
		&help_must_be_monospaced,
		/* no new help for this error message */
		TXT_HELP_MUST_BE_MONOSPACED);
		return;
	}
	font_save_choice.name = OLDEFAULT_FIXED_FONT_RESOURCE_NAME;
	font_save_menu.items[0].set = TRUE;
	font_save_menu.items[0].mod.resource_value = view->cur_xlfd;
		/* see if there is a bitmap to apply */
   		if ((GetBitmapFontName(view->cur_xlfd, 
		&apply_xlfd_name[0])) == True) {
               		strcpy(view->cur_xlfd, &apply_xlfd_name[0]);
   		}
	ManipulateGizmo(ChoiceGizmoClass, &font_save_choice, SetGizmoValue);
	font_save_choice.name = XTERM_FONT_RESOURCE_NAME;
	font_save_menu.items[0].set = TRUE;
	font_save_menu.items[0].mod.resource_value = view->cur_xlfd;
	ManipulateGizmo(ChoiceGizmoClass, &font_save_choice, SetGizmoValue);
	Xterm_set = TRUE;
	InformUser(TXT_APPLIED_FONT_TO_XTERM);
} /* end of ApplyToXtermCB */


static void
ExitCB() 
{
    exit(0);
}


static Widget
CreateCaption(Widget parent, Widget ref, String str)
{
    Arg largs[15];
    Cardinal n;
    Widget rubber;

    n = 0;
    if (parent != ref) {
	XtSetArg(largs[n], XtNrefWidget, ref);			n++;
    }
    XtSetArg(largs[n], XtNrefPosition, OL_RIGHT);		n++;
    XtSetArg(largs[n], XtNweight, 0);         n++;
    XtSetArg(largs[n], XtNalignment, OL_CENTER);		n++;
    XtSetArg(largs[n], XtNstring, str);			n++;
    XtSetArg(largs[n], XtNwrap, FALSE);			n++;
    rubber = XtCreateManagedWidget("caption", 
				   staticTextWidgetClass, parent, largs, n);
    return rubber;
} /* end of CreateCaption */


static void
CreateMainView(view)
    view_type *view;
{
    Widget parent = view->form;
    Widget scroll_win, box_parent, family_sw, style_sw;
    Arg largs[15];
    Cardinal n;
    static String dummy[] = {" ", NULL};
    static String item_fields[] = {XtNlabel, XtNuserData};

    n = 0;
    XtSetArg(largs[n], XtNshadowThickness, 0);		n++;
    XtSetArg(largs[n], XtNwidth, MAXWIDTH);		n++;
    XtSetArg(largs[n], XtNlayoutWidth, OL_MINIMIZE);		n++;
    XtSetArg(largs[n], XtNlayoutHeight, OL_IGNORE);		n++;
    box_parent = XtCreateManagedWidget("box_parent", 
	 panesWidgetClass, parent, largs, n);

    n = 0;
    XtSetArg(largs[n], XtNrefPosition, OL_BOTTOM);			n++;
    scroll_win = XtCreateManagedWidget("ts",
          scrolledWindowWidgetClass, parent, largs, n);
    n = 0;
    XtSetArg(largs[n], XtNwrapMode, OL_WRAP_ANY);		n++;
    XtSetArg(largs[n], XtNsource, GetGizmoText(TXT_SAMPLE_TEXT));	n++;
    XtSetArg(largs[n], XtNlinesVisible, 7);	n++;
    XtSetArg(largs[n], XtNcharsVisible, 30);	n++;
    XtSetArg(largs[n], XtNcontrolCaret, False);	n++;

    /*
     * fontGroup needs to be rethinked
     */
    /* comment out - breaks display of characters in Japanese locale */
    /*XtSetArg(largs[n], XtNfontGroup, NULL);	n++;*/

    XtSetArg(largs[n], XtNwidth, MAXWIDTH);		n++;
    XtSetArg(largs[n], XtNheight, MAXHEIGHT);		n++;
    view->sample_text = XtCreateManagedWidget("sample",
		textEditWidgetClass, scroll_win, largs, n);

    /*
     * create labels
     */
    view->family_caption = CreateCaption(box_parent, box_parent,
				GetGizmoText(TXT_TYPEFACE_FAMILY));
    view->style_caption = CreateCaption( box_parent,view->family_caption,GetGizmoText(TXT_STYLE));
    view->size_caption = CreateCaption(box_parent, view->style_caption, GetGizmoText(TXT_POINT_SIZE));

    /*
     * create family
     */
    n = 0;
    XtSetArg(largs[n], XtNrefWidget, view->family_caption);			n++;
    family_sw = XtCreateManagedWidget("scroll_win",
	 scrolledWindowWidgetClass, box_parent, largs, n);
    n = 0;
    XtSetArg(largs[n], XtNitems, dummy);		n++;
    XtSetArg(largs[n], XtNnumItems, 1);	n++;
    XtSetArg(largs[n], XtNitemFields, item_fields);		n++;
    XtSetArg(largs[n], XtNnumItemFields, XtNumber(item_fields));	n++;
    XtSetArg(largs[n], XtNsameWidth, OL_ALL);	n++;
    XtSetArg(largs[n], XtNlabelJustify, OL_CENTER);	n++;
    XtSetArg(largs[n], XtNselectProc, FamilySelectCB);	n++;
    XtSetArg(largs[n], XtNclientData, view);			n++;
    XtSetArg(largs[n], XtNweight, 0);         n++;
    XtSetArg(largs[n], XtNmaintainView, TRUE);			n++;
    view->family_exclusive = XtCreateManagedWidget("families",
		flatListWidgetClass, family_sw, largs, n);

    /*
     * create style
     */
    n = 0;
    XtSetArg(largs[n], XtNrefWidget, view->style_caption);			n++;
    style_sw = XtCreateManagedWidget("style_sw",
          scrolledWindowWidgetClass, box_parent, largs, n);
    n = 0;
    XtSetArg(largs[n], XtNitems, dummy);		n++;
    XtSetArg(largs[n], XtNnumItems, 1);	n++;
    XtSetArg(largs[n], XtNitemFields, item_fields);	n++;
    XtSetArg(largs[n], XtNnumItemFields, XtNumber(item_fields));	n++;
    XtSetArg(largs[n], XtNsameWidth, OL_ALL);	n++;
    XtSetArg(largs[n], XtNlabelJustify, OL_CENTER);	n++;
    XtSetArg(largs[n], XtNselectProc, StyleSelectCB);	n++;
    XtSetArg(largs[n], XtNclientData, view);			n++;
    XtSetArg(largs[n], XtNuserData, view->font_state[LOOK]);n++;
    XtSetArg(largs[n], XtNmaintainView, TRUE);			n++;
    XtSetArg(largs[n], XtNviewHeight, 5);			n++;
    XtSetArg(largs[n], XtNweight, 0);         n++;
    view->style_exclusive = XtCreateManagedWidget("styles",
		flatListWidgetClass, style_sw, largs, n);

    /*
     * create point size
     */
    n = 0;
    XtSetArg(largs[n], XtNrefWidget, view->size_caption);		n++;
    view->size_window = XtCreateManagedWidget("sw",
          scrolledWindowWidgetClass, box_parent, largs, n);
    n = 0;
    XtSetArg(largs[n], XtNitems, dummy);		n++;
    XtSetArg(largs[n], XtNnumItems, 1);	n++;
    XtSetArg(largs[n], XtNitemFields, item_fields);		n++;
    XtSetArg(largs[n], XtNnumItemFields, XtNumber(item_fields)); n++;
    XtSetArg(largs[n], XtNsameWidth, OL_ALL);	n++;
    XtSetArg(largs[n], XtNlabelJustify, OL_CENTER);	n++;
    XtSetArg(largs[n], XtNselectProc, PSSelectCB);	n++;
    XtSetArg(largs[n], XtNclientData, view);		n++;
    XtSetArg(largs[n], XtNuserData, view->font_state[POINT]);n++;
    XtSetArg(largs[n], XtNweight, 0);         n++;
    XtSetArg(largs[n], XtNviewHeight, 5);			n++;
    XtSetArg(largs[n], XtNmaintainView, TRUE);			n++;
    view->size_exclusive = XtCreateManagedWidget("sizes",
		flatListWidgetClass, view->size_window, largs, n);

    view->cur_size = atoi(DEFAULT_POINT_SIZE);
    n = 0;
    XtSetArg(largs[n], XtNmaximumSize, 3);	n++;
    XtSetArg(largs[n], XtNvalueMin, MIN_PS_VALUE);		n++;
    XtSetArg(largs[n], XtNvalueMax, MAX_PS_VALUE);		n++;
    XtSetArg(largs[n], XtNvalue, view->cur_size);		n++;
    XtSetArg(largs[n], XtNrefWidget, view->size_caption);	n++;
    XtSetArg(largs[n], XtNtype, OL_DECORATION);         n++;
    XtSetArg(largs[n], XtNweight, 0);         n++;
    view->ps_text = XtCreateManagedWidget("os", integerFieldWidgetClass,
				   box_parent, largs, n);
    XtAddCallback(view->ps_text, XtNvalueChanged, OutlinePSCB, view);

} /* end of CreateMainView */


static void
CreateStatusArea(view, parent)
    view_type *view;
    Widget parent;
{
    Arg largs[10];
    int n;

    n = 0;
    XtSetArg(largs[n], XtNleftFoot,    base.error); n++;
    XtSetArg(largs[n], XtNrightFoot,   base.status); n++;
    XtSetArg(largs[n], XtNleftWeight,  100);   n++;
    XtSetArg(largs[n], XtNrightWeight, 0); n++;
    XtSetArg(largs[n], XtNweight, 0);       n++;
    view->footer_text = XtCreateManagedWidget (
      "footer", footerWidgetClass, parent, largs, n   );

} /* end of CreateStatusArea */


void
UpdateMainView()
{
    DeleteFontDB( view_data.family_array);

    /* fill database with font information */
    CreateFontDB(&view_data, view_data.family_array);

    /*  Initialize the state of the exclusives for the Reset button.  */
    ResetFont(&view_data);
} /* end of UpdateMainView */


int
MyXErrorHandler(Display *display, XErrorEvent *err)
{
    char msg[MAX_STRING];

    XGetErrorText(display, err->error_code, msg, sizeof(msg));
    fprintf(stderr, "fontsetup: X Error code %s\n", msg);
    fprintf(stderr, "fontsetup: X Error request code %d\n", err->request_code);
    fprintf(stderr, "fontsetup: X Error minor code %d\n", err->minor_code);

} /* end of MyXErrorHandler */


/*
 * search for X resource in .Xdefaults, return TRUE if found
 */
static Boolean
CheckResource(char *filename, String res, Boolean value_needed)
{
    FILE *file;
    int key_len = strlen(res);
    char buf[MAX_PATH_STRING];
	char *ptr;
    Boolean found = FALSE;

    file = fopen(filename, "r");
    if (FileOK(file)) {
	while (fgets(buf, MAX_PATH_STRING, file) != NULL) {
	    if (strncmp(buf, res, key_len) == STR_MATCH) {
		found = TRUE;
		if (value_needed) {
			ptr = strchr(buf, ':');
			if (ptr) ptr++;
			strcpy(font_value, ptr);
			break;
	    	}
		}
	}
	fclose(file);
    }
    return found;
}


/* 
*******************************************************************************
*   MAIN function
*******************************************************************************
*/

main ( argc, argv )
int    argc;
char *argv[];
{   
    
   OlToolkitInitialize(&argc, argv, (XtPointer) NULL);
   app_shellW = XtInitialize(ClientName, ClientClass, NULL, 0,
		 &argc, argv);
   DtInitialize(app_shellW);
 


    if (app_shellW) {
	InitFontMgr(argc, argv);
	}

    signal(SIGCLD, SIG_IGN);
    XtMainLoop  ( );


}


InitFontMgr(argc, argv)
int	argc;
char *	argv[];
{

    int n;
    Widget deepest;
    Arg largs[10];
    int c;
    extern char *optarg;
    String device = NULL;
    String display = (String) getenv("DISPLAY");
    String language;

    int connect_mode = LOCAL;
    char filename[MAX_PATH_STRING];
    char hostname[128];
    XSetErrorHandler(MyXErrorHandler);

    
    if (display) {
 	gethostname(hostname, 128);
	c = strlen(hostname);
#ifdef DEBUG
	fprintf(stderr,"len=%d hostname=%s\n",c,hostname);
#endif
	if ((strncmp(display, hostname, c)!=STR_MATCH) && 
        (*display != ':') && (strncmp(display, "unix:", 5)!=STR_MATCH))  {
	    connect_mode = REMOTE;
        }
#ifdef DEBUG
	fprintf(stderr,"connect_mode=%d\n", connect_mode);
#endif

    }
    while ((c = getopt(argc, argv, "?hvpf:")) != EOF)
	switch (c) {
	case '?':
	case 'h':
	    fprintf(stderr, GetGizmoText(USAGE_MSG));
	    exit (2);
	    break;
	case 'p':
#ifdef DEBUG
	fprintf(stderr,"started fontmgr -p\n");
#endif
	    install_allowed = TRUE;
	    break;
	case 'v':
	    printf(GetGizmoText(VERSION_MSG),
		   OL_VERSION, XtSpecificationRelease);
	    exit (0);
	    break;
	case 'f':
	    device = optarg;
#ifdef DEBUG
	fprintf(stderr,"setting device=%s\n",device);
#endif
	    break;
	}

    if (connect_mode == REMOTE) install_allowed = FALSE;
    xwin_home = getenv("XWINHOME");
    if (xwin_home)
	xwin_home = XtNewString(xwin_home);
    else
	xwin_home = "/usr/X";

	GetCurrentLocale();
    home = getenv("HOME");
    if (home) sprintf(filename, "%s/.Xdefaults", home);
     else
    sprintf(filename, "/.Xdefaults");

    Xterm_set = CheckResource(filename, XTERM_FONT_RESOURCE_NAME, 0);

    GetScreenResolutions(&view_data);
    edit_menu_item[ADD_BUT].sensitive = install_allowed; 
    edit_menu_item[DELETE_BUT].sensitive = install_allowed; 
    file_menu_item[INTEGRITY_BUT].sensitive = install_allowed; 
    base.title = GetGizmoText(base.title);
    base.icon_name = GetGizmoText(base.icon_name);
    base_shell = CreateGizmo(app_shellW, BaseWindowGizmoClass, &base, NULL, 0);
    deepest = base.form;
    XtVaSetValues(deepest, XtNshadowThickness, 0, NULL);
    XtVaSetValues(menu_bar.child, XtNshadowThickness, 2, NULL);

    view_data.form = XtVaCreateManagedWidget("basePane", 
	 panesWidgetClass, deepest, XtNshadowThickness, 0, 
	 XtNlayoutWidth, OL_MINIMIZE, NULL);
    CreateMainView(&view_data);
    CreateStatusArea(&view_data, deepest);

    /* fill database with font information */
    CreateFontDB(&view_data, view_data.family_array);
    ResetToDefaultFont( &view_data);
    ResetFont( &view_data);

    /* this gizmo is for saving to .Xdefaults */
    XtUnmanageChild(font_save_choice.captionWidget);

    XtUnmanageChild(base.message);
    MapGizmo(BaseWindowGizmoClass, &base);
    XtDestroyWidget(base.message);

    if (device) {
	DnDPopupAddWindow (device);
	device = NULL;
    }

    OlDnDRegisterDDI(view_data.form, OlDnDSitePreviewNone, 
		     DropNotify,
		     (OlDnDPMNotifyProc)NULL, True, NULL);
    OlDnDRegisterDDI(base.icon_shell, OlDnDSitePreviewNone, 
		     DropNotify,
		     (OlDnDPMNotifyProc)NULL, True, NULL);
   callRegisterHelp(app_shellW, ClientName, TXT_HELP_MAIN_MENU);
}


static void
GetScreenResolutions(view)
view_type *view;
{
	Display *dpy;
	int scr;
	double res;
	
	dpy = XtDisplay(app_shellW);
	scr = DefaultScreen(dpy);
	res = ((((double) DisplayWidth(dpy, scr)) * 25.4) /
		((double) DisplayWidthMM(dpy, scr)));
	view->resx = (int) (res + 0.5);

	res = ((((double) DisplayHeight(dpy, scr)) * 25.4) /
		((double) DisplayHeightMM(dpy, scr)));
	view->resy = (int) (res + 0.5);
	view->dimx = DisplayWidth(dpy,scr);
	view->dimy = DisplayHeight(dpy,scr);
#ifdef DEBUG
	fprintf(stderr,"GetScreenResolutions: resx=%d resy=%d dimx=%d dimy=%d\n",view->resx,view->resy, view->dimx, view->dimy);
#endif
}


static Boolean
DropNotify OLARGLIST((w, win, x, y, selection, timestamp,
			    drop_site_id, op, send_done, forwarded, closure))
  OLARG( Widget,		w)
  OLARG( Window,		win)
  OLARG( Position,		x)
  OLARG( Position,		y)
  OLARG( Atom,			selection)
  OLARG( Time,			timestamp)
  OLARG( OlDnDDropSiteID,	drop_site_id)
  OLARG( OlDnDTriggerOperation,	op)
  OLARG( Boolean,		send_done)
  OLARG( Boolean,		forwarded)
  OLGRA( XtPointer,		closure)
{
    XtGetSelectionValue(
			w, selection, OL_XA_FILE_NAME(XtDisplay(w)),
			SelectionCB, (XtPointer) send_done, timestamp
			);

    return(True);
} /* end of DropNotify */


static void
SelectionCB OLARGLIST ((w, client_data, selection, type, value, length,
			   format))
  OLARG( Widget,		w)
  OLARG( XtPointer,		client_data)
  OLARG( Atom *,		selection)
  OLARG( Atom *,		type)
  OLARG( XtPointer,		value)
  OLARG( unsigned long *,	length)
  OLGRA( int *,			format)
{
   int x;
    Boolean send_done = (Boolean)client_data;
    String fullname;

    /* Since only OL_XA_FILE_NAME(dpy) is passed in, we know we have a
       valid type.
       */

#ifdef DEBUG
fprintf(stderr,"in SelectionCB install_allowed=%d\n",install_allowed);
#endif

    XtMapWidget(base_shell);
 
    fullname = (String) value;

#ifdef DEBUG
	fprintf(stderr,"fullname=%s\n", fullname);
	fprintf(stderr,"now try to raise window\n");
#endif

    XRaiseWindow(XtDisplay(base_shell), XtWindow(base_shell));
    if (fullname && *fullname) {
	if ((x=strcmp(fullname, "-p"))!= 0) {
    	if (install_allowed == FALSE ) {
		PopupErrorMsg(TXT_NO_DND_PRIVILEGE,
		ErrorCancelCB,
		NULL,
		NO_DND_TITLE,
		&help_apply_to_windows,
		TXT_APPLY_WINDOWS_HELP_SECTION);
		return;
	}
		DnDPopupAddWindow( fullname);
	} else {

		;
#ifdef DEBUG
		fprintf(stderr,"don't popup add window\n");
#endif
	}
    }

    /* We don't care if there was an error. 
       The transaction is done regardless. */

    XtFree(value);

    if (send_done == True) {
	OlDnDDragNDropDone(w, *selection, CurrentTime, NULL, NULL);
    }
} /* end of SelectionCB */


static Boolean
GetLocaleCodeSet(xwin_home,locale)
char *xwin_home;
char *locale;
{
    FILE *file;
    char buf[MAX_PATH_STRING];
    Boolean found = FALSE;
	char *ptr = NULL;

    sprintf(buf, "%s/%s/%s/%s", xwin_home, "lib/locale", locale, "Codeset");
    if (locale_encoding) free(locale_encoding);
    locale_encoding = NULL;
    file = fopen(buf, "r");
    found = FALSE;
    if (FileOK(file)) {
        while (fgets(buf, MAX_PATH_STRING, file) != NULL) {
            if (strncmp(buf, "NAME", 4) == STR_MATCH)  {
					ptr  = strrchr(buf, '\t');
					if (ptr) ptr++;
#ifdef DEBUG
					fprintf(stderr,"ptr=%s\n",ptr);
#endif
					if ((ptr) && (strncmp(ptr, "ISO8859", 7) == STR_MATCH)) {
                		found = TRUE;
						locale_encoding =strdup(ptr+8);
#ifdef DEBUG
			fprintf(stderr,"locale_encoding %s\n",locale_encoding);
#endif
                		break;
            		} else  {
		
						ptr  = strrchr(buf, '\t');
#ifdef DEBUG
						fprintf(stderr,"ptr=%s\n",ptr);
#endif
						if (ptr) locale_encoding = strdup(ptr+1);
#ifdef DEBUG
						fprintf(stderr,"locale_encoding %s\n",locale_encoding);
#endif
                		break;
        			}
				} /* end if */
		} /* end while */
        fclose(file);
    }
    return found;
}



Boolean CheckRegistryEncodingIso8859(lowername)
char * lowername;
{
    int len, f, result;
    char *p;
    Boolean  registry=FALSE, encoding=FALSE;
    char field_str[MAX_STRING];
    char *fieldP;

#ifdef DEBUG
	fprintf(stderr,"CheckRegistryEncodingIso8859 name=%s\n",lowername);
	fprintf(stderr,"CheckRegistryEncodingIso8859 locale_encoding=%s\n",locale_encoding);
#endif
	for (f=1, p=lowername; (f <= FIELD_COUNT); f++) {
	    p = (char *) GetNextField(DELIM, p, &fieldP, &len);
	    if (f < 13) continue;
	    if (len) strncpy(field_str, fieldP, len);
		strcat(field_str, '\0');
	    if  (f == 13)  { 
		    /* registry field */
			result=strncmp(field_str,"iso8859", 7);

			if (strncmp(field_str,"iso8859", 7) == 0) {
		    	registry = TRUE;
			} else {
		    	registry = FALSE;
	    	}
		
		}
	    if (f == 14) {
				/* does encoding match encoding for locale */
			if (strncmp(field_str,locale_encoding,len) == 0) {
				encoding = TRUE;
			} else 
			if (strncmp(locale_encoding,"C", 1) == 0) {
				encoding = TRUE;
			} else  {
		    	encoding = FALSE;
			}
	    }
	} /* end for */
    result = (Boolean) registry && encoding;
#ifdef DEBUG
	fprintf(stderr,"result=%d\n", result);
#endif
    return (registry && encoding);

}


SetMotifFontResources(view)
view_type *view;
{

	int i,result;
 	int numOtherFonts;
	Display * display;
	motif_font_resources motif_res;
	motif_font_resources *otherFontsInFamily;
	Boolean first_time = True;
	char slant[10]; 
	char weight[25];
	char pattern[256];
	char *cur_font;
	int cur_font_indx;

	display = XtDisplay(app_shellW);
	/* initialize the font resources */

	otherFontsInFamily = &motif_res;
	otherFontsInFamily->plain_font = 0;
	otherFontsInFamily->italic_font = 0;
	otherFontsInFamily->bold_font = 0;
	otherFontsInFamily->italic_bold_font=0;
	otherFontsInFamily->complete_family = False;
	first_time = False;
	/* need to get and save an index to the current font, so that
		we can put that one first in the apply list */

	cur_font_indx = GetFontWeightSlant(view->cur_xlfd);
		/* need the pattern here to use for finding the rest
		/* put applied font into the family first so that fonts
		that sort before it in the XListFonts will not override it */

	SetFontInFamily(view->cur_xlfd, otherFontsInFamily, True);

	SetupWeightSlantPattern(view->cur_xlfd,pattern, weight, slant);
		/* need the pattern here to use for finding the rest
			of the family */
		/* problem parsing xlfd name then return */
#ifdef APPLY_DEBUG	
	fprintf(stderr,"SetMotifFontResources: view->cur_xlfd=%s pattern=%s weight=%s slant=%s\n",view->cur_xlfd, pattern, weight, slant);
#endif
	
	numOtherFonts = (int)FontIsPartOfFamily(display, pattern, otherFontsInFamily);

	/* otherFontsInFamily returns in this order:
		regular
		regular italic
		bold
		bold italic 
		fonts and a count of how many were found.
	*/
#ifdef DEBUG
	fprintf(stderr,"MotifSetResources plain=%s\n", otherFontsInFamily->plain_font);
	fprintf(stderr,"MotifSetResources italic=%s\n",otherFontsInFamily->italic_font);
	fprintf(stderr,"MotifSetResources bold=%s\n", otherFontsInFamily->bold_font);
	fprintf(stderr,"MotifSetResources bold italic=%s\n", otherFontsInFamily->italic_bold_font);

#endif
	/* now write out resources */

	cur_font=  (char *) SetFontDefaults(cur_font_indx, otherFontsInFamily);
	switch (cur_font_indx) {

		case 0:
			sprintf(font_value, "%s%s%s", 
				cur_font,
				PLAIN_FONT, 
				",");
			sprintf(font_value, "%s%s%s%s", 
				font_value,
				otherFontsInFamily->italic_font,
				ITALIC_FONT, 
				",");
			sprintf(font_value, "%s%s%s%s", 
				font_value,
				otherFontsInFamily->bold_font,
				BOLD_FONT, 
				",");
			sprintf(font_value, "%s%s%s", 
				font_value,
				otherFontsInFamily->italic_bold_font,
				ITALIC_BOLD_FONT);
		 	break;	
		case 1:

			sprintf(font_value, "%s%s%s", 
				cur_font,
				ITALIC_FONT, 
				",");
			sprintf(font_value, "%s%s%s%s", 
				font_value,
				otherFontsInFamily->plain_font,
				PLAIN_FONT, 
				",");
			sprintf(font_value, "%s%s%s%s", 
				font_value,
				otherFontsInFamily->bold_font,
				BOLD_FONT, 
				",");
			sprintf(font_value, "%s%s%s", 
				font_value,
				otherFontsInFamily->italic_bold_font,
				ITALIC_BOLD_FONT);
			break;
		case 2:

			sprintf(font_value, "%s%s%s", 
				cur_font,
				BOLD_FONT, 
				",");
			sprintf(font_value, "%s%s%s%s", 
				font_value,
				otherFontsInFamily->plain_font,
				PLAIN_FONT, 
				",");
			sprintf(font_value, "%s%s%s%s", 
				font_value,
				otherFontsInFamily->italic_font,
				ITALIC_FONT, 
				",");
			sprintf(font_value, "%s%s%s", 
				font_value,
				otherFontsInFamily->italic_bold_font,
				ITALIC_BOLD_FONT);
				break;
		case 3:

			sprintf(font_value, "%s%s%s", 
				cur_font,
				ITALIC_BOLD_FONT, 
				",");
			sprintf(font_value, "%s%s%s%s", 
				font_value,
				otherFontsInFamily->plain_font,
				PLAIN_FONT, 
				",");
			sprintf(font_value, "%s%s%s%s", 
				font_value,
				otherFontsInFamily->italic_font,
				ITALIC_FONT, 
				",");
			sprintf(font_value, "%s%s%s", 
				font_value,
				otherFontsInFamily->bold_font,
				BOLD_FONT);
				break;
		default:

			break;
	}	

#ifdef DEBUG
	fprintf(stderr,"font_value for apply font =%s\n",font_value);
#endif
    font_save_choice.name = SANS_SERIF_FONTLIST_RESOURCE_NAME;
    font_save_menu.items[0].set = TRUE;
    font_save_menu.items[0].mod.resource_value = font_value;
    ManipulateGizmo(ChoiceGizmoClass, &font_save_choice, SetGizmoValue);
#ifdef xxx
    font_save_choice.name = FONTLIST_RESOURCE_NAME;
    font_save_menu.items[0].set = TRUE;
    font_save_menu.items[0].mod.resource_value = font_value;
    ManipulateGizmo(ChoiceGizmoClass, &font_save_choice, SetGizmoValue);
#endif
				
    FreeFontsInFamily(otherFontsInFamily);
}


static char *
SetFontDefaults(cur_font_indx, others)
int cur_font_indx;
motif_font_resources *others;
{
	char *cur_font;
	switch (cur_font_indx) {
		case 0:
			cur_font = others->plain_font;
			break;
		case 1:
			cur_font = others->italic_font;
			break;
		case 2:
			cur_font = others->bold_font;
			break;
		case 3:
			cur_font = others->italic_bold_font;
			break;
		default:
			cur_font = NULL;
	}

 	if (others->plain_font == NULL) others->plain_font = cur_font;
	if (others->italic_font == NULL) others->italic_font = cur_font;	
	if (others->bold_font == NULL) others->bold_font = cur_font;	
	if (others->italic_bold_font == NULL) others->italic_bold_font = cur_font;	

	return cur_font;
}


static void
GetCurrentLocale()
{
	static char *newlocale=NULL; 

	char  file[MAX_STRING];
	char * resource_type;
	XrmValue value;
	Boolean C_locale;
	XrmDatabase		db;


   	char * home = getenv("HOME");
#ifdef DEBUG
	fprintf(stderr,"After Resource get  locale=%s newlocale=%s\n",locale,newlocale);
#endif
	sprintf(file, "%s/.Xdefaults", home);
	db = XrmGetFileDatabase(file);
	if (db) {
		if (XrmGetResource (db, "xnlLanguage", (char *) NULL, 
				&resource_type, &value) == True) {
					newlocale = (char *) value.addr;
		}
	
	}

		/* default to C locale if no .Xdefaults file */
	if (newlocale == NULL) newlocale= strdup("C");
#ifdef DEBUG
	fprintf(stderr,"After Resource get  locale=%s newlocale=%s\n",locale,newlocale);
#endif
	if (locale == newlocale) return; /* no change in locale*/
	locale = newlocale;
	(void) setlocale(LC_MESSAGES, newlocale);
#ifdef DEBUG
	fprintf(stderr,"GetCurrentLocale newlocale=%s \n", newlocale);
#endif
    if (newlocale && (strcmp(newlocale,"C")!=STR_MATCH)) {
    	C_locale = FALSE;
    } else {
    	C_locale = TRUE;
	}
   	ISO8859_locale = GetLocaleCodeSet(xwin_home, newlocale);


	ApplyAllowed = (C_locale|ISO8859_locale);
#ifdef DEBUG
		fprintf(stderr,"ApplyAllowed = %d\n", ApplyAllowed);
		fprintf(stderr,"C_locale=%d ISO8859_locale=%d\n", C_locale, ISO8859_locale);
#endif
}


