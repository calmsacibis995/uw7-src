#ifndef	NOIDENT
#pragma ident	"@(#)dtadmin:dashboard/tz.c	1.7"
#endif

/* *************************************************************************
 *
 * Description: TIME ZONE
 *
 ******************************file*header******************************** */
#include <stdio.h>
#include <string.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/BaseWindow.h>
#include <Xol/AbbrevButt.h>
#include <Xol/RubberTile.h>
#include <Xol/MenuShell.h>
#include <Xol/PopupWindo.h>
#include <Xol/FList.h>
#include <Xol/ScrolledWi.h>
#include <Xol/FButtons.h>
#include <Xol/StaticText.h>
#include <Xol/Caption.h>
#include <Xol/StepField.h>
#include <Xol/ControlAre.h>
#include <Xol/Modal.h>
#include <libDtI/DtI.h>
#include "dashmsgs.h" 
#include "tz.h" 
#include <Gizmo/Gizmos.h>

/* ************************************************************************* 
 * Define global/static variables and #defines, and 
 * Declare externally referenced variables
 *
 *****************************file*variables*******************************/
static void	choiceCB(Widget, XtPointer, XtPointer);
static void	selectCB(Widget, XtPointer, XtPointer);
static void	unselectCB(Widget, XtPointer, XtPointer);
static void	ApplyCB(Widget, XtPointer, XtPointer);
static void	GmtApplyCB(Widget, XtPointer, XtPointer);
static void	PopdownCB(Widget, XtPointer, XtPointer);
static void	GMTPopdownCB(Widget, XtPointer, XtPointer);
static void	GetValueCB(Widget, XtPointer, XtPointer);
static void	OtherCB(Widget, XtPointer, XtPointer);
static void	SetUpTime(Widget, XtPointer, XtPointer);
static void	CreateScrolledWindow(Widget, char *, _choice_items*, int);
static void	CancelCB(Widget, XtPointer, XtPointer);
static void     SetCountryTZ (_choice_items *, int);
char 		*gettz ();

extern int	tz_update;
extern void 	ErrorNotice (Widget, char *);
extern void	tzhelpCB(Widget, XtPointer, XtPointer);
extern void	VerifyCB(Widget, XtPointer, XtPointer);
extern void	SetLabels (MenuItem *, int);
extern void 	SetCountryLabels (_choice_items *, int);
extern	Widget	w_tz;
extern  char	*GetXWINHome ();

static struct _country {
        XtArgVal name;
        char    *timezone;
} country[] = {
        (XtArgVal) TXT_Greenwich,       "Greenwich",
        (XtArgVal) TXT_Mexico,          "Mexico/General",
        (XtArgVal) TXT_Great_Britain,   "GB-Eire",
        (XtArgVal) TXT_Eastern_Europe,  "EET",
        (XtArgVal) TXT_Western_Europe,  "WET",
        (XtArgVal) TXT_Central_Europe,  "MET",
        (XtArgVal) TXT_Eastern,         "US/Eastern",
        (XtArgVal) TXT_Central,         "US/Central",
        (XtArgVal) TXT_Mountain,        "US/Mountain",
        (XtArgVal) TXT_Pacific,         "US/Pacific",
        (XtArgVal) TXT_Yukon,           "US/Yukon",
        (XtArgVal) TXT_Hawaii_Alaska,   "US/Hawaii",
        (XtArgVal) TXT_Atlantic,        "Canada/Atlantic",
        (XtArgVal) TXT_Thailand,        "GMT+8",
        (XtArgVal) TXT_China,           "PRC",
        (XtArgVal) TXT_Singapore,       "Singapore",
        (XtArgVal) TXT_Korea,           "ROK",
        (XtArgVal) TXT_Hongkong,        "Hongkong",
        (XtArgVal) TXT_Japan,           "Japan",
        (XtArgVal) TXT_NorthA,          "Australia/North",
        (XtArgVal) TXT_WestA,           "Australia/West",
        (XtArgVal) TXT_SouthA,          "Australia/South",
        (XtArgVal) TXT_Queensland,      "Australia/Queensland",
};

#define  THAI	"#THAILAND\n"

static char *choice_fields[] = {
        XtNlabel, XtNselectProc, XtNuserData, XtNmnemonic,
};

