#ident	"@(#)controlArea.h	1.2"
#ifndef	CONTROL_AREA_H
#define	CONTROL_AREA_H

/*
//  This file contains the structures and function declarations needed
//  for the control area of the window.
*/


#include	"setupAPIs.h"	//  for setupObj_t definition
#include	"setupWin.h"	//  for SetupWin   definition

typedef struct _buttonItem
{
	Widget	widget;		//  widget id of the button
	setupChoice_t *choice;	//  choice for the button
	char	*label;		//  label for the button
	char	*mnemonic;	//  mnemonic for the button
	setupObject_t *curObj;	//  the current setup object we're working with
	SetupWin *win;		//  the current setup window we're working with
} ButtonItem;


extern Widget	createControlArea (Widget parent, SetupWin *win);
extern Widget	createMainWindowArea (Widget parent, SetupWin *win);
extern Widget	createVariableList (Widget parent, setupObject_t *varObjs,
							    SetupWin *win);
extern void	setVariableFocus (setupObject_t *curObj);
extern void	setOptionMenuValue (Widget optionMenu, setupObject_t *curObj);
extern void	setVariableFocus (setupObject_t *obj);
extern int	strToWideCaseCmp (const char *s1, const char *s2);

#endif	CONTROL_AREA_H
