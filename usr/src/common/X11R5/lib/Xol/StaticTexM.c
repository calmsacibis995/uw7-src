#ifndef NOIDENT
#ident	"@(#)statictext:StaticTexM.c	1.4"
#endif

/*
 *************************************************************************
 *
 * Description:	Static Text widget.  Motif GUI-specific code
 *		Most of code from HP widget set,
 *		selection and clipboard code by Andy Oakland, AT&T.
 *
 *******************************file*header*******************************
 */

/*************************************<+>*************************************
 *****************************************************************************
 **
 **		File:        StaticTextM.c
 **
 **		Project:     X Widgets
 **
 **		Description: Code/Definitions for StaticText widget class.
 **
 *****************************************************************************
 **   
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1988 by the Massachusetts Institute of Technology
 **   
 **   
 *****************************************************************************
 *************************************<+>*************************************/


#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/StaticText.h>

/*
 *  _OlmSTActivateWidget - stub for Motif mode.
 *
 */

Boolean
_OlmSTActivateWidget OLARGLIST((w, type, call_data))
	OLARG( Widget,		w)
	OLARG( OlVirtualName,	type)
	OLGRA( XtPointer,	call_data)
{
	   /* Do not consume event */
	return (False);
} /* end of _OlmSTActivateWidget */

/*
 *  _OlmSTHandleButton: this routine handles the ButtonPress and ButtonRelease
 *			events.  In Motif mode, all events are ignored.
 *
 */
void
_OlmSTHandleButton OLARGLIST((w, ve))
	OLARG(Widget,		w)
	OLGRA(OlVirtualEvent,	ve)
{
	/* Do nothing */
} /* end of _OlmSTHandleButton */

/*
 *  _OlmSTHandleMotion: this routine handles the Motion events.  All events
 * 					are ignored in Motif mode.
 */
void
_OlmSTHandleMotion OLARGLIST((w, ve))
	OLARG(Widget,		w)
	OLGRA(OlVirtualEvent,	ve)
{
	/* Do Nothing */
} /* end of _OlmSTHandleMotion */

/*************************************************************************
 *	_OlmSTHighlightSelection: highlight selected text in StaticText widget.
 *		Motif mode is a stub because Motif mode StaticText does not
 * 	support text selection
 *************************************************************************/
void
_OlmSTHighlightSelection OLARGLIST((w, forceit))
	OLARG(StaticTextWidget, w)
	OLGRA(Boolean, forceit)
{
	/* Do nothing */
}
