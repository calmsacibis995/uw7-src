#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/list.c	1.78"
#endif

/******************************file*header********************************

    Description:
        This routine uses the flattened list widget (aka. FlatList)
        to display the contents of the /etc/uucp/Systems file.
*/

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrolledWi.h>
#include <Xol/StaticText.h>
#include <Xol/FButtons.h>
#include <Xol/Caption.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#include <Xol/PopupWindo.h>
#include <Xol/Error.h>
#include <Xol/Form.h>
#include <Xol/Category.h>
#include <Xol/ControlAre.h>
#include <Xol/MenuShell.h>
#include <Xol/ChangeBar.h>
#include <Xol/Footer.h>
#include <Xol/FList.h>
#include <Gizmos.h>
#include <STextGizmo.h>
#include <LabelGizmo.h>
#include <MenuGizmo.h>
#include <ChoiceGizm.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include "uucp.h"
#include "error.h"
#define EMPTY 2

void 			SetPropertyAddLabel();
extern void callRegisterHelp(Widget, char *, char *);
extern char *		OlTextFieldGetString();
static int CheckPromptResponse();
extern FocusChangeCB();
extern char *		ApplicationName;
extern caddr_t		PortSpeed();
static void DropProcCB(Widget w, XtPointer client_data, XtPointer call_data);
void NewPageCB(Widget wid, XtPointer client_data, XtPointer call_data);
void SelectHeaderCB(Widget wid, XtPointer client_data, OlFlatCallData *call_data);
void LoginSelectCB(Widget wid, XtPointer client_data, OlFlatCallData *call_data);
void LoginUnselectCB(Widget wid, XtPointer client_data, OlFlatCallData *call_data);
extern void		UnselectSelect();
extern void		FreeNew();
extern void		ApplyNewEntry();
extern void		SetFields();
extern void		ResetFields();
extern void		Warning();
extern void		DisallowPopdown();
extern Boolean		ChangeCB();
extern int		VerifySpeedField();
extern Boolean		VerifyAllFields();
extern Boolean		VerifyNameField();
extern Boolean		VerifyPhoneField();
extern Boolean		VerifyExpectField();
extern void		GetFlatItems();
extern void		ModifyPhoneCB ();
extern Widget		CreateCaption();
extern int		CreateAttrsFile();
extern void             PropPopupCB();
extern void             NotifyUser();

void SelectCB(Widget wid, XtPointer client_data, OlFlatCallData *call_data);
void		UpdateLoginScrollList();
static void		SelectedType();
static void		ModemCB();
static void		DirectCB();
static void		DatakitCB();
static void		PropPopdownCB();
void		        CreateIconCursor();

Widget		w_phone;
DeviceItems	support_type[] = {
	{ "Modem", "ACU" , True},
	{ "Direct Line", "Direct" , True},
#ifdef old
	{ "Data Kit", "DK", True },
#endif
};

typedef enum {
	TypeACU, TypeDirect /*, TypeDK*/
} TypeMenuIndex;

Arg arg[50];
static char **target;

static String itemResources[] = {
    XtNformatData
};


typedef struct {
    XtPointer	name;
    XtPointer	speed;
    XtPointer	phone;
} FormatData;

static HostData *Header;
static HostData headerinfo;
static Setting stype;




static MenuItems  TypeItems[] = {
	{True,	label_modem,	mnemonic_modem, "modem", ModemCB },
	{True,	label_direct,	mnemonic_direct, "uudirect", DirectCB },
#ifdef old
	{True,	label_datakit,	mnemonic_datakit, "datakit", DatakitCB },
#endif
	{ 0 }
};

static MenuGizmo TypeMenu = {
	NULL, "type", "_X_", TypeItems, NULL, NULL, EXC,
        OL_FIXEDROWS,	/* Layout type	*/
        1,		/* Measure	*/
        OL_NO_ITEM	/* Default item	*/
};

static ChoiceGizmo TypeChoice = {
	NULL,
	"type",
	label_modemType,
	&TypeMenu,
	&stype,
};

/* Create the extra item check box */

static Setting extra = {
	"extra", (XtPointer)"_"
};

extern void             HelpCB();
static HelpText AppHelp = {
    title_property, HELP_FILE, help_property,
};

void HandleLoginButtonCB();

static Items loginButtons [] = {
    { HandleLoginButtonCB, NULL, (XA)TRUE, NULL, NULL, (XA)ADD}, /* apply */
    { HandleLoginButtonCB, NULL, (XA)TRUE, NULL, NULL, (XA)MODIFY}, /* reset */
    { HandleLoginButtonCB, NULL, (XA)TRUE, NULL, NULL, (XA)DELETE}, /* cancel */
};

static Menus loginButtonsMenu = {
    "logbuttons",
    loginButtons,
    XtNumber (loginButtons),
    False,
    OL_FIXEDCOLS,
    OL_NONE,
    NULL
};

void HandleButtonCB();

static Items propertyItems [] = {
    { HandleButtonCB, NULL, (XA)TRUE, NULL, NULL, (XA)APPLY}, /* apply */
    { HandleButtonCB, NULL, (XA)TRUE, NULL, NULL, (XA)RESET}, /* reset */
    { HandleButtonCB, NULL, (XA)TRUE, NULL, NULL, (XA)CANCEL}, /* cancel */
    { HelpCB, NULL, (XA)TRUE, NULL, NULL, (XA)&AppHelp},
};

static Menus propertyMenu = {
	"property",
	propertyItems,
	XtNumber (propertyItems),
	False,
	OL_FIXEDROWS,
	OL_NONE,
	NULL
};