static _choice_items choice_items[] = {
{ (XtArgVal) TXT_Greenwich, (XtArgVal) SetUpTime, (XtArgVal) 
'G', (XtArgVal)MNEM_Greenwich, },
{ (XtArgVal) TXT_North_America, (XtArgVal) choiceCB, (XtArgVal)
'N', (XtArgVal)MNEM_North_America, },
{ (XtArgVal) TXT_Europe, (XtArgVal) choiceCB, (XtArgVal) 
'E', (XtArgVal)MNEM_Europe, },
{ (XtArgVal) TXT_Mexico, (XtArgVal) SetUpTime, (XtArgVal) 
'M', (XtArgVal)MNEM_Mexico,},
{ (XtArgVal) TXT_Australia, (XtArgVal) choiceCB,  (XtArgVal) 
'U', (XtArgVal)MNEM_Australia,},
{ (XtArgVal) TXT_Asia, (XtArgVal) choiceCB, (XtArgVal) 
'A', (XtArgVal)MNEM_Asia, },
{ (XtArgVal) TXT_Other, (XtArgVal) OtherCB, (XtArgVal) 
'O', (XtArgVal)MNEM_Other,},
};

static _choice_items _Europe [] = {
{ (XtArgVal) TXT_Great_Britain, (XtArgVal) selectCB,}, 
{ (XtArgVal) TXT_Eastern_Europe, (XtArgVal) selectCB,}, 
{ (XtArgVal) TXT_Western_Europe, (XtArgVal) selectCB,}, 
{ (XtArgVal) TXT_Central_Europe, (XtArgVal) selectCB,}, 
};

static _choice_items North_America [] = {
{ (XtArgVal) TXT_Eastern, (XtArgVal) selectCB,}, 
{ (XtArgVal) TXT_Central, (XtArgVal) selectCB,}, 
{ (XtArgVal) TXT_Mountain, (XtArgVal) selectCB,}, 
{ (XtArgVal) TXT_Pacific, (XtArgVal) selectCB,}, 
{ (XtArgVal) TXT_Yukon, (XtArgVal) selectCB,}, 
{ (XtArgVal) TXT_Hawaii_Alaska,  (XtArgVal) selectCB,},
{ (XtArgVal) TXT_Atlantic,  (XtArgVal) selectCB,},
}; 

static _choice_items Asia [] = { 
{ (XtArgVal) TXT_Thailand, (XtArgVal) selectCB,}, 
{ (XtArgVal) TXT_China, (XtArgVal) selectCB,}, 
{ (XtArgVal) TXT_Singapore, (XtArgVal) selectCB,}, 
{ (XtArgVal) TXT_Korea, (XtArgVal) selectCB,}, 
{ (XtArgVal) TXT_Hongkong, (XtArgVal) selectCB,}, 
{ (XtArgVal) TXT_Japan, (XtArgVal) selectCB,}, 
} ;

static _choice_items Australia [] = { 
{ (XtArgVal) TXT_NorthA, (XtArgVal) selectCB,}, 
{ (XtArgVal) TXT_WestA, (XtArgVal) selectCB,}, 
{ (XtArgVal) TXT_SouthA, (XtArgVal) selectCB,}, 
{ (XtArgVal) TXT_Queensland, (XtArgVal) selectCB,}, 
} ;

/**********************************************************************
                Lower Control Area buttons
***********************************************************************/
static HelpInfo HelpProps       = { 0, "", HELP_PATH, help_props };

static MenuItem CommandItems [] = {
    { (XtArgVal) TXT_OK, (XtArgVal) MNEM_OK, (XtArgVal) True,
          (XtArgVal) ApplyCB, (XtArgVal) True, (XtArgVal) 0},/*Save */
    { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel, (XtArgVal) True,
          (XtArgVal) CancelCB, },                       /* Cancel */
    { (XtArgVal) TXT_help, (XtArgVal) MNEM_help, (XtArgVal) True,
          (XtArgVal) tzhelpCB,  (XtArgVal) False, (XtArgVal) &HelpProps},     /* Help */
};

static MenuItem GmtItems [] = {
    { (XtArgVal) TXT_OK, (XtArgVal) MNEM_OK, (XtArgVal) True,
          (XtArgVal) GmtApplyCB, (XtArgVal) True, (XtArgVal) 0},/*Save */
    { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel, (XtArgVal) True,
          (XtArgVal) CancelCB, },                       /* Cancel */
    { (XtArgVal) TXT_help, (XtArgVal) MNEM_help, (XtArgVal) True,
          (XtArgVal) tzhelpCB,  (XtArgVal) False, (XtArgVal) &HelpProps},   /* Help */
};

static String   TZFields [] = {
    XtNlabel, XtNmnemonic, XtNsensitive, XtNselectProc, XtNdefault,
    XtNclientData, 
};
static int      NumTZFields = XtNumber (TZFields);

/**********************************************
        format header fields
***********************************************/
static String   ListFields [] = {
    XtNformatData,
};

typedef struct {
    XtPointer   timezone;
} Format;

typedef struct {
    XtArgVal    formatData;
} TZItem;

