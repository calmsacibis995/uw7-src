#ifndef	NOIDENT
#ident	"@(#)menu:MenuP.h	1.25"
#endif

#ifndef _Ol_MenuP_h
#define _Ol_MenuP_h

/*
 ************************************************************************
 *
 * Description:
 *		Private ".h" file for the Menu Widget
 *
 *****************************file*header********************************
 */

#include <X11/ShellP.h>
#include <Xol/Menu.h>

		/* New fields for the Menu widget class record	*/

typedef struct _MenuClass {
    char no_class_fields;		/* Makes compiler happy */
} MenuShellClassPart;

				/* Full class record declaration 	*/

typedef struct _MenuClassRec {
    CoreClassPart		core_class;
    CompositeClassPart		composite_class;
    ShellClassPart		shell_class;
    WMShellClassPart		wm_shell_class;
    VendorShellClassPart	vendor_shell_class;
    TransientShellClassPart	transient_shell_class;
    MenuShellClassPart	        menu_shell_class;
} MenuShellClassRec;

extern MenuShellClassRec menuShellClassRec;

/***************************************
 *
 *  Instance (widget) structure 
 *
 **************************************/

typedef void (*OlRevertButtonProc) OL_ARGS((MenuShellWidget, Boolean));


			/* New fields for the Menu widget record */
typedef struct {
					/* New Resources		*/

    OlRevertButtonProc	revert_button;	/* unhighlight procedure for
					 * MenuButtons			*/
    ShellBehavior	shell_behavior;	/* Current state of the Menu	*/
    Boolean		augment_parent;	/* Does menu augment parent's
					   event handler list ??	*/
    Boolean		attach_query;	/* prevent Menus from being
					   attached to this shell menu	*/
    Boolean		application_menu;/* is this an appliation's menu*/
    Boolean		prevent_stayup;	/* Stay-up mode not allowed	*/
    Boolean		pushpin_default;/* is the pushpin the default	*/
    Widget		title_widget;	/* id of the title widget	*/
    Widget		pushpin;	/* Pushpin widget id		*/
    Widget		parent_cache;	/* Cache of parent widget't id	*/
    CompositeWidget	pane;		/* Pane to take on child Widgets*/
    MenuShellWidget	root;		/* Root of Menu Cascade		*/
    MenuShellWidget	next;		/* Next menu in cascade		*/
    Widget		emanate;	/* Widget from which menu emanates */
    GC			shadow_GC;	/* Drop Shadow's GC		*/
    GC			backdrop_GC;	/* GC for underneath copy	*/
    Pixmap		backdrop_right;	/* Pixmap under Menu		*/
    Pixmap		backdrop_bottom;/* Pixmap under Menu		*/
    Pixel		foreground;	/* Foreground for the menu	*/
    Position		post_x;		/* Pointer x coor. at posting	*/
    Position		post_y;		/* Pointer y coor. at posting	*/
    Dimension		shadow_right;	/* Pixel width of right shadow	*/
    Dimension		shadow_bottom;	/* Pixel height of bottom shadow*/
} MenuShellPart;

			/*
			 * Widget Instance declaration
			 */

typedef struct _MenuShellRec {
    CorePart		core;
    CompositePart	composite;	
    ShellPart		shell;
    WMShellPart		wm;
    VendorShellPart	vendor;
    TransientShellPart	transient;
    MenuShellPart	menu;
} MenuShellRec;

/*
 * function prototype section
 */

OLBeginFunctionPrototypeBlock

	/* Check to see if position is within a "click" threshold,
	 * specified in points						*/
extern Boolean
_OlIsMenuMouseClick OL_ARGS((
	MenuShellWidget,	/* menu;- menu in question	*/
	int,			/* x;	- x coordinate		*/
	int			/* y;	- y coordinate		*/
));

	/* Propagate a new menu state down a menu cascade	*/
extern void
_OlPropagateMenuState OL_ARGS((
	MenuShellWidget,	/* menu;- start point in cascade	*/
	ShellBehavior		/* sb;	- new state			*/
));

OLEndFunctionPrototypeBlock

#endif /* _Ol_MenuP_h */