Widget
InitLists(form)
Widget form;
{
	register	i;
	int		index;
	char 		buf[128];
	Widget		header;
	Widget		sw, sw1;
	Widget		controlBottom;
	Widget 		loginSeqCaption;
	Widget		footer;
	Widget 		upper;
	Widget		loginScrollWin;
	Widget		category ;
	Widget		control, control2, control3;
	Widget 		w1;
	Widget 		w;
	Widget 		dummy;
	Widget 		currLoginCaption;
	Widget		scrollw, scrollh;
	HostData	*dp;
	void		HandleButtonCB();

#ifdef TRACE
	fprintf(stderr,"InitLists\n");
#endif
	SET_HELP(AppHelp);
	GetFlatItems (sf->filename);

	if (sf->numFlatItems == 0)
		dp = (HostData *) NULL;
	else
		dp = sf->flatItems[0].pField;
	Header = &headerinfo;
	headerinfo.f_name = (XtPointer) GGT(string_system);
	headerinfo.f_class = (XtPointer) GGT(string_speed);
	headerinfo.f_phone = (XtPointer) GGT(string_phone);
	sw1 = XtVaCreateManagedWidget (
		"form1 Window",
		formWidgetClass,
		form,
		XtNweight,		0,
		XtNshadowThickness, 0,
		XtNborderWidth, 0,
		(String)0
	);

	header = XtVaCreateManagedWidget("header",
			flatListWidgetClass,
			sw1,
			XtNviewHeight,		(XtArgVal) 1,
			XtNshadowThickness, (XtArgVal) 0,
			XtNexclusives, True,
			XtNset,	False,
			XtNnoneSet,			(XtArgVal) True,
			XtNdblSelectProc, SelectHeaderCB,
			XtNselectProc,	SelectHeaderCB,
			/*XtNselectable,		(XtArgVal) False,*/
			XtNformatData, 	&headerinfo,
			XtNgranularity,		1,
			XtNitems,		(XtArgVal) &headerinfo,
			XtNnumItems,		(XtArgVal) 1,
			XtNheight,	(XtArgVal) 1,	
			XtNformat,			(XtArgVal) FORMAT,
			XtNtraversalOn,		(XtArgVal) False,
			XtNborderWidth, 1,
			NULL);

	SetValue(header, XtNweight,		0);
	/* Create the scrolling list of indices */

	sw = XtVaCreateManagedWidget (
		"Scrolled Window",
		scrolledWindowWidgetClass,
		form,
		XtNgranularity,		1,
		XtNborderWidth, 1,
		(String)0
	);
	sf->scrollingList = XtVaCreateManagedWidget (
		"Scrolling List",
		flatListWidgetClass,
		sw,
		XtNitems,		sf->flatItems,
		XtNnumItems,		sf->numFlatItems,
		XtNitemFields,		itemResources,
		XtNnumItemFields,	XtNumber(itemResources),
		XtNviewHeight,		VIEWHEIGHT,
		XtNformat,		FORMAT,
		XtNselectProc,		SelectCB,
		XtNselectable,		True,
		XtNdblSelectProc,	PropPopupCB,
		XtNweight,		0,
		XtNdropProc,		DropProcCB,
		XtNnoneSet,			True,
		XtNdragCursorProc,	CreateIconCursor,
		0);

	/* Create the popup shell to contain the entry */

	sf->propPopup = XtVaCreatePopupShell(
			"systemProperties",		/* instance name */
			popupWindowShellWidgetClass,
			form, 
			(String) 0);
	XtAddCallback (
                sf->propPopup,
                XtNverify,
                DisallowPopdown,
                (XtPointer)0
        );
	XtAddCallback (
                sf->propPopup,
                XtNpopdownCallback,
                PropPopdownCB,
                (XtPointer)0
        );

	callRegisterHelp(sf->propPopup, title_property, help_property);
	/*
	 * retrieve the widget IDs of the control areas and the footer panel
	 */

	XtVaGetValues(
		sf->propPopup,
		XtNupperControlArea,  &upper,
		XtNlowerControlArea,	&controlBottom,
		XtNfooterPanel,		&footer,
		0
	);
	sf->category = category = XtVaCreateManagedWidget("category", 
		categoryWidgetClass,
		upper,
		XtNcategoryLabel, (XtArgVal) GGT(title_category),
		XtNlayoutWidth, (XtArgVal) OL_MAXIMIZE,
		XtNlayoutHeight, (XtArgVal) OL_MAXIMIZE,
		0);

	sf->sprop_footer = XtVaCreateManagedWidget("sprop_footer",
			footerWidgetClass,
			footer,
			0);

	sf->pages[0]= XtVaCreateManagedWidget("catpage",
		controlAreaWidgetClass,
		category,
		XtNavailableWhenUnmanaged, (XtArgVal) False,
		XtNpageLabel,		(XtArgVal) GGT(title_category1),
		XtNallowChangeBars,	(XtArgVal) True,
		XtNmeasure, 1,
		XtNborderWidth, 0,
		XtNshadowThickness, 0,
		XtNalignCaptions,	(XtArgVal) True,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		0);
	sf->pages[1]= XtVaCreateManagedWidget("catpage2",
		controlAreaWidgetClass,
		category,
		XtNavailableWhenUnmanaged, (XtArgVal) False,
		XtNpageLabel,		(XtArgVal) GGT(title_category2),
		XtNallowChangeBars,	(XtArgVal) True,
		XtNmeasure, 1,
		XtNborderWidth, 0,
		XtNshadowThickness, 0,
		XtNalignCaptions,	(XtArgVal) True,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		0);
	if (sf->update == False) {
			XtSetMappedWhenManaged(sf->pages[1], False);
	}
	XtAddCallback(category, XtNnewPage, NewPageCB, 0);
	
	XtAddCallback(sf->propPopup, XtNpopdownCallback, PropPopdownCB, 0);
	sf->w_name = (TextFieldWidget) XtVaCreateManagedWidget(
		"name",
		textFieldWidgetClass,
		CreateCaption(sf->pages[0], GGT(label_system)),
		XtNcharsVisible, MAXNAMESIZE, /* 14 */
                XtNmaximumSize, MAX_VISIBLE,
		XtNstring, (dp) ? dp->f_name : NULL,
		(String)0
	);

	/* ChoiceGizmo returns the caption widget, not the flatbutton widget */
	CreateGizmo(sf->pages[0], ChoiceGizmoClass, &TypeChoice, arg, 0);

	sf->w_type = GetChoiceButtons(&TypeChoice);
	PortSpeed(SYSTEMS_TYPE,NEW, sf->pages[0]);
	if (dp != (HostData *) NULL) {
		PortSpeed(SYSTEMS_TYPE, SET, dp->f_class);
	}
	sf->w_phone = (TextFieldWidget) XtVaCreateManagedWidget(
		"phone",
		textFieldWidgetClass,
		w_phone = CreateCaption(sf->pages[0], GGT(label_phone)),
		XtNcharsVisible, MAXPHONESIZE,
                XtNmaximumSize, MAXPHONESIZE, /* 20 characters */
		XtNstring, (dp) ? dp->f_phone : NULL,
		(String)0
	);
	SetValue(w_phone,
		XtNmappedWhenManaged,
		(XtArgVal)((dp) ?
			 !strcmp(dp->f_type, support_type[TypeACU].value) : 1)
		);
	/* comment for passwd field which is now in expect/send array */
	/* although only the first 8 characters will be recognized,
	 * we should not limited the user to input more than 8.
	 * also, the space is allowed in a password, but since we are
	 * feeding the Systems file where spaces is not allowed in
	 * any field.
	 */

	loginSeqCaption = XtVaCreateManagedWidget(
		"loginSeqCaption",
		captionWidgetClass,
		sf->pages[1],
		XtNlabel, GGT(label_loginSeq),
		NULL);
	control2 = XtVaCreateManagedWidget(
		"control2",
		controlAreaWidgetClass,
		sf->pages[1],
		XtNlayoutType, (XtArgVal) OL_FIXEDCOLS,
		XtNmeasure, 2,
		/*XtNcenter, True,*/
		XtNcenter, False,
		XtNalignCaptions, (XtArgVal) True,
		XtNshadowThickness, (XtArgVal) 0,
		(String) 0
	);
	sf->w_prompt = (TextFieldWidget) XtVaCreateManagedWidget(
		"prompt",
		textFieldWidgetClass,
		CreateCaption(control2, GGT(label_prompt)),
		(String)0
	);
	sf->w_response = (TextFieldWidget) XtVaCreateManagedWidget(
		"response",
		textFieldWidgetClass,
		CreateCaption(control2, GGT(label_response)),
		(String)0
	);
		/* dummy unmmanaged caption so the next will align correctly */
	control3 = XtVaCreateManagedWidget(
		"control3",
		controlAreaWidgetClass,
		sf->pages[1],
		XtNlayoutType, (XtArgVal) OL_FIXEDCOLS,
		XtNmeasure, 2,
		XtNcenter, True,
		/*XtNcenter, False,*/
		XtNalignCaptions, (XtArgVal) True,
		XtNshadowThickness, (XtArgVal) 0,
		(String) 0
	);
	dummy = XtVaCreateManagedWidget(
		"currLoginCaption",
		staticTextWidgetClass,
		control3,
		XtNstring, NULL,
		NULL);
	currLoginCaption = XtVaCreateManagedWidget(
		"currLoginCaption",
		staticTextWidgetClass,
		control3,
		XtNstring, GGT(label_currLoginSeq),
		NULL);
	SET_LABEL (loginButtons,0,loginadd);
	SET_LABEL (loginButtons,1,modify);
	SET_LABEL (loginButtons,2,delete);

    if (work->numExpectSend <= 0){
		 /*loginButtons[0].sensitive = False;*/
		 loginButtons[1].sensitive = False;
		 loginButtons[2].sensitive = False;
	}
	AddMenu (control3, &loginButtonsMenu, False);
	loginScrollWin = XtVaCreateManagedWidget(
		"loginScrollWin",
		scrolledWindowWidgetClass,
		control3,
		XtNgranularity,		1,
		XtNvAutoScroll, TRUE,
		XtNviewHeight, 4,
		NULL);
	
	work = (LoginData *) CreateBlankLoginEntry();
	sf->loginScrollList = XtVaCreateManagedWidget (
		"loginScrollingList",
		flatListWidgetClass,
		loginScrollWin,
		XtNnoneSet,			(XtArgVal) True,
		XtNitemFields,		itemResources,
		XtNnumItemFields,	XtNumber(itemResources),
		XtNviewHeight,		LOGIN_VIEWHEIGHT,
		XtNformat,		LOGIN_FORMAT,
		XtNunselectProc,	LoginUnselectCB,
		XtNselectable,		True,
		XtNselectProc,		LoginSelectCB,
		XtNweight,		1,
		XtNdropProc,		DropProcCB,
		XtNdragCursorProc,	CreateIconCursor,
		(String)0
	);
	if (sf->update == False)
		XtSetMappedWhenManaged(sf->loginScrollList, False);

	XtAddCallback (
		(Widget) sf->w_phone,
		XtNmodifyVerification,
		(XtCallbackProc)ModifyPhoneCB,
		(caddr_t) sf->sprop_footer
	);
	XtAddCallback (
		(Widget) sf->w_name,
		XtNverification,
		(XtCallbackProc)ChangeCB,
		(caddr_t) F_NAME
	);
	XtAddCallback (
		(Widget) sf->w_otherSpeed,
		XtNverification,
		(XtCallbackProc)ChangeCB,
		(caddr_t) F_SPEED
	);
	XtAddCallback (
		(Widget) sf->w_response,
		XtNverification,
		(XtCallbackProc)ChangeCB,
		(caddr_t) F_EXPECT2
	);
	XtAddCallback (
		(Widget) sf->w_prompt,
		XtNverification,
		(XtCallbackProc)ChangeCB,
		(caddr_t) F_EXPECT1
	);
	XtAddCallback (
		(Widget) sf->w_phone,
		XtNverification,
		(XtCallbackProc)ChangeCB,
		(caddr_t) F_PHONE
	);
	SET_LABEL (propertyItems,0,add);
	SET_LABEL (propertyItems,1,reset);
	SET_LABEL (propertyItems,2,cancel);
	SET_LABEL (propertyItems,3,help);

    if (sf->update == False) propertyItems[0].sensitive = sf->update;

	XtVaSetValues((Widget)sf->w_name, XtNregisterFocusFunc, FocusChangeCB, 0);
	XtVaSetValues((Widget)sf->w_phone, XtNregisterFocusFunc, FocusChangeCB, 0);
	XtVaSetValues((Widget)sf->w_prompt, XtNregisterFocusFunc, FocusChangeCB, 0);
	XtVaSetValues((Widget)sf->w_response, XtNregisterFocusFunc, FocusChangeCB, 0);
	AddMenu (controlBottom, &propertyMenu, False);
#ifdef TRACE
	fprintf(stderr,"End : InitLists\n");
#endif
	return sw;
} /* InitLists */