static Format   ColHdrs [1];
static TZItem         ColItem [] = {
    (XtArgVal) ColHdrs,
};

/*******************************************************
		global variables
*******************************************************/
Boolean         PopdownOK;
Boolean         Unselected = True;
static Widget   gmtw =  NULL;
static 	Widget	timezones = NULL;
static Widget	list, header_bar;
static char 	TZ_buffer[BUFSIZ];
static char 	Step_buffer[BUFSIZ];
static char 	*tzptr, *labelptr;
Widget		current_selection;
Widget          gmtval;

/**************************************************************************
 * Create - creates an AbbrevMenuButton widget
 ****************************procedure*header******************************/
void
Create_tz(Widget parent)
{
	Widget 		w, menu_button, control, rubber_tile;
	static Widget 	menupane;
	Arg 		args[20];
	int 		num;
	static char	ptr[256];
	FILE		*fp;
	static Boolean  first = True;

	if (first) {
		SetCountryLabels (choice_items, XtNumber (choice_items));
		SetCountryLabels (North_America, XtNumber (North_America));
		SetCountryLabels (Asia, XtNumber (Asia));
		SetCountryLabels (_Europe, XtNumber (_Europe));
		SetCountryLabels (Australia, XtNumber (Australia));
                SetCountryTZ (North_America, XtNumber (North_America));
                SetCountryTZ (Asia, XtNumber (Asia));
                SetCountryTZ (_Europe, XtNumber (_Europe));
                SetCountryTZ (Australia, XtNumber (Australia));
	 	first = False;
	}

        control = XtVaCreateManagedWidget("caption", captionWidgetClass,
                	parent, XtNlabel, GetGizmoText (tag_tz), (String)0);

	rubber_tile = XtVaCreateManagedWidget( "rubberTile", 
				rubberTileWidgetClass,
				control, 
                		XtNshadowThickness,    (XtArgVal)0,
                		XtNorientation,        (XtArgVal)OL_HORIZONTAL,
                		(String)0);

        menupane = XtVaCreatePopupShell ( "pane", popupMenuShellWidgetClass,
					XtParent (parent), (String)0);

        menu_button = XtVaCreateManagedWidget( "abbrev",
              abbreviatedButtonWidgetClass, rubber_tile,
              XtNpopupWidget,   (XtArgVal) menupane, (String)0);

   	current_selection = XtVaCreateManagedWidget( "Time Zone", 
	      staticTextWidgetClass, rubber_tile,
              XtNrefName,       (XtArgVal) "abbrev",
              XtNrefSpace,      (XtArgVal) OlScreenPointToPixel 
				(OL_HORIZONTAL, 7, XtScreenOfObject(parent)),
              (String)0);
	
	/* Give the preview Widget to the  AbbreviatedMenuButton	*/
	num=0;
	XtSetArg(args[num], XtNpreviewWidget, current_selection); ++num;
	XtSetValues(menu_button, args, num);

	/* Since the menu is automatically added to
	 * the AbbreviatedMenuButton, get the
	 * widget id of the pane that is to be
	 * populated.				*/
	/* Now, we are ready to populate the pane.	*/
        
        w = XtVaCreateManagedWidget( "nextChoice", flatButtonsWidgetClass,
                    menupane,
                    XtNitemFields,      (XtArgVal)choice_fields,
                    XtNnumItemFields,   (XtArgVal)XtNumber(choice_fields),
                    XtNitems,           (XtArgVal)choice_items,
                    XtNnumItems,        (XtArgVal)XtNumber(choice_items),
                    (String)0);

	if ((gettz (ptr)) != NULL)
		XtVaSetValues (current_selection, XtNstring,(XtArgVal) ptr, 0);
	else 
		XtVaSetValues (current_selection, XtNstring,(XtArgVal)NULL, 0);
} /* END OF Create() */

