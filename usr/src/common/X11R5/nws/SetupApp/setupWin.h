#ident	"@(#)setupWin.h	1.3"
#ifndef SETUPWIN_H
#define SETUPWIN_H

/*
//  This file contains the structures needed for each window of the application.
//  The application may have one or more windows associated with it.  
*/


#include	<stdio.h>		//  for FILE definition
#include	<Xm/Xm.h>

#include	"dtFuncs.h"		//  for HelpText definition
#include	"actionArea.h"		//  for ActionAreaItem definition
#include	"setupAPIs.h"		//  for setupObject_t definition
#include	"treeBrowse.h"		//  for the treeBrowse object


#define		HEIGHT_OFFSET	5
#define		WIDTH_OFFSET	31



/*  SetupWin-specific variables.  These variables are needed for each SetupWin.*/

typedef struct _setupWin
{
	Widget		topLevel;	//  widget id of this top level shell
	Window		window;		//  window id of this top level window
	Widget		highLevelWid;	//  highest level Motif widget id (form)
	Boolean		isPrimary;	//  is this window Primary?
	Widget		varList;	//  widget id of the variable list
	setupObject_t	*firstObj;	//  the first var obj (for init focus)
	char 		*widestLabel;	//  the widest (in pixels) var label
	Widget		varWin;		//  widget id of the var list scroll win
	Dimension	varListWidth;	//  min width that var list should be
	int		maxLabelWidth;	//  width in pixels of widest var label
	Widget		descArea;	//  widget id of scrolled text desc area
	Widget		actionArea;	//  needed for setting actionArea height
	Pixmap		iconPixmap;	//  Pixmap to be used for iconifying
	setupObject_t	*object;	//  ptr to the setup object
	setupObject_t	*objList;	//  ptr to the setup object list
	char		*title;		//  ptr to the localized app title
	char		*iconFile;	//  ptr to the name of the icon file
	char		*iconTitle;	//  ptr to the localized icon title
	HelpText	*help;		//  ptr to the Help information
	treeBrowse	*treeBrowse;	//  the treeBrowse object
	int		numOpts;	//  # of categories for option menu
	Widget		*widList;	//  widget list of Category menu items
	void*	mPtr;		//  ptr to the button mnemonic record
} SetupWin;



#include	"cDebug.h"		//  for SetupApp debugging definitions
					//  (needs to be after AppStruct def.)

typedef struct _flagVar
{
	Widget		onBtn;		//  Widget id of the "On" button
	Widget		offBtn;		//  Widget id of the "Off" button
} FlagVar;



typedef struct _menuVar
{
	Widget		origChoice;	//  original choice for the option menu
} MenuVar;



typedef struct _pwdVar
{
	char		*new1stText;	//  First password text
	char		*new2ndText;	//  Second (verification) password text
	void*	mPtr;		//  Ptr to the passwd dialog mnem info rec
} PwdVar;



typedef struct _listVar
{
	char		*selectedItem;	//  item that is selected in list
} ListVar;



typedef union _varData
{
	struct		FlagVar;
	struct		MenuVar;
	struct		PwdVar;
	struct		ListVar;
} VarData;



typedef struct _objData
{
	SetupWin	*win;
} ObjData;



typedef struct _varEntry		//  Info for per variable client data
{
	SetupWin	*win;		//  pointer to the current setup window
	Widget		var;		//  widget id of the variable
	Widget		label;		//  widget id of the variable label
	setupObject_t	*popupButton;	//  list ptr, and used as Boolean
	union
	{		//  information needed for the specific variable types
		FlagVar		flgVar;
		MenuVar		menuVar;
		PwdVar		pwdVar;
		ListVar		listVar;
	} varData;

} VarEntry;




/*  Some shortcuts ... (I couldn't stand it anymore!)			     */
#define	f_offBtn	varData.flgVar.offBtn
#define	f_onBtn		varData.flgVar.onBtn
#define	m_origChoice	varData.menuVar.origChoice
#define	p_1stText	varData.pwdVar.new1stText
#define	p_2ndText	varData.pwdVar.new2ndText
#define	p_mPtr		varData.pwdVar.mPtr
#define	l_selectedItem	varData.listVar.selectedItem


/* 
//		Functions that may be used by others
*/

extern SetupWin	*getNewSetupWin (void);
extern void	createSetupWindow (SetupWin *win);
extern void	createPrimaryWindow (Widget parent, SetupWin *win);
extern void	createSecondaryWindow (Widget parent, SetupWin *win);
extern void	resizeCB (Widget w, SetupWin *clientData, XEvent *xev);
extern void	turnOffSashTraversal (Widget pane);

#endif	//  SETUPWIN_H