void
SetPropertyAddLabel(index, value, mnemonic)
int index;
char *value;
char * mnemonic;
{

#ifdef DEBUG
	fprintf(stderr,"SetPropertyAddLabel: setting property label to %s mnemonic=%c\n",GGT(value),*GGT(mnemonic));
#endif
	OlVaFlatSetValues((Widget) propertyMenu.widget,
			index, 
			XtNlabel, (String) GGT(value), 
			XtNmnemonic, (XtArgVal) *GGT(mnemonic),
			0);
#ifdef TRACE
	fprintf(stderr,"End : SetPropertyAddLabel\n");
#endif
}


 /* HandleLoginButtonCB() - manages events that effect the state of the 
 * the expect/send scrolling window.
 */
void HandleLoginButtonCB(wid, client_data, call_data)
Widget	wid;
caddr_t	client_data;
caddr_t	call_data;
{
	register HostData	*dp;
	register i,j;
	int	action = (int) client_data;
	Widget  popup = sf->propPopup;
	int index,result;
	Setting *setting;
	Boolean	flag;
#ifdef TRACE
	fprintf(stderr,"Start : HandleLoginButtonsCB\n");
#endif
	if (new) 
		dp = new->pField ;
	else 
		dp = sf->flatItems[sf->currentItem].pField;

	ClearFooter(sf->footer); /* clear mesaage area */
#ifdef DEBUG
	fprintf(stderr,"HandleLoginButtonCB work->numAllocated=%d work->currExpectSend=%d work->numExpectSend=%d\n", work->numAllocated, work->currExpectSend, work->numExpectSend);
#endif
	switch (action) {

	/* addd new cases ADD, MODIFY & DELETE for the
		new expect/send scrolling list actions */

	case ADD:
	case APPLY:
#ifdef DEBUG
		fprintf(stderr, "in Add for HandleLoginButtonCB\n"); 
#endif
		/* Place the current contents of the prompt and
		response text fields in the scrolling list.
		If the scroll list is selected then plase the new
		item after it. If no item is selected then place
		the prompt/response at the end of the list /
		/* XtRealloc to add one more entry */
		/* if prompt field is blank don't add  login sequence
				entry  on OK */
		result = CheckPromptResponse();
		if ((result == EMPTY) || (result == INVALID)) return;
		if (work->numExpectSend >= work->numAllocated) 
			AllocateLoginEntry(work,LOGIN_INCREMENT);
	

#ifdef DEBUG
	fprintf(stderr,"ADD with work->currExpectSend=%d\n",work->currExpectSend);
#endif
		if ((work->currExpectSend >= 0) && (work->numExpectSend > 0)) {
			/* add after the current item */
			/* may have to reshuffle current entries */
			/* add a new entry */
			/*for (i=work->numExpectSend; i >= work->currExpectSend && i >0;*/
			for (i=work->numExpectSend; i > work->currExpectSend && i >0;
				i--) {
#ifdef DEBUG
	fprintf(stderr,"i=%d numExpect=%d currExpect=%d\n",i, work->numExpectSend,work->currExpectSend);
#endif
				work->expectFlatItems[i].pExpectSend->f_prompt = 
					work->expectFlatItems[i-1].pExpectSend->f_prompt;
				work->expectFlatItems[i].pExpectSend->f_response = 
					work->expectFlatItems[i-1].pExpectSend->f_response;
			}
		work->expectFlatItems[work->currExpectSend+1].pExpectSend->f_prompt = 
			OlTextFieldGetString((Widget) sf->w_prompt,
			NULL);
		work->expectFlatItems[work->currExpectSend+1].pExpectSend->f_response = 
			OlTextFieldGetString((Widget) sf->w_response,
			NULL);
		work->numExpectSend++;
		} else {
#ifdef DEBUG
	fprintf(stderr,"Add to end numExpect=%d currExpect=%d\n", work->numExpectSend,work->currExpectSend);
#endif
		/* add to end of the list */
		/* FORMAT THE PROMPT/RESPONSE STRINGS for display in the
			scrolling list */
		work->expectFlatItems[work->numExpectSend].pExpectSend->f_prompt = 
			OlTextFieldGetString((Widget) sf->w_prompt,
			NULL);
		work->expectFlatItems[work->numExpectSend].pExpectSend->f_response = 
			OlTextFieldGetString((Widget) sf->w_response,
			NULL);
		/* FORMAT THE PROMPT/RESPONSE STRINGS for display in the
			scrolling list */

			work->numExpectSend += 1;

		}
		OlVaFlatSetValues((Widget) loginButtonsMenu.widget,
			0, /* item index to set sensitive */
			XtNsensitive,
			True,
			/*False,*/
			0);
		work->currExpectSend = -1;
		break;
		


	case MODIFY:
#ifdef DEBUG
		fprintf(stderr, "in Modify for HandleLoginButtonCB\n");
	fprintf(stderr,"work->currExpectSend=%d\n",work->currExpectSend);
#endif
	result = CheckPromptResponse();
	if ((result == EMPTY) || (result == INVALID)) return;
       work->expectFlatItems[work->currExpectSend].pExpectSend->f_prompt =
            OlTextFieldGetString((Widget) sf->w_prompt,
            NULL);
        work->expectFlatItems[work->currExpectSend].pExpectSend->f_response =
            OlTextFieldGetString((Widget) sf->w_response,
            NULL);
	
	work->currExpectSend = -1;
 
		break;


	case DELETE:
	
#ifdef DEBUG
		fprintf(stderr, "in Delete for HandleLoginButtonCB\n");
	fprintf(stderr,"work->currExpectSend=%d work->numExpectSend=%d\n",work->currExpectSend, work->numExpectSend);
#endif
		if (work->numExpectSend > 0 ) {
		for (i=work->currExpectSend; i < work->numExpectSend; i++) {
				work->expectFlatItems[i].pExpectSend->f_prompt = 
					work->expectFlatItems[i+1].pExpectSend->f_prompt;
				work->expectFlatItems[i].pExpectSend->f_response = 
					work->expectFlatItems[i+1].pExpectSend->f_response;
			}
		work->numExpectSend--;
		if (work->currExpectSend == work->numExpectSend) {
			/* we deleted the last one in the list
			so now there is no current entry */
			work->currExpectSend = -1;
		}

		}
		work->currExpectSend = -1;
		break;

	default:
		break;

	}

	UpdateLoginScrollList(work);
	AcceptFocus(sf->w_prompt);

#ifdef TRACE
	fprintf(stderr,"End : HandleLoginButtonsCB\n");
#endif
}