/*
 *************************************************************************
			selected callback
 ****************************procedure*header*****************************
*/
static void
choiceCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData *pFlatData = (OlFlatCallData *) call_data;
	_choice_items	*select;

	select = (_choice_items *) pFlatData->items + pFlatData->item_index;

        Unselected = True;

	/* raise the window into sight if its there already */
	if (timezones) 
		XRaiseWindow (XtDisplay (list), XtWindow (list));

	/* if the timezones window is not there then create it */
	if (timezones == NULL) {
		switch (select->userData) {
			case 'N':
				CreateScrolledWindow(widget, GetGizmoText
					(TXT_AmericaZone), North_America, 7);
				break;
			case 'A' :
				CreateScrolledWindow(widget, GetGizmoText 
					(TXT_AsianZone), Asia, 6);
				break;
			case  'E' :
				CreateScrolledWindow(widget, GetGizmoText 
					(TXT_EuropeZone), _Europe, 4);
				break;
			case 'U' :
				CreateScrolledWindow(widget, GetGizmoText 
					(TXT_AustraliaZone), Australia, 4);
				break;
		}
	}
	/* if the timezones window is already there, then
	 * reset the items and the header of the scrolled
	 * window and reset the  view height depending on 
	 * the timezone asked for
	 */
	else {
		XtVaSetValues (timezones, 
                	XtNselectProc, 		(XtArgVal) selectCB, 
                	XtNunselectProc, 	(XtArgVal) unselectCB, 
                	XtNdblSelectProc, 	(XtArgVal) ApplyCB, 
                	XtNclientData, 		(XtArgVal)  DOUBLE, 
			0);
		switch (select->userData) {
			case 'N':
        			ColHdrs [0].timezone = (XtPointer) 
						GetGizmoText(TXT_AmericaZone);
			XtVaSetValues (timezones, 
                		XtNitems,  (XtArgVal) North_America, 
                		XtNnumItems, (XtArgVal) XtNumber(North_America),
               			XtNviewHeight, 	(XtArgVal) 7,
				0);
				break;
			case 'E':
        			ColHdrs [0].timezone = (XtPointer) 
						GetGizmoText(TXT_EuropeZone);
			XtVaSetValues (timezones, 
                		XtNitems,  (XtArgVal) _Europe, 
                		XtNnumItems,  (XtArgVal) XtNumber (_Europe), 
               			XtNviewHeight, (XtArgVal) 4,
				0);
				break;
			case 'A':
        			ColHdrs [0].timezone = (XtPointer) 
						GetGizmoText(TXT_AsianZone);
			XtVaSetValues (timezones, 
                		XtNitems,  (XtArgVal) Asia, 
                		XtNnumItems,  (XtArgVal) XtNumber (Asia), 
               			XtNviewHeight, (XtArgVal) 6,
				0);
				break;
			case 'U':
        			ColHdrs [0].timezone = (XtPointer) 
						GetGizmoText(TXT_AustraliaZone);
			XtVaSetValues (timezones, 
                		XtNitems,  (XtArgVal) Australia, 
                		XtNnumItems,  (XtArgVal) XtNumber (Australia), 
               			XtNviewHeight, (XtArgVal) 4,
				0);
				break;
		} /* switch statement */
		/* reset the header depending on who called us */
		XtVaSetValues (header_bar, 
                		XtNitems,  		(XtArgVal) ColItem, 
                		XtNitemsTouched,	(XtArgVal) True, 
				0);
	} /* if the window is already there */
}

/**************************************************************************
			each choice
 ****************************procedure*header******************************/
static void
CreateScrolledWindow (Widget parent, char *header_string, _choice_items *items, 
		int view_no)
{
	Widget 		scrolledWindow, menupane, lcaMenu, w, lca, uca;
	Arg 		arg[20];
	static Boolean	first = True;

       /* Set Labels */
        if (first) {
		first = False;
                SetLabels (CommandItems, XtNumber (CommandItems));
	}
		 
       /* Set header */
        ColHdrs [0].timezone = (XtPointer) header_string;

    	/* Create user sheet */
    	list = XtVaCreatePopupShell ("chooser",
                popupWindowShellWidgetClass, parent,
                XtNtitle,               (XtArgVal) GetGizmoText (TXT_TZtitle),
                0);

    	XtVaGetValues (list,
                XtNlowerControlArea,    (XtArgVal) &lca,
                XtNupperControlArea,    (XtArgVal) &uca,
                0);

    	/* Create a list of timezones   Add a flat list above this
     	 * with a single item in it to act as column headers.
     	 */

    	 header_bar = XtVaCreateManagedWidget ("colHdr",
                flatListWidgetClass, uca,
                XtNviewHeight,          (XtArgVal) 1,
                XtNexclusives,          (XtArgVal) True,
                XtNnoneSet,             (XtArgVal) True,
                XtNselectProc,          (XtArgVal) 0,
                XtNitems,               (XtArgVal) ColItem,
                XtNnumItems,            (XtArgVal) 1,
                XtNitemFields,          (XtArgVal) ListFields,
                XtNnumItemFields,       (XtArgVal) XtNumber (ListFields),
                XtNformat,              (XtArgVal) "%s",
                XtNtraversalOn,         (XtArgVal) False,
                0);

    	scrolledWindow = XtVaCreateManagedWidget ("scrolledWindow",
                scrolledWindowWidgetClass, uca,
                0);

 	timezones = XtVaCreateManagedWidget ("timezone", flatListWidgetClass, 
			scrolledWindow, 
       			XtNviewHeight, 		(XtArgVal) view_no,
			XtNnumItems, 		(XtArgVal) view_no,
   			XtNselectProc, 		(XtArgVal) selectCB,
   			XtNunselectProc, 	(XtArgVal) unselectCB,
        		XtNexclusives, 		(XtArgVal) True,
        		XtNnoneSet,    		(XtArgVal) True,
   			XtNdblSelectProc, 	(XtArgVal) ApplyCB,
   			XtNclientData, 		(XtArgVal) DOUBLE,
        		XtNitemFields,    	(XtArgVal) choice_fields,
        		XtNnumItemFields, 	(XtArgVal)
						XtNumber(choice_fields),
        		XtNformat,        	(XtArgVal) "%s",
			XtNitems,   		(XtArgVal) items,
			0);	

    	/* We want an "cancel" and "apply" buttons in the lower control area
         */
    	lcaMenu = XtVaCreateManagedWidget ("lcaMenu",
                flatButtonsWidgetClass, lca,
                XtNitemFields,          (XtArgVal) TZFields,
                XtNnumItemFields,       (XtArgVal) NumTZFields, 
		XtNitems,               (XtArgVal) CommandItems, 
		XtNnumItems,            (XtArgVal) XtNumber (CommandItems), 0); 

	/* Add callbacks to verify and destroy widgets the sheet goes away */
    	XtAddCallback (list, XtNverify, VerifyCB, (XtPointer) &PopdownOK) ;
    	XtAddCallback (list, XtNpopdownCallback, PopdownCB, (XtPointer)NULL);

    	XtPopup (list, XtGrabNone);
}

 /***************************************************************************
 * Popdown callback.  
 ***************************************************************************/
