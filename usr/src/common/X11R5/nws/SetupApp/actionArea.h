#ident	"@(#)actionArea.h	1.3"
#ifndef	ACTION_AREA_H
#define	ACTION_AREA_H

/*
//  This file contains the structures and constants needed for the action area
//  (bottom-row buttons on a secondary window).
*/

#include	<Xm/Xm.h>		//  ... always needed

#define	TIGHTNESS	20


/*  Values to be used in the "which" element of the ActionAreaItem	     */
#define	OK_BUTTON	1
#define	APPLY_BUTTON	2
#define	RESET_BUTTON	3
#define	CANCEL_BUTTON	4
#define	HELP_BUTTON	5


typedef struct _action_area_item
{
	char		*label;		//  label of the button
	char		*mnemonic;	//  mnemonic of the button
	int		which;		//  which button (OK, Apply, Help, etc)
	Boolean		sensitive;	//  sensitivity of action area button
	void   (*callback)(Widget, XtPointer clientData, XmAnyCallbackStruct *);
					//  pointer to callback routine
	void		*clientData;	//  client data for callback routine
} ActionAreaItem;


extern Widget createActionArea (Widget parent, ActionAreaItem *actions,
		  Cardinal numActions, Widget highLevelWid, void* mPtr);
extern void	setActionAreaHeight (Widget actionArea);
extern void	setButtonSensitivity (ActionAreaItem *buttons, int thisButton,
							Boolean sensitivity);

#endif	//  ACTION_AREA_H