void 
UpdateLoginScrollList(loginp)
LoginData *loginp;
{

#ifdef TRACE
	fprintf(stderr,"Start : UpdateLoginScrollList\n");
#endif
#ifdef DEBUG
fprintf(stderr,"UpdateLoginScrollList loginp->numExpectSend=%d\n",
		loginp->numExpectSend);
fprintf(stderr,"UpdateLoginScrollList loginp->currExpectSend=%d\n",
		loginp->currExpectSend);
#endif

		/* don't allow mapping of the login information if
		they are not a privileged user */

    XtVaSetValues ((Widget)
        sf->loginScrollList,
        XtNitems,       loginp->expectFlatItems,
        XtNnumItems,       loginp->numExpectSend,
        XtNviewHeight,          LOGIN_VIEWHEIGHT,
        XtNitemsTouched,    True,
        (String)0);

#ifdef DEBUG
	fprintf(stderr,"UpdateLoginScrollList: setting viewItem currExpectSend=%d\n", loginp->currExpectSend);
	fprintf(stderr,"UpdateLoginScrollList: numExpectSend=%d\n", loginp->numExpectSend);
#endif

if (loginp->currExpectSend >= 0) {
    XtVaSetValues ((Widget)
        sf->loginScrollList,
        XtNviewItemIndex,   loginp->currExpectSend,
        (String)0);
} else {
	XtVaSetValues(( Widget)
		sf->loginScrollList,
		XtNviewItemIndex,
		loginp->numExpectSend-1,
		(String) 0);
}
	if (loginp->currExpectSend >= 0 ) 
		OlVaFlatSetValues((Widget) sf->loginScrollList,
			loginp->currExpectSend,
			XtNset, True,
			0);
	XtVaSetValues((Widget) sf->w_prompt, XtNstring, NULL, NULL);
	XtVaSetValues ((Widget) sf->w_response, XtNstring, NULL, NULL);
	if (loginp->currExpectSend >= 0) {
	OlVaFlatSetValues((Widget) loginButtonsMenu.widget,
			1, /* item index to set sensitive */
			XtNsensitive,
			True,
			0);
	OlVaFlatSetValues((Widget) loginButtonsMenu.widget,
			2, /* item index to set sensitive */
			XtNsensitive,
			True,
			0);
	} else {
	OlVaFlatSetValues((Widget) loginButtonsMenu.widget,
			2, /* item index to set sensitive */
			XtNsensitive,
			False,
			0);

	/* set add insensitive since we clear out
		the prompt/response text fields each time */
	OlVaFlatSetValues((Widget) loginButtonsMenu.widget,
			1, /* item index to set sensitive */
			XtNsensitive,
			False,
			0);
	OlVaFlatSetValues((Widget) loginButtonsMenu.widget,
			0, /* item index to set sensitive */
			XtNsensitive,
			True,
			/*False,*/
			0);
	}
#ifdef TRACE
	fprintf(stderr,"End : UpdateLoginScrollList\n");
#endif

}

/*
 * HandleButtonCB() - manages events that effect the state of the 
 * entire popup window.
 */
void HandleButtonCB(wid, client_data, call_data)
Widget	wid;
caddr_t	client_data;
caddr_t	call_data;
{
	register HostData	*dp;
	register i;
	int	action = (int) client_data;
	int result;
	Widget  popup = sf->propPopup;
	Setting *setting;
	Boolean	flag;

#ifdef TRACE
	fprintf(stderr,"Start: HandleButtonCB\n");
#endif
	ClearFooter(sf->footer); /* clear mesaage area */
	switch (action) {

	case APPLY:
		/* check for valid prompt field before allowing
			the OK button to popdown the window */
		result = CheckPromptResponse();
		if (result == INVALID) return;
		 if (result == VALID) {
			HandleLoginButtonCB(wid, APPLY ,0);
		}
		ManipulateGizmo((GizmoClass)&ChoiceGizmoClass,
				&TypeChoice,
				GetGizmoValue);
		if (new)
			dp = new->pField;
		else
			dp = sf->flatItems[sf->currentItem].pField;

		ClearFooter(sf->footer); /* clear mesaage area */
		ClearLeftFooter(sf->footer);

		/* LATER: need to check the device avaiablability */
		if (VerifyAllFields(dp) == INVALID)  /* something wrong */
			return;
		/* needs to check for duplication */
		if(new) {
			ApplyNewEntry(); 
			if (VerifySpeedField(SYSTEMS_TYPE,dp->f_class)
				< 0) return;
		 } else {
			SetFields(dp);
			if (VerifySpeedField(SYSTEMS_TYPE,dp->f_class)
				< 0) return;
			dp->changed = True;
			XtVaSetValues ((Widget)
				sf->scrollingList,
				XtNviewHeight,          VIEWHEIGHT,
				XtNitemsTouched,	True,
				(String)0
			);
			/* Re-select the current item since we have */
                        /* given a new list                         */

#ifdef DEBUG
fprintf(stderr,"sf->currentItem =%d\n",sf->currentItem);
#endif
			if (sf->currentItem >= 0) {
			XtVaSetValues ((Widget)
				sf->scrollingList,
				XtNviewItemIndex,	sf->currentItem,
				(String)0
			);
			} else {
			XtVaSetValues ((Widget)
				sf->scrollingList,
				XtNviewItemIndex,	sf->numFlatItems,
				(String)0
			);
			}
            		OlVaFlatSetValues ((Widget)
				sf->scrollingList,
           			sf->currentItem,
				XtNset, True,
				0
                );
		}
		SetSysCopyToFolder();
		ManipulateGizmo((GizmoClass)&ChoiceGizmoClass,
				&TypeChoice,
				ApplyGizmoValue);
		SaveSystemsFile(); 	/* jhg */
		BringDownPopup(popup);
		break;

	case RESET:
		if (new) {
			dp = new->pField;
		} else {
			dp = sf->flatItems[sf->currentItem].pField;
		}
		ResetFields(dp);
		ClearLeftFooter(sf->footer);
		break;

	case CANCEL:
		/* check if this entry is a newly added but net yet applied */
		if (new) {
			FreeNew();
		}
		XtPopdown(sf->propPopup);
		callRegisterHelp(sf->toplevel, title_setup, help_setup);
		break;

	default:
		Warning("HandleButtonCB: Unknown action %d\n", action);
		break;
	}
#ifdef TRACE
	fprintf(stderr,"End: HandleButtonCB\n");
#endif

	return;
} /* HandleButtonCB */