static void
PopdownCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	XtDestroyWidget (widget);
	timezones = (Widget) NULL;
	list = (Widget) NULL;
}       /* End of PopdownCB () */

 /***************************************************************************
 * GmtPopdown callback.  
 ***************************************************************************/
static void
GMTPopdownCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	XtDestroyWidget (widget);
	gmtw = (Widget) NULL;
}       /* End of PopdownCB () */

 /***************************************************************************
	select callback
 ***************************************************************************/
static void
selectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData  *pFlatData = (OlFlatCallData *) call_data;
	Arg 		arg[5];

	Unselected = False;
        XtSetArg(arg[0], XtNlabel, &labelptr);
        OlFlatGetValues(widget, pFlatData->item_index, arg, 1);
        XtSetArg(arg[0], XtNuserData, &tzptr);
        OlFlatGetValues(widget, pFlatData->item_index, arg, 1);
}

 /***************************************************************************
	unselect callback
 ***************************************************************************/
static void
unselectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
		Unselected = True;
}

/***************************************************************************
 * Apply callback.  
 ***************************************************************************/
static void
ApplyCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	char 		*dbl = (char *) client_data;
	OlFlatCallData  *pFlatData = (OlFlatCallData *) call_data;
	char		ptr[20];	
  	Widget      	shell;
	Arg 		arg[5];
	int 		i=0;
	char 		*tmpptr;

	/* if it is unselected go back */
	if (Unselected == True) {
		ErrorNotice (widget, GetGizmoText (TXT_selectone));
		return;
	}

	/* if it is double click over an item get the item name */
        if (dbl != NULL){
	if (strcmp (dbl, DOUBLE) == 0) {
                XtSetArg(arg[0], XtNlabel, &labelptr);
                OlFlatGetValues(widget, pFlatData->item_index, arg, 1);
                XtSetArg(arg[0], XtNuserData, &tzptr);
                OlFlatGetValues(widget, pFlatData->item_index, arg, 1);
	 }
        }

	/* if it is not double click and item has not been selected then
	 * display error message
         */
	if (tzptr == NULL)  { 
		ErrorNotice (widget, GetGizmoText (TXT_selectone));
		return;
	}

	/* copy the timezone from the countries file and store
	 * it in the global TZ_buffer and display it in the 
	 * properties menu
	 */
	sprintf (TZ_buffer, "%s %s\n", GetGizmoText (tag_tz), tzptr);
	XtVaSetValues (	current_selection, XtNstring,    (XtArgVal) labelptr, 0);

	/* set the tz_update flag to 1 indicating that
	 * the timezone has to be updated in the /etc/TIMEZONE and 
	 * the main menu when apply is hit in the properties menu
	 */
	tz_update = 1;

	/* pop down the menu */
    	XtPopdown (list);


}       /* End of ApplyCB () */

