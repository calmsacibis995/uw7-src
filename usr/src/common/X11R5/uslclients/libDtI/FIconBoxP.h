#ifndef	NOIDENT
#ident	"@(#)libDtI:FIconBoxP.h	1.9"
#endif

#ifndef _OL_FICONBOXP_H
#define _OL_FICONBOXP_H

/*
 ************************************************************************
 * Description:
 *	This is the flattened IconBox widget's private header file.
 ************************************************************************
 */

#include <Xol/FGraphP.h>

#include <DnD/OlDnDVCX.h>

#include "FIconBox.h"

/*
 ************************************************************************
 * Define Widget Class Part and Class Rec
 ************************************************************************
 */

				/* Define new fields for the class part	*/

typedef struct {
    char no_class_fields;		/* Makes compiler happy */
} FlatIconBoxClassPart;

				/* Full class record declaration 	*/

typedef struct _FlatIconBoxClassRec {
    CoreClassPart		core_class;
    PrimitiveClassPart		primitive_class;
    FlatClassPart		flat_class;
    FlatGraphClassPart		graph_class;
    FlatIconBoxClassPart	iconBox_class;
} FlatIconBoxClassRec;

				/* External class record declaration	*/

extern FlatIconBoxClassRec		flatIconBoxClassRec;

/*
 ************************************************************************
 * Define Widget Instance Structure
 ************************************************************************
 */

				/* Define Expanded sub-object instance	*/

typedef struct {
	Boolean		set;		/* is this item selected?	*/
	Boolean		busy;		/* is this item busy?		*/
	XtPointer	object_data;	/* object data			*/
	XtPointer	client_data;	/* client data			*/
} FlatIconBoxItemPart;

			/* Item's Full Instance record declaration	*/

typedef struct {
	FlatItemPart		flat;
	FlatGraphItemPart	graph;
	FlatIconBoxItemPart	iconBox;
} FlatIconBoxItemRec, *FlatIconBoxItem;

			/* Define new fields for the instance part	*/

typedef struct {
	Boolean		movable;	/* movable icons		*/
	Boolean		exclusives;	/* exclusives 			*/
	Boolean		noneset;	/* no icons selected		*/
	Cardinal	select_count;	/* # of selected items		*/
	Cardinal	last_select;	/* idx of last selected items	*/
	XtPointer	trigger_proc;	/* trigger msg proc for DnD	*/
	XtCallbackProc	drop_proc;	/* drop routine			*/
	XtCallbackProc	select1_proc;	/* single select callback	*/
	XtCallbackProc	select2_proc;	/* double select callback	*/
	XtCallbackProc	adjust_proc;	/* adjust button callback	*/
	XtCallbackProc	menu_proc;	/* menu button callback		*/
	XtCallbackProc	draw_proc;	/* drawing  routine		*/
	XtCallbackProc	cursor_proc;	/* drag cursor callback		*/
	XtCallbackProc	post_select1_proc;/* post single select callback*/
	XtCallbackProc	post_adjust_proc;/* post adjust button callback	*/
	OlDnDDropSiteID	drop_site_id;	 /* dnd drop site id		*/
} FlatIconBoxPart;

			/* Full instance record declaration:
			 * 1. declare Widget "Part" Fields and then
			 * 2. declare Full flat Item Record		*/

typedef struct _FlatIconBoxRec {
    CorePart		core;
    PrimitivePart	primitive;
    FlatPart		flat;
    FlatGraphPart	graph;
    FlatIconBoxPart	iconBox;

    FlatIconBoxItemRec	default_item;
} FlatIconBoxRec;

#endif /* _OL_FICONBOXP_H */