Boolean
ChangeCB (textField, which, tf_verify)
Widget	textField;
int	which;
OlTextFieldVerify       *tf_verify;
{
	char *		string;
	char *speed;

	string = tf_verify->string;
#ifdef TRACE
	fprintf(stderr,"Start: ChangeCB\n");
#endif

	ClearLeftFooter(sf->sprop_footer); /* clear mesaage area */
	switch (which) { 
	case F_NAME:
#ifdef debug
		printf ("ChangeCB for name field\n");
#endif
		if (VerifyNameField(string) == INVALID)  {
			tf_verify->ok = False;
			return(INVALID);
		}
		break;
	case F_PHONE:
#ifdef debug
		printf ("ChangeCB for phone field\n");
#endif
		if (VerifyPhoneField(string) == INVALID) {
			tf_verify->ok = False;
			return(INVALID);
		}
		break;
	case F_EXPECT1:
#ifdef DEBUG
		printf("ChangeCB for prompt field\n");
#endif
	
		/* use 1 in call to indicate prompt field is being checked */
		if (VerifyExpectField(string, 1) == INVALID) {
			AcceptFocus(sf->w_prompt);
			tf_verify->ok = False;
			return INVALID;
		}
		/* set Add button sensitive */

		break;
	case F_SPEED:

		speed = strdup((String)PortSpeed(SYSTEMS_TYPE, GET,0));
#ifdef DEBUG
		fprintf(stderr,"ChangeCB for other speed field speed=%s\n",speed);
#endif

		if ((speed != NULL) && ((strcmp(speed, "Other")) == 0)) {
		if(VerifySpeedField(SYSTEMS_TYPE,string) < 0) {
			tf_verify->ok = False;
			XtFree(speed);
			return INVALID;
			}
		}
		XtFree(speed);
		break;
	case F_EXPECT2:
#ifdef DEBUG
		printf("ChangeCB for response field\n");
#endif
		/* use 2 in call to indicate response field is being checked*/
		if (VerifyExpectField(string, 2) == INVALID) {
			AcceptFocus(sf->w_response);
			tf_verify->ok = False;
			return INVALID;
		}
		break;
	}
#ifdef TRACE
	fprintf(stderr,"End: LoginCB\n");
#endif
	return(VALID);
} /* ChangeCB */

/* Perform the form-wise checking */
Boolean
VerifyAllFields(dp)
HostData	*dp;
{
	char *	phone;
#ifdef TRACE
	fprintf(stderr,"Start: VerifyAllFields\n");
#endif

	if(VerifyNameField((XtPointer) OlTextFieldGetString
		((Widget) sf->w_name, NULL)) == INVALID) {
		return(INVALID);
	}
	if ((dp->f_class) && (strcmp( dp->f_class,"Any")!=0)) {
		if(VerifySpeedField(SYSTEMS_TYPE,(XtPointer) OlTextFieldGetString
			((Widget) sf->w_otherSpeed, NULL)) < 0) {
			return(INVALID);
		}
	}
	if(VerifyPhoneField((XtPointer) OlTextFieldGetString
		((Widget) sf->w_phone, NULL)) == INVALID) {
		return(INVALID);
	}
#ifdef TRACE
	fprintf(stderr,"End: VerifyAllFields\n");
#endif
	return(VALID);
}


int
VerifySpeedField(type,string)
int type;
String	string;
{
	int 	deviceType;
	char *speed;

#ifdef DEBUG
	fprintf(stderr,"Start: VerifySpeedField type=%d\n", type);
	fprintf(stderr,"Start: VerifySpeedField speed=%s\n", string);

#endif
        speed = strdup((String)PortSpeed(type, GET,0));

       if ((speed != NULL) && ((strcmp(speed, "Other")) != 0)) {
			XtFree(speed);
				/* not other field */
			return(0);
	}
#ifdef DEBUG
	fprintf(stderr,"speed=%s\n",speed);
#endif
	if ((string != NULL) && (strcmp(string, "14400") == 0)) {
			XtFree(speed);
			if (type == SYSTEMS_TYPE) {
				NotifyUser(sf->toplevel, GGT(string_resetSpeed));
			} else {
				DeviceNotifyUser(df->toplevel, GGT(string_resetSpeed));
			}
			PortSpeed(type,SET, "19200");
			return (-2);

	}
	if ((string != NULL) && (strcmp(string, "28800") == 0)) {
			XtFree(speed);
			if (type == SYSTEMS_TYPE) {
				NotifyUser(sf->toplevel, GGT(string_resetSpeed));
			} else {
				DeviceNotifyUser(df->toplevel, GGT(string_resetSpeed));
			}
			PortSpeed(type,SET, "38400");
			return(-2);
	}


	XtFree(speed);

	if(strlen(string) == 0) {
		OlCategorySetPage(sf->category, sf->pages[0]);
		if (type == SYSTEMS_TYPE) {
			NotifyUser(sf->toplevel, GGT(string_blankSpeed));
		} else {
			DeviceNotifyUser(df->toplevel, GGT(string_blankSpeed));
		}
		return (-1);
	}
	if((strchr(string, ' ') != NULL)) {
		if (type == SYSTEMS_TYPE) {
			NotifyUser(sf->toplevel, GGT(string_badExpect));
		} else {
			DeviceNotifyUser(df->toplevel, GGT(string_badExpect));
		}
		return (-1);
	}
#ifdef TRACE
	fprintf(stderr,"End: VerifySpeedField\n");
#endif
	return (0);

}



