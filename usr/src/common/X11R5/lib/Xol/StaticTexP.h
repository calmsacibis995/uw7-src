#ifndef	NOIDENT
#ident	"@(#)statictext:StaticTexP.h	1.7.2.11"
#endif

/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        StaticTexP.h    
 **
 **   Project:     X Widgets
 **
 **   Description: Private include file for StaticText class
 **
 *****************************************************************************
 **   
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1988 by the Massachusetts Institute of Technology
 **   
 *****************************************************************************
 *************************************<+>*************************************/
#ifndef _StaticTexP_h
#define _StaticTexP_h

#include <Xol/PrimitiveP.h>	/* include superclasses's header */
#include <Xol/StaticText.h>
#include <DnD/OlDnDVCX.h>	/* for Drag and Drop */

/********************************************
 *
 *   No new fields need to be defined
 *   for the StaticText widget class record
 *
 ********************************************/
typedef struct 
{
    char no_class_fields;               /* Makes compiler happy */
} StaticTextClassPart;

/****************************************************
 *
 * Full class record declaration for StaticText class
 *
 ****************************************************/
typedef struct _StaticTextClassRec {
	CoreClassPart      	core_class;
	PrimitiveClassPart	primitive_class;
	StaticTextClassPart	statictext_class;
} StaticTextClassRec;

/********************************************
 *
 * New fields needed for instance record
 *
 ********************************************/
typedef struct _StaticTextPart {
	/*
	 * "Public" members (Can be set by resource manager).
	 */
	char       	*input_string;	/* String sent to this widget. */
	OlDefine	alignment;	/* Alignment within the box */
	Boolean    	wrap;		/* Controls wrapping on spaces */
	Boolean		strip;		/* Controls stripping of blanks */
	int	       	gravity;	/* Controls extra space in window */
	int     	line_space;	/* Ratio of font height as dead space
					   between lines.  Can be less than zero
					   but not less than -1.0  */
	Dimension 	internal_height; /* Space from text to top and bottom highlights */
	Dimension 	internal_width; /* Space from left and right side highlights */
	Boolean		recompute_size;

/*
** Fields taken from XwPrimitive
*/

	Dimension	highlight_thickness;

/*
 * "Private" fields, used by internal widget code.
 */

	GC         	normal_GC; 	/* GC for text			*/
	GC         	hilite_GC; 	/* GC for highlighted text	*/
	GC         	cursor_GC; 	/* GC for x-or'ed cursor	*/
	XRectangle 	TextRect; 	/* The bounding box of the text, or clip rectangle of the window; whichever is smaller. */
	char       	*output_string; /* input_string after formatting*/
	char		*selection_start;	/* ptr to start of selection*/
	char		*selection_end;	/* ptr to end of selection	*/
	char		*oldsel_start;	/* old start of selection	*/
	char		*oldsel_end;	/* old end of selection		*/
	int		selection_mode;	/* chars, words, lines...	*/
	Time            time;           /* time of last key or button action*/ 
	Position        ev_x, ev_y;     /* coords for key or button action*/
	Position        old_x, old_y;   /* previous key or button coords*/
	char		**line_table;	/* ptrs to start of each line	*/
	int		*line_len;	/* length of stripped line	*/
	int		line_count;	/* count of line_table		*/
	char		*clip_contents;	/* contents of clipboard	*/
	Atom		transient;	/* transient atom for DragNDrop */
	Cursor 		dragCursor;	/* drag cursor for DragNDrop */
	Boolean		is_dragging;	/* Drag operation is in progress*/
} StaticTextPart;

/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/
typedef struct _StaticTextRec {
	CorePart      	core;
	PrimitivePart	primitive;
	StaticTextPart	static_text;
} StaticTextRec;

extern Dimension _OlSTGetOffset OL_ARGS((StaticTextWidget, int));
extern void      _OlSTDisplaySubstring OL_ARGS((StaticTextWidget, char *, char *, GC));
extern int      _OlSTXYForPosition OL_ARGS((StaticTextWidget, char *, Position *, Position *));
extern void      _OlmSTHighlightSelection OL_ARGS((StaticTextWidget, Boolean));
extern Boolean   _OlmSTActivateWidget OL_ARGS((Widget, OlVirtualName, XtPointer));
extern void      _OlmSTHandleButton OL_ARGS((Widget, OlVirtualEvent));
extern void      _OlmSTHandleMotion OL_ARGS((Widget, OlVirtualEvent));

extern void      _OloSTHighlightSelection OL_ARGS((StaticTextWidget, Boolean));
extern Boolean   _OloSTActivateWidget OL_ARGS((Widget, OlVirtualName, XtPointer));
extern void      _OloSTHandleButton OL_ARGS((Widget, OlVirtualEvent));
extern void      _OloSTHandleMotion OL_ARGS((Widget, OlVirtualEvent));




#endif
