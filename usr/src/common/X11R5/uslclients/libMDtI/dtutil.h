#ifndef NOIDENT
#ident	"@(#)libMDtI:dtutil.h	1.2"
#endif

#ifndef _dtutil_h
#define _dtutil_h

#include <DesktopP.h>

#ifdef FLH_REMOVE  

     IS ANY OF THIS STUFF USED??? 
    
    REPLACE IT AS NEEDED



/* options for DmCreatePromptBox() */
#define DM_B_SHELL_CREATED		(1 << 0)
#define DM_B_DONT_POPUP_SHELL		(1 << 1)
#define DM_B_DONT_CREATE_BUTTONS	(1 << 2)


typedef Widget (*PFW) ();
typedef void   (*PFV) ();

typedef struct DmInput
   {
   int              Length;
   int              Width;
   char *           Caption;
   char *           Temporary;
   char *           Current;
   char *           CaptionName;
   char *           InputName;
   Widget           CaptionWidget;
   Widget           InputWidget;
   Widget	    ControlWidget;
   int              FirstCall;
   } DmInput;

typedef struct DmAlternatives
      {
      XtArgVal      Label;
      XtArgVal      Set;
      XtArgVal      Default;
      XtArgVal      Dim;
      } DmAlternatives;

typedef struct Choice
   {
   int              Type;
   int              Number;
   char *           Caption;
   unsigned long    Default;
   unsigned long    Temporary;
   unsigned long    Current;
   unsigned long    Refresh;
   char *           CaptionName;
   char *           ChoiceName;
   Widget           CaptionWidget;
   Widget           ChoiceWidget;
   int              FirstCall;
   } Choice;

/* manish,this struct seems ok, for now !! */
typedef struct dmMenuItems {
	XtArgVal	/* String */	label;
	XtArgVal	/* char   */ mnemonic;
	XtArgVal	/* Widget */	popup_menu;
	XtArgVal	/* (*)() */	select;
	XtArgVal	/* Boolean */	sensitive;
} DmMenuItemsRec, *DmMenuItemsPtr;

typedef struct dmPopup *DmPopupPtr;
typedef struct dmPopup
   	{
	char *	 	label;
   	char *           name;
   	DmMenuItemsPtr  items;
   	Widget           parent;
	Widget		 popup;	/* manish - can this be taken out? */
   	Widget           child;
} DmPopupRec;


/*
 * This structure stores dynamic information needed to pop up a command window
 *
 */

typedef struct {
	char	*previous;	/* file name displayed previously */
	char	*current;	/* new user input */
	Widget	shell;  	/* popup shell */
	Widget	input;		/* input field, need this to retrieve input */
	XtArgVal *button_items; /* button item list used in control area */
	XtPointer userdata;	/* additional info. other prompts (e.g. Link)*/
	Boolean flag;
} DmPromptRec, *DmPromptPtr;

/*
 * This structure stores static information about a command window
 */
typedef struct {
	char	*title;			/* command window title */
	char	*action_label;		/* action button label */
	char	*cancel_label;		/* cancel button label */
	char	action_mnemonic;	/* mnemonic for action button */
	char	cancel_mnemonic;	/* mnemonic for cancel button */
	XtCallbackProc	action_proc;	/* select proc for action button */
	XtCallbackProc	cancel_proc;	/* select proc for cancel button */
	char	*caption_label;		/* label for the textfiled */
} DmPromptInfoRec, *DmPromptInfoPtr;

/*
 * This structure stores additional information needed in INstall and Link
 * prompt box. The information is hung to via userdata field in DmPromptRec.
 *
 */

typedef struct {
	char	*caption_label;		/* label for additional TF */
	char	*current;		/* current val. in TF */
	Widget	widget;			/* input TF widget or Flat excl. */
} DmInstallTBdataRec, *DmInstallTBdataPtr;

/*
 * This structure stores additional information needed for Link prompt box.
 * This information is hung to promptbox structure via userdata field
 *
 */

typedef struct {
    Widget w;			/* widget id of flat buttons */
    struct {
	struct {
	    XtArgVal	label;
	    XtArgVal	mnemonic;
	    XtArgVal	set;
	} hardLink, softLink;
    } settings;

} DmLinkTypeRec, *DmLinkTypePtr;

/*
 * This structure stores static information about a Link commnad window
 */

typedef struct {
	char * hl_label;
	char * sl_label;
	char   hl_mne;
	char   sl_mne;
	char * caption_label;
} DmLinkTypeInfoRec, *DmLinkTypeInfoPtr;

/* 
 * This structure stores information needed to pop up a notice
 *
 */

typedef struct 
   {
   Widget     textarea;
   Widget     shell;
   char *     action_label;
   char *     cancel_label;
   Boolean     flag;
}  DmNoticeRec, *DmNoticePtr;

extern int DmCreatePromptBox(Widget parent,
			   DmPromptPtr	prompt,
			   DmPromptInfoPtr info,
			   XtPointer	action_cd,
			   XtPointer	cancel_cd,
			   DtAttrs	options
			  );

extern void DmDestroyPromptBox(DmPromptPtr prompt);

extern int DmCreateNotice(char	*name,
			Widget	parent,
			Widget	emanate,
			char	*text,
			DmNoticeRec	*notice,
			PFV	f,
			XtPointer	fp,
			PFV	v
		       );

extern Widget DmCreateInputPrompt(Widget parent, char   *label, char   *string);
extern Widget DmCreateStaticText(Widget parent, char   *label, char   *string);

extern void SetFlagCallback(Widget, XtPointer, XtPointer);
extern void UnsetFlagCallback(Widget, XtPointer, XtPointer);
extern void Dm__VerifyPromptCB(Widget, XtPointer, XtPointer);
extern Widget Dm__CreatePopupButtons(Widget parent, XtArgVal *list, int count);
extern Widget
Dm__CreateButtons( Widget parent, XtArgVal *items, int count, 
		   char **fields, int nfields, XtPointer client_data,
		   Boolean exclusive, Boolean noneset);
extern Widget DmCreateExclusives(
			Widget parent,
			char *label,
			XtArgVal *items,
			int count,
			XtPointer client_data,
			Boolean noneset);

extern Widget DmCreateButtons(Widget parent, XtArgVal *items, 
			      int count, XtPointer client_data);

#endif

#endif /* _dtutil_h */