/* String Ok: return VALID, otherwise INVALID */
Boolean
VerifyPhoneField(string)
String	string;
{
	int 	deviceType;
	static Boolean first_time = True;
	static char *badPhone, *blankPhone;

#ifdef TRACE
	fprintf(stderr,"Start: VerifyPhoneField\n");
#endif
	if (first_time) {
		first_time = False;
		badPhone = GGT(string_badPhone);
		blankPhone = GGT(string_blankPhone);
	}
	if(strlen(string) == 0) {
		OlCategorySetPage(sf->category, sf->pages[0]);
        NotifyUser(sf->toplevel, blankPhone);
		return (INVALID);
	}
	ManipulateGizmo((GizmoClass)&ChoiceGizmoClass,
				&TypeChoice,
				GetGizmoValue);
	deviceType = (int) stype.current_value;
	/* is TypeACU ? */
	if ( deviceType == TypeACU )
		if (strlen(string) != strspn(string, "0123456789=-*#")) {
            NotifyUser(sf->toplevel, badPhone);
			return (INVALID);
		}
#ifdef old
	else if ( deviceType == TypeDK )
		return(VerifyNameField(string));
#endif
	return (VALID);
} /* VerifyPhoneField */

/* String Ok: return VALID, otherwise INVALID */
Boolean
VerifyNameField(string)
String	string;
{
	static Boolean first_time = True;
	static char *badName, *blankName;
#ifdef TRACE
	fprintf(stderr,"VerifyNameField\n");
#endif

	if (first_time) {
		first_time = False;
		badName = GGT(string_badName);
		blankName = GGT(string_blankName);
	}
	if(strlen(string) == 0) {
		OlCategorySetPage(sf->category, sf->pages[0]);
        NotifyUser(sf->toplevel, blankName);
		return (INVALID);
	}
	if(isdigit(string[0]) || (strchr(string, ' ') != NULL)) {
                NotifyUser(sf->toplevel, badName);
		return (INVALID);
	}
	return (VALID);
} /* VerifyNameField */

/* String Ok: return VALID, otherwise INVALID */
Boolean
VerifyExpectField(string, action)
String	string;
int action;
{

#ifdef DEBUG
	fprintf(stderr,"VerifyExpectField\n");
#endif
	if(strlen(string) == 0) {
		if (action == 1)  {
			/* action 1 is for prompt field */
			NotifyUser(sf->toplevel, GGT(string_badPrompt)); 
			return (INVALID);
		} else {
			/* response field */
			return (EMPTY);
		}
	}
		/* check for spaces in the field */
	if((strchr(string, ' ') != NULL)) {
                NotifyUser(sf->toplevel, GGT(string_badExpect));
		return (INVALID);
	}
	return (VALID);
} /* VerifyExpectField */

static void
PropPopdownCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	/* check if this entry is a newly added but net yet applied */
#ifdef TRACE
	fprintf(stderr,"PropPopdownCB\n");
#endif
	ClearFooter(sf->footer); /* clear mesaage area */
	ClearLeftFooter(sf->sprop_footer); /* clear mesaage area */
		/* reset to first page before popdown so first page always
		comes up first */
	OlCategorySetPage(sf->category, sf->pages[0]);
	callRegisterHelp(sf->toplevel, title_setup, help_setup);
	if (new) {
		FreeNew();
		UnselectSelect();
	}
}

static void
ModemCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	static Boolean first_time = True;
	static char	*phone_label;

#ifdef TRACE
	fprintf(stderr,"ModemCB\n");
#endif
	if (first_time == True) {
		first_time = False;
		phone_label = GGT(label_phone);
	}
	ClearLeftFooter(sf->sprop_footer); /* clear mesaage area */
	SetValue( sf->w_phone,
		XtNstring,		"",
		NULL);
	SetValue( w_phone,
		XtNmappedWhenManaged,	True,
		NULL);
	SetValue( w_phone, XtNlabel, phone_label, NULL);
} /* ModemCB */

static void
DirectCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
#ifdef TRACE
	fprintf(stderr,"DirectCB\n");
#endif
	SetValue( sf->w_phone,
		XtNstring,		"direct",
		NULL);
	SetValue( w_phone,
		XtNmappedWhenManaged,	False,
		NULL);
	ClearLeftFooter(sf->sprop_footer); /* clear mesaage area */
} /* DirectCB */

static void
DatakitCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	HostData	*dp;
	static Boolean first_time = True;
	static char	*destination_label;

#ifdef TRACE
	fprintf(stderr,"DatakitCB\n");
#endif
	ClearLeftFooter(sf->sprop_footer); /* clear mesaage area */
	if (first_time == True) {
		first_time = False;
		destination_label = GGT(label_destination);
	}

	dp = sf->flatItems[sf->currentItem].pField;

	SetValue( sf->w_phone,
		XtNstring, (new) ? OlTextFieldGetString((Widget)sf->w_name, NULL)
					:dp->f_name,
		NULL);
	SetValue( w_phone,
		XtNmappedWhenManaged,	True,
		NULL);
	SetValue( w_phone, XtNlabel, destination_label, NULL);
} /* DatakitCB */

void
SetFields(hp)
HostData	*hp;
{
	int	index;
	LoginData *l = hp->loginp;

#ifdef TRACE
	fprintf(stderr,"SetFields\n");
#endif
	if (hp->f_name) {
		XtFree (hp->f_name);
		hp->f_name = NULL;
	}
	if (hp->f_type) {
		XtFree (hp->f_type);
		hp->f_type = NULL;
	}
	if (hp->f_class) {
		XtFree (hp->f_class);
		hp->f_class = NULL;
	}
		/* update items from work to hp */

	UpdateLoginList(work, l);
	hp->f_name = OlTextFieldGetString ((Widget)sf->w_name, NULL);
	hp->f_class = strdup((String) PortSpeed(SYSTEMS_TYPE, GET, 0));
	if ((hp->f_class == NULL) ||
		((hp->f_class != NULL) &&
		(strcmp(hp->f_class, "Other") == 0))) {
			/* if other then get the value for speed 
				from the textfield */
			if (hp->f_class) XtFree(hp->f_class);
			hp->f_class = OlTextFieldGetString(sf->w_otherSpeed,
				NULL);
	}
#ifdef DEBUG
	fprintf(stderr, "SetFields : speed=%s\n", hp->f_class);
#endif
	if (hp->f_phone) {
		XtFree (hp->f_phone);
	}
	hp->f_phone = OlTextFieldGetString ((Widget)sf->w_phone, NULL);
	index = (int) stype.current_value;
	hp->f_type = strdup((char *)support_type[index].value);
	SetSysCopyToFolder();
} /* SetFields */

void
ResetFields(hp)
HostData	*hp;
{
	char buf[BUFSIZ];
	register i;
	int	 index;
	LoginData *l = hp->loginp;

#ifdef TRACE
	fprintf(stderr,"ResetFields \n");
#endif
		/* update from hp to work  - reset the original values */
	UpdateLoginList(l, work);
	SetValue (
		sf->w_name,
		XtNstring, hp->f_name
	);
	PortSpeed(SYSTEMS_TYPE, SET, hp->f_class);
	if ((strcmp(hp->f_class, "Other"))== 0) {
#ifdef DEBUG	
	fprintf(stderr, "Reset Fields with speed Other\n");
#endif
		SetValue(
			sf->w_otherSpeed,
			XtNstring, hp->f_class
		);

	}	
	SetValue (
		sf->w_phone,
		XtNstring, hp->f_phone
	);
	SetSysCopyToFolder();
	for (i=0; i< XtNumber(support_type); i++)
		if (!strcmp(support_type[i].value, hp->f_type))
			break;
	if (i == XtNumber(support_type)) {
		index = 0;
		sprintf (buf, GGT(string_useModem));
		XtVaSetValues (sf->footer, XtNstring, buf, (String)0);
	} else  index = i;
	stype.previous_value = (XtPointer)index;
	SelectedType(hp);
	if (XtIsRealized(sf->propPopup)) {
		ManipulateGizmo (
			(GizmoClass)&ChoiceGizmoClass,
			&TypeChoice,
			ResetGizmoValue
		);
	}
} /* ResetFields */