/**************************************************************************
		from the main window pane
****************************procedure*header******************************/
static void
SetUpTime (Widget widget, XtPointer client_data, XtPointer call_data)
{	
	OlFlatCallData *pFlatData = (OlFlatCallData *) call_data;
	_choice_items	*select;
	char 		ptr[20];
        Arg             arg[2];

        XtSetArg(arg[0], XtNlabel, &labelptr);
        OlFlatGetValues(widget, pFlatData->item_index, arg, 1);

	select = (_choice_items *) pFlatData->items + pFlatData->item_index;	
	switch (select->userData) {
		case 'G':
                        tzptr = strdup (country[0].timezone);
			break;
		case 'M':
                        tzptr = strdup (country[1].timezone);
			break;
	}
	sprintf (TZ_buffer, "%s %s\n", GetGizmoText (tag_tz), tzptr);
	XtVaSetValues (current_selection, XtNstring,    (XtArgVal) labelptr, 0);
	tz_update = 1;
}

/*********************************************************************
	set the timezone in the main window
 *********************************************************************/
void SetTime (Widget wid)
{
	static char	line[BUFSIZ], 	buf[BUFSIZ];
	FILE		*fp, *fp1;
	char		*ptr,*lineptr;
	uid_t		myuid;
	int		retvalue = 0, found = 0;
	
	/* set the ptr to the timezone value that was stored in 
	 * the TZ buffer 
	 */
	ptr = strchr (TZ_buffer, ':');
	ptr+=2;
	
	/* setuid to store /etc/TIMEZONE file */
	myuid = getuid ();
	setuid (0);

	/* open /etc/TIMEZONE and open a /tmp/TIMEOZNE file */
	if ((fp = fopen (TIMEZONE_FILE, "r")) != NULL)  {
		if ((fp1 = fopen (TMP_TIMEZONE, "w")) == NULL) {
			fclose (fp);
			retvalue = 1;
		}
	}
	else
		retvalue = 1;

	/* if the opens failed return */
	if (retvalue) {
  		XtVaSetValues (current_selection, XtNstring, (XtArgVal)NULL, 0);
		setuid (myuid);
		ErrorNotice (wid, GetGizmoText (TXT_Fail));
		return;
	}

        found = 0;
	/* for each line in the /etc/TIMEZONE file */
	while (fgets (buf, BUFSIZ, fp)) {
		lineptr = buf;
		/* if it is a comment */
		if (*lineptr == '#'){ 
                       if (!strncmp (lineptr, THAI, strlen (THAI))) {/*THAI*/
                                /* if current chosen label is Thailand then
                                 * keep the comment line otherwise discard it
                                 */
                                if (!strncmp (GetGizmoText(TXT_Thailand),
                                        labelptr, strlen ( labelptr ))) {
                                        fputs (lineptr, fp1) ;
                                        found = 1;
                                }
                        }
                        else
                                fputs (lineptr, fp1) ;
                }
        }
        if (!found) {
                /* if comment was not found for Thailand
                 * and label chosen was for Thailand
                 * then create the comment line
                 */
                if (!strncmp (GetGizmoText(TXT_Thailand), labelptr,
                                strlen ( labelptr )))
                        fputs (THAI, fp1) ;
        }
	strcpy (line, TZ_VARIABLE);
	strcat (line, TZ_EQUAL);
	strcat (line, ptr);
	strcat (line, exportTZ);
	strcat (line, NEWLINE);
	fputs (line, fp1) ;

	/* close the tmp file and /etc/TIMEZONE */
	fclose (fp);
	fclose (fp1);

	/* move the temporary file to /etc/TIMEZONE */
	if (system (TIME_COMMAND) == -1) 
		ErrorNotice (w_tz, GetGizmoText (TXT_FailedtoCopy));
        else {
                sprintf (line, "%s %s\n", GetGizmoText (tag_tz), labelptr);
                XtVaSetValues (w_tz,    XtNstring, (XtArgVal) line, 0);
        }

        /* remove the temporary file to /etc/TIMEZONE */
        system (TIME_COMMAND2); 

	setuid (myuid);
}


/*********************************************************************
 * OtherCB
 *********************************************************************/