static void
SelectedType(dp)
HostData	*dp;
{
	char buf[BUFSIZ];
	register i;
	int	 index;
	static Boolean first_time = True;
	static char	*phone_label;
	static char	*destination_label;
#ifdef TRACE
	fprintf(stderr,"Start: SelectedType\n");
#endif

	if (first_time == True) {
		first_time = False;
		phone_label = GGT(label_phone);
		destination_label = GGT(label_destination);
	}

	for (i=0; i< XtNumber(support_type); i++)
		if (!strcmp(support_type[i].value, dp->f_type))
			break;
	if (i == XtNumber(support_type)) {
		index = 0;
		sprintf (buf, GGT(string_useModem));
		XtVaSetValues (sf->footer, XtNstring, buf, (String)0);
	} else  index = i;

	XtSetArg (arg[0], XtNset, TRUE);
	OlFlatSetValues(
		sf->w_type,
		index,   /* sub-item to modify */
		arg,
		1
	);
	SetValue(w_phone,
		XtNmappedWhenManaged,
		(XtArgVal)(strcmp(dp->f_type, support_type[TypeDirect].value) != 0)
		);
	if (strcmp(dp->f_type, support_type[TypeACU].value) == 0)
		SetValue( w_phone, XtNlabel, phone_label, NULL);
#ifdef old
	if (strcmp(dp->f_type, support_type[TypeDK].value) == 0)
		SetValue( w_phone, XtNlabel, destination_label, NULL);
#endif
} /* SelectedType */


UpdateLoginList(from, to)
LoginData *from;
LoginData *to;
{
	int i, need;

#ifdef TRACE
	fprintf(stderr,"UpdateLoginList\n");
#endif

	if (to->numExpectSend) FreeLoginEntries(to);
	if (to->numAllocated < from->numExpectSend){
			/* free any existing entries */
		need = from->numExpectSend - to->numAllocated;
			/* if we have additional entries then allocate space */
		AllocateLoginEntry(to, need);
	}
	for (i=0; i < from->numExpectSend; i++) {
		to->expectFlatItems[i].pExpectSend->f_prompt =
			XtNewString(from->expectFlatItems[i].pExpectSend->f_prompt);
		to->expectFlatItems[i].pExpectSend->f_response =
			XtNewString(from->expectFlatItems[i].pExpectSend->f_response);
	}
	to->numExpectSend = from->numExpectSend;
	to->currExpectSend = from->currExpectSend;
	UpdateLoginScrollList(to);
}


void
CreateIconCursor (Widget w, XtPointer client_data, XtPointer call_data)
{
    OlFlatDragCursorCallData	*cursorData =
					(OlFlatDragCursorCallData *) call_data;
    Display			*dpy = XtDisplay(w);
    DmGlyphPtr			glyph;
    XColor			white;
    XColor			black;
    static unsigned int		xHot, yHot;
    static Cursor		cursor;
    static Boolean		first = True;

#ifdef TRACE
	fprintf(stderr,"Start: CreateIconCursor\n");
#endif
    if (first)
    {
	XColor junk;

	first = False;
	XAllocNamedColor (dpy, DefaultColormapOfScreen (XtScreen (w)),
			  "white", &white, &junk);
	XAllocNamedColor (dpy, DefaultColormapOfScreen (XtScreen (w)),
			  "black", &black, &junk);

	glyph = DmGetCursor (XtScreen (w), "netnode.icon");
	if (glyph)
	{
	    xHot = glyph->width / 2;
	    yHot = glyph->height / 2;
	    cursor = XCreatePixmapCursor (dpy, glyph->pix, glyph->mask,
					  &black, &white, xHot, yHot);
	}
	else
	{
	    cursor = None;
	    xHot = yHot = 0;
	}
    }
    
    cursorData->yes_cursor = cursor;
    cursorData->x_hot = xHot;
    cursorData->y_hot = yHot;
}	/* End of CreateIconCursor () */


static void
CleanUpCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	char *fname;
	char msg[BUFSIZ];
#ifdef TRACE
	fprintf(stderr,"CleanupCB\n");
#endif
	sprintf(msg, GGT(string_installDone),
		(fname=strrchr(*target, '/'))? fname + 1: *target);
	PUTMSG(msg);
	if (*target) XtFree (*target);
} /* CleanUpCB */

static void
DropProcCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	HostData		*dp;
	FILE			*attrp;
	OlFlatDropCallData	*drop = (OlFlatDropCallData *)call_data;
	static char 		node_directory[PATH_MAX];
	char 			buf[PATH_MAX];
	static Boolean		first_time = True;
	struct stat		stat_buf;
	int			index, num;
	DmItemPtr		itemp;
	DmObjectPtr		op;

#ifdef TRACE
	fprintf(stderr,"Start: DropProcCB\n");
#endif
	dp = sf->flatItems[sf->currentItem].pField;

	/* check to see if the local machine has been selected */
	if (strcmp(dp->f_name, sf->nodeName) == 0) {
                NotifyUser(sf->toplevel, string_sameNode);
		return;
	}

	if (first_time) {
		first_time = False;
		sprintf (node_directory, "%s/.node", sf->userHome);
	}
	/* check the node directory is there or not. */
	/* if not, then create it			*/

	if (!DIRECTORY(node_directory, stat_buf) ) {
		if (mkdir(node_directory, DMODE) == -1) {
			NotifyUser(sf->toplevel, string_noAccessNodeDir);
			return;
		}
		if (chown(node_directory, getuid(), getgid()) == -1) {
			NotifyUser(sf->toplevel, string_noAccessNodeDir);
			return;
		}
	} else
		if ( stat_buf.st_mode != DMODE ) {
			NotifyUser(sf->toplevel, string_noAccessNodeDir);
			return;
		}

#ifdef debug
	fprintf(stderr,"the DMODE is: %o\n", DMODE);
	fprintf(stderr,"the mode for %s is: %o\n", node_directory,
						stat_buf.st_mode);
#endif

	itemp = (DmItemPtr ) drop->item_data.items;
	num = drop->item_data.num_items;
	if (num = 1) {
		sprintf (buf, "%s/%s", node_directory, dp->f_name);
		target = (char **)malloc(sizeof(char *) * (1 + 1));
		*(target + 1) = NULL;
		*target = strdup(buf);
		attrp = fopen( *target, "w");
		if (attrp == (FILE *) 0) {
			NotifyUser(sf->toplevel, string_noAccessNodeDir);
			return;
		}

		if (chmod( *target, MODE) == -1) {
			NotifyUser(sf->toplevel, string_noAccessNodeDir);
			return;
		}

		/* put the node's properties here */
		fprintf( attrp, "SYSTEM_NAME=%s\n", dp->f_name);
		fprintf( attrp, "LOGIN_NAME=%s\n", sf->userName);
		fprintf( attrp, "DISPLAY_CONN=\n");
		fprintf( attrp, "TRANSFER_FILE_TO=\n");
		fprintf( attrp, "TRANSFER_FILE_USING=\n");
		fprintf( attrp, "COPY_FILE_TO=\n");
		fprintf( attrp, "ALWAYS_CONFIRM=\n");
		fprintf( attrp, "CONN_CONFIRM=\n");
		fprintf( attrp, "FTP_CONFIRM=\n");
		fprintf( attrp, "XFER_OPTION=%s\n", "UUCP");
		(void) fflush(attrp);
		fclose( attrp );
	} else {
		NotifyUser(sf->toplevel, string_noMultiple);
		return;
	}

	DtNewDnDTransaction(
		w,				/* owner widget */
		target,				/* file list */
		DT_B_STATIC_LIST,		/* options */
		drop->root_info->root_x,	/* root x */
		drop->root_info->root_y,	/* root y */
		drop->ve->xevent->xbutton.time,	/* timestamp */
		drop->dst_info->window,		/* dst window */
		DT_LINK_OP,			/* dnd hint */
		NULL,				/* del proc */
		CleanUpCB,			/* state proc */
		(XtPointer) *target		/* client data */
		);

} /* DropProcCB */


void
LoginUnselectCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
OlFlatCallData *call_data;
{

	register HostData	*dp;
	int itemIndex = call_data->item_index;
#ifdef TRACE
	fprintf(stderr, "In LoginUnselectCB\n");
#endif
	work->currExpectSend = itemIndex;
#ifdef DEBUG
	fprintf(stderr,"setting work->currExpectSend to %d\n",itemIndex);
#endif
	
	/* Select the new item */
	OlVaFlatSetValues ((Widget)
		sf->loginScrollList,
		work->currExpectSend,
		XtNset,	False,
		(String)0
		);
}

void
LoginSelectCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
OlFlatCallData *call_data;
{
	register HostData	*dp;
	int itemIndex = call_data->item_index;

	/* callback for login sequence scrolling list */

#ifdef TRACE
	fprintf(stderr, "In LoginSelectCB\n");
#endif
	work->currExpectSend = itemIndex;
#ifdef DEBUG
	fprintf(stderr,"LoginSelectCBsetting work->currExpectSend to %d\n",itemIndex);
#endif
	
	/* Select the new item */
	OlVaFlatSetValues ((Widget)
		sf->loginScrollList,
		work->currExpectSend,
		XtNset,	True,
		(String)0
	);
#ifdef DEBUG
fprintf(stderr,"LoginSelectCB setting viewItem currExpectSend=%d\n", work->currExpectSend);
#endif
	XtVaSetValues ((Widget)
		sf->loginScrollList,
		XtNviewItemIndex, work->currExpectSend,
		(String)0
	);
	/* move value to prompt & response box */
	
	
	OlTextEditClearBuffer((Widget)sf->w_prompt);
	OlTextEditClearBuffer((Widget)sf->w_response);
#ifdef DEBUG
	fprintf(stderr,"itemIndex=%d prompt=%s response=%s\n",itemIndex,
		work->expectFlatItems[itemIndex].pExpectSend->f_prompt,
		work->expectFlatItems[itemIndex].pExpectSend->f_response);

#endif
	OlTextEditInsert((Widget)sf->w_prompt, work->expectFlatItems[itemIndex].pExpectSend->f_prompt, strlen(work->expectFlatItems[itemIndex].pExpectSend->f_prompt));
	OlTextEditInsert((Widget)sf->w_response, work->expectFlatItems[itemIndex].pExpectSend->f_response, strlen(work->expectFlatItems[itemIndex].pExpectSend->f_response));

	/* set add, delete &  modify sensitive since we copy 
		the prompt/response text fields each time */
	OlVaFlatSetValues((Widget) loginButtonsMenu.widget,
			1, /* item index to set sensitive */
			XtNsensitive,
			True,
			0);
	OlVaFlatSetValues((Widget) loginButtonsMenu.widget,
			2, /* item index to set sensitive */
			XtNsensitive,
			True,
			0);
	OlVaFlatSetValues((Widget) loginButtonsMenu.widget,
			0, /* item index to set sensitive */
			XtNsensitive,
			True,
			0);
#ifdef TRACE
	fprintf(stderr,"End: LoginSelectCB\n");
#endif
} /* LoginSelectCB */

void
SelectHeaderCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
OlFlatCallData *call_data;
{
	int itemIndex = call_data->item_index;

        /* UnSelect the heading item */
        OlVaFlatSetValues ((Widget)
                wid,
                itemIndex,
                XtNset, False,
                (String)0
        );

    XtVaSetValues ((Widget)
        wid,
        XtNitems,       &headerinfo,
        XtNnumItems,       1,
        XtNviewHeight,      1,
        XtNitemsTouched,    True,
        (String)0);

}

void
SelectCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
OlFlatCallData *call_data;
{
	register HostData	*dp;
	int itemIndex = call_data->item_index;
	static Boolean first_time = True;
	static char	*phone_label;
	static char	*destination_label;

#ifdef TRACE
	fprintf(stderr,"Start: SelectCB\n");
#endif
	if (first_time == True) {
		first_time = False;
		phone_label = GGT(label_phone);
		destination_label = GGT(label_destination);
	}
	ClearFooter(sf->footer); /* clear mesaage area */
	ClearLeftFooter(sf->footer);
	if (new) FreeNew();
	sf->currentItem = itemIndex;
	dp = sf->flatItems[itemIndex].pField;
	UnselectSelect ();
	if (strcmp(dp->f_type, support_type[TypeDirect].value) != 0) {
		SetValue( w_phone, XtNmappedWhenManaged, True, NULL);
		if (strcmp(dp->f_type, support_type[TypeACU].value) == 0)
			SetValue( w_phone, XtNlabel, phone_label, NULL);
	} else
		SetValue( w_phone, XtNmappedWhenManaged, False, NULL);
#ifdef TRACE
	fprintf(stderr,"End: SelectCB\n");
#endif
} /* SelectCB */

void NewPageCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	OlCategoryNewPage *pageinfo;
	pageinfo = call_data;

#ifdef TRACE
	fprintf(stderr,"NewPageCB pages[1]=%0x\n", sf->pages[1]);
#endif
	OlCategorySetPage(sf->category, pageinfo->new_page);
	if (pageinfo->new_page == sf->pages[1]) {
		AcceptFocus(sf->w_prompt);
	} else {
		AcceptFocus(sf->w_name);
	}

	
	ClearLeftFooter(sf->sprop_footer);
}

static
int CheckPromptResponse()
{
char *prompt;
char *response;
	/* check response */
#ifdef DEBUG
fprintf(stderr,"CheckPromptResponse\n");
#endif

response = OlTextFieldGetString((Widget)sf->w_response, NULL);
prompt =  OlTextFieldGetString((Widget)sf->w_prompt, NULL);

	/* if both are null and we are on OK button then
		return ok */

#ifdef DEBUG
fprintf(stderr,"prompt=%s response=%s\n", prompt, response);
if (*prompt == NULL) fprintf(stderr,"*prompt is null\n");
if (*response == NULL) fprintf(stderr,"*response is null\n");
#endif 

if ((*prompt == NULL) && (*response == NULL)) return EMPTY;


	/* check prompt field */
if ((VerifyExpectField(prompt, 1)) == INVALID) {
	XtFree(prompt);
	XtFree(response);
#ifdef DEBUG
	fprintf(stderr,"returning INVALID from CheckPromptResponse\n");
#endif
	AcceptFocus(sf->w_prompt);
	return INVALID;
	}
	
if ((VerifyExpectField(response, 2)) == INVALID) {
	XtFree(prompt);
	XtFree(response);
#ifdef DEBUG
	fprintf(stderr,"returning INVALID from CheckPromptResponse\n");
#endif
	AcceptFocus(sf->w_response);
	return INVALID;
	}

XtFree(prompt);
XtFree(response);
return VALID;
}