static void
OtherCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	Arg 		arg[20];
	Widget		lca, uca, w_cap, lcaMenu;
	char		buffer[20];
	static Boolean	first = True;

	if (gmtw) 
		XRaiseWindow (XtDisplay (gmtw), XtWindow (gmtw));
	else {
		if (first == True) {
			first = False;
        		SetLabels (GmtItems, XtNumber (GmtItems));
		}
        	gmtw = XtVaCreatePopupShell ("chooser",
                	popupWindowShellWidgetClass, widget,
                	XtNtitle,               (XtArgVal) GetGizmoText (TXT_gmt),
                	0);

        	XtVaGetValues (gmtw,
	                XtNlowerControlArea,    (XtArgVal) &lca,
                	XtNupperControlArea,    (XtArgVal) &uca,
                	0);

 		XtSetArg(arg[0], XtNlabel, GetGizmoText(TXT_GMT));
        	w_cap = XtCreateManagedWidget("gmtcap", captionWidgetClass,
                        uca, arg, 1);

		XtSetArg(arg[0], XtNcharsVisible,  4);
		gmtval = XtCreateManagedWidget("gmt", stepFieldWidgetClass, 
							w_cap, arg, 1);

    		sprintf(buffer, "%d", 0);
        	XtSetArg(arg[0], XtNstring, buffer);
        	XtSetValues(gmtval, arg, 1);
        	XtAddCallback(gmtval, XtNstepped, GetValueCB, (XtPointer)NULL);


        	/* We want an "cancel" and "reset" buttons in both the lower 
		 * control area and in a popup menu on the upper control area.
         	 */
        	lcaMenu = XtVaCreateManagedWidget ("lcaMenu",
                flatButtonsWidgetClass, lca,
                	XtNitemFields,          (XtArgVal) TZFields,
                	XtNnumItemFields,       (XtArgVal) NumTZFields,
                	XtNitems,               (XtArgVal) GmtItems,
                	XtNnumItems,            (XtArgVal) XtNumber (GmtItems),
			0) ;

        	/* Add callbacks to verify and destroy all widget when the 
		 * property sheet goes away
         	 */
        	XtAddCallback (gmtw, XtNverify,VerifyCB,(XtPointer)&PopdownOK);
        	XtAddCallback (gmtw, XtNpopdownCallback, GMTPopdownCB, 
							(XtPointer)NULL);
        	XtPopup (gmtw, XtGrabNone);
	}
}

/*********************************************************************
 * GmtApplyCB
 *********************************************************************/
static void
GmtApplyCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	char 		buffer[10];
	Widget		shell;
        char    	c, *ptr, *val;
	int		gmttime;

	/* get the displayed field */
        XtVaGetValues(gmtval, XtNstring, &val, 0);

	/* convert it to integer */
	gmttime = atoi (val);

	/* validate the time field */
	if (gmttime > 12 || gmttime <  -12 || strlen (val) < 1) {
		OlTextEditClearBuffer (gmtval);
		ErrorNotice (widget, GetGizmoText (TXT_wrongno)); 
		return;
	}

	/* validate for space or alphabets */
	ptr = val;
	while (*ptr) {
		c = *ptr;
		if (isalpha (c) || isspace (c)) {
			OlTextEditClearBuffer (gmtval);
			ErrorNotice (widget, GetGizmoText (TXT_wrongno)); 
			return;
		}
		ptr++;
	}

	/* if the field does not match what was stored
	 * then it has been entered
	 */
	if (strcmp (val, Step_buffer) != 0) 
		sprintf (Step_buffer, "%d", gmttime); 

	/* create the TZ buffer */
	strcpy (buffer, "GMT");
	if (gmttime > 0)
		strcat (buffer, "+");
	strcat (buffer, Step_buffer);
        labelptr = strdup (buffer);
	sprintf (TZ_buffer, "%s %s\n", GetGizmoText (tag_tz), buffer);
  	XtVaSetValues (current_selection, XtNstring,    (XtArgVal) buffer, 0);

	/* reset flag */
	tz_update = 1;

	/* popdown the other menu */
    	shell = XtParent(widget);
    	while (!XtIsShell (shell))
        	shell = XtParent(shell);
    	XtPopdown (shell);
}       /* End of ApplyCB () */

/*********************************************************************
 * GetValue
 *********************************************************************/
static void
GetValueCB(Widget wid, XtPointer client_data, XtPointer call_data) 
{
	OlTextFieldStepped *stp = (OlTextFieldStepped *) call_data;
        int     n;
	Arg     arg[2];
        char    *val;
	int	GMT_integer;

   	XtSetArg(arg[0], XtNstring, &val);
        XtGetValues(wid, arg, 1);

	GMT_integer = atoi (val);
	if (GMT_integer > 12 ) {
        	XtVaSetValues(wid, XtNstring, (XtArgVal) "12", 0);
		return;
	}
	if (GMT_integer <  -12) {
        	XtVaSetValues(wid, XtNstring, (XtArgVal) "-12", 0);
		return;
	}

       	switch(stp->reason) {
               	default:                        return;
               	case OlSteppedIncrement:        n = 1;  break;
               	case OlSteppedDecrement:        n = -1; break;
       	}

	if (GMT_integer == 12 && stp->reason == OlSteppedIncrement) {
		GMT_integer = -13;
		n = 1;
	}
	else if (GMT_integer == -12 && stp->reason == OlSteppedDecrement) {
		GMT_integer = 13;
		n = -1;
	}

	GMT_integer += n;
	
	sprintf (Step_buffer, "%d", GMT_integer); 
       	XtSetArg(arg[0], XtNstring, Step_buffer);
       	XtSetValues(wid, arg, 1);
}

/*********************************************************************
	reset the time to what it was
 *********************************************************************/
void Reset_Time ()
{
	char 	buffer[256];

	if (gettz (buffer) != NULL) {
  		XtVaSetValues (current_selection, 
				XtNstring, (XtArgVal) buffer, 0);
                labelptr = strdup (buffer);
	}
	else {
  		XtVaSetValues (current_selection,XtNstring,(XtArgVal)NULL, 0);
                labelptr = NULL;
        }
}

/***********************************************************
	get the timezone
*************************************************************/
char *
gettz (char *ptr)
{
	FILE		*fp;
	char		*lineptr,  buf[BUFSIZ], tmpbuf[BUFSIZ];	
	int		found = 0,i = 0, j = 0;
        Boolean         Thai = False;
	
	/* if the TIMEZONE file is not there */
	if ((fp = fopen (TIMEZONE_FILE, "r")) == NULL)  
		return NULL;

	/* get each line in /etc/TIMEZONE */
	while (fgets (buf, BUFSIZ, fp)) {

		lineptr = buf;
		while (*lineptr != '\n') {

			/* if it is a comment get next line */
                        if (*lineptr == '#')  {
                                if (!strncmp (THAI, lineptr, strlen (lineptr)))
                                        Thai = True;    /* save boolean value */                                break;                  /* for comparison */
                        }

			/* if the TZ variable is found */
			if (strncmp (TZ_VARIABLE, lineptr, 2) == 0){

				/* get the equal sign */
				while (*lineptr  != '=' && *lineptr != '\n') 
					lineptr++;	
                                if (*lineptr == '=') lineptr++;

				/* get the value of TZ */
				if (*lineptr == ':') {
					while (*lineptr++ == ' ');
					i = 0;
					while (*lineptr != '\n') 
						tmpbuf[i++]=*lineptr++;
					tmpbuf[i++] = '\0';
					found = 1;
					break;
				} 
                               else {
					while (*lineptr == ' ')lineptr++;
					i = 0;
					while (*lineptr != '\n') 
						tmpbuf[i++]=*lineptr++;
					tmpbuf[i++] = '\0';
					found = 1;
					break;
				} /* retrieve the value of TZ */
                                

			} /* if we find the TZ variable */

			lineptr++;	
		} /* for each line */ 

		if (found)
			break;
	} /* read thru each line of the  file */

	/* store the timezone value in the TZ_buffer */
	sprintf (TZ_buffer, "%s %s\n", GetGizmoText (tag_tz), tmpbuf);

	fclose (fp); /* close the /etc/TIMEZONE file */
        if (!i) {
                ptr = NULL; /* nothing was found return  NULL */
        }
        else {
                found = 0;
                for (j = 0; j < XtNumber (country); j++) {
                        if (!strncmp (tmpbuf, country[j].timezone,
                                        strlen (country[j].timezone)))  {
                                /* Thailand is also GMT+8 but the value in
                                 * /etc/TIMEZONE  may be GMT+8 and not refer
                                 * to Thailand at all
                                 */
                                if (!strcmp (GetGizmoText((char *)
                                        country[j].name),
                                        GetGizmoText (TXT_Thailand))) {
                                        if (!Thai)      /* was not thailand */
                                                break;  /* go to !found code */
                                }

                                strcpy (ptr,
                                        GetGizmoText ((char *)country[j].name));                                found = 1;
                                break;
                        }
                }

                if (!found) {
                        for (j = 0; j < i; j++)         /* if item was not in */                                *ptr++ = tmpbuf[j];     /* file then GMT item */                }
        }

        return ptr;
}

 /***************************************************************************
	cancel the popup
 ***************************************************************************/
static void
CancelCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
  	Widget      shell;
	
    	shell = XtParent(widget);
    	while (!XtIsShell (shell))
        	shell = XtParent(shell);
	
    	XtPopdown (shell);
}


static void SetCountryTZ (_choice_items *items, int cnt)
{
        int i;

        for (; --cnt >=0; items++) {
                for (i = 0; i < XtNumber (country); i++)  {
                        if (!strncmp(GetGizmoText ((char *)country[i].name),
                        (char *)items->label, strlen ((char *)items->label)))
                                items->userData = (XtArgVal)
                                                strdup (country[i].timezone);
                }
        }
}       /* End of SetCountryTZ */
