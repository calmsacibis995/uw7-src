#ident	"@(#)toolbarItem.C	1.2"
/*----------------------------------------------------------------------------
 *
 */
#include <iostream.h>

#include <Xm/PushB.h>

#include "toolbarItem.h"
#include "toolbar.h"
#include "action.h"


extern "C" {
	int	XReadPixmapFile (Display*, Drawable, Colormap, char*, unsigned int*, unsigned int*, unsigned int, Pixmap*, long);
};

/*----------------------------------------------------------------------------
 *
 */
ToolbarItem::ToolbarItem (Toolbar*			parent,
		  Action*			action,
		  const char*		name,
		  const XmString	labelString)
{
	initialize (parent, action, name);
	label (labelString);
	manage ();
}

ToolbarItem::ToolbarItem (Toolbar*		parent,
						  Action*		action,
						  const char*	name,
						  const char*	pixmapName)
{
	initialize (parent, action, name);
	createPixmap (pixmapName);
	manage ();
}

ToolbarItem::~ToolbarItem ()
{
	if (d_helpString) {
		XmStringFree (d_helpString);
	}
	if (d_widget) {
		XtDestroyWidget (d_widget);
	}
}

/*----------------------------------------------------------------------------
 *	Private Constructor Helper Methods
 */
void
ToolbarItem::initialize (Toolbar* parent, Action* action, const char* name)
{
	d_parent = parent;
	d_action = action;

	d_helpString = 0;
	d_helpCallback = 0;
	d_helpData = 0;
	d_width = 0;
	d_height = 0;
	d_widget = 0;
	d_display = 0;
	d_screen = 0;

	if (d_parent) {
		if (d_widget = XmCreatePushButton (d_parent->widget (),
										   (char*)name,
										   0,
										   0)) {
			if (d_action && d_action->callback ()) {
				XtAddCallback (d_widget,
							   XmNactivateCallback,
							   d_action->callback (),
							   d_action->data ());
			}

			XtAddEventHandler (d_widget,
							   EnterWindowMask | LeaveWindowMask,
							   False,
							   &helpCallback,
							   (XtPointer)this);

			d_display = XtDisplay (d_widget);
			d_screen = XtScreen (d_widget);
		}
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
ToolbarItem::helpCallback (HelpCallback helpCallback, XtPointer data)
{
	d_helpCallback = helpCallback;
	d_helpData = data;
}

/*----------------------------------------------------------------------------
 *
 */
void
ToolbarItem::helpCallback (Widget, XtPointer data, XEvent* event, Boolean*)
{
	ToolbarItem*				area = (ToolbarItem*)data;

	if (area && event) {
		area->help (event->type);
	}
}

void
ToolbarItem::help (int type)
{
	if (d_helpCallback) {
		if (type == EnterNotify) {
			if (d_helpString) {
				d_helpCallback (d_helpData, d_helpString);
			}
			else {
				if (d_action) {
					d_helpCallback (d_helpData, d_action->helpString ());
				}
			}
		}
		else {
			d_helpCallback (d_helpData, 0);
		}
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
ToolbarItem::helpString (const XmString str)
{
	if (d_helpString) {
		XmStringFree (d_helpString);
		d_helpString = 0;
	}
	if (str) {
		d_helpString = XmStringCopy (str);
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
ToolbarItem::position (int x, int y)
{
	if (d_widget) {
		XtVaSetValues (d_widget,
					   XmNx, x,
					   XmNy, y,
					   XmNleftAttachment, XmATTACH_NONE,
					   XmNtopAttachment, XmATTACH_NONE,
					   0);
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
ToolbarItem::label (const XmString labelString)
{
	if (d_widget && labelString) {
		XtVaSetValues (d_widget,
					   XmNlabelString, labelString,
					   XmNlabelType, XmSTRING,
					   0);
		XtVaGetValues (d_widget, XmNwidth, &d_width, XmNheight, &d_height, 0);
	}
}

void
ToolbarItem::pixmap (const char* pixmapName)
{
	createPixmap (pixmapName);
}

/*----------------------------------------------------------------------------
 *	Public Interface Methods
 */
void
ToolbarItem::manage ()
{
	if (d_widget) {
		XtManageChild (d_widget);
	}
}

void
ToolbarItem::unmanage ()
{
	if (d_widget) {
		XtUnmanageChild (d_widget);
	}
}

void
ToolbarItem::activate ()
{
	if (d_widget) {
		XtVaSetValues (d_widget, XmNsensitive, True, 0);
	}
}

void
ToolbarItem::deactivate ()
{
	if (d_widget) {
		XtVaSetValues (d_widget, XmNsensitive, False, 0);
	}
}

/*----------------------------------------------------------------------------
 *	Public Data Access Methods
 */
Widget
ToolbarItem::widget ()
{
	return (d_widget);
}

Dimension
ToolbarItem::width ()
{
	return (d_width);
}

Dimension
ToolbarItem::height ()
{
	return (d_height);
}

/*----------------------------------------------------------------------------
 *
 */
void
ToolbarItem::createPixmap (const char* pixmapName)
{
	Pixmap						pixmap = 0;
	Pixmap						sensitivePixmap = 0;
	Pixmap						tempPixmap = 0;
	Pixel						pixel;
	XGCValues 					values;
	GC							gc = 0;
	unsigned int				width;
	unsigned int				height;
	long						background = 0;

	if (!d_widget || !d_display || !d_screen || !pixmapName) {
		return;
	}

	XtVaGetValues (d_widget,
				   XmNbackground, &background,
				   0);

	XReadPixmapFile (d_display,
					 RootWindowOfScreen (d_screen),
					 DefaultColormapOfScreen (d_screen),
					 (char*)pixmapName,
					 &width,
					 &height,
					 DefaultDepthOfScreen (d_screen),
					 &tempPixmap,
					 background);

	if (!tempPixmap) {
		return;
	}

	if (!(sensitivePixmap = XCreatePixmap (d_display,
					   RootWindowOfScreen (d_screen),
					   width,
					   height, 
					   DefaultDepthOfScreen (d_screen)))) {
		return;
	}

	XtVaGetValues (d_widget,
				   XmNbackground, &pixel,
				   0);
	values.foreground = pixel;
	values.stipple = XmGetPixmapByDepth (d_screen, "50_foreground", 1, 0, 1);

	if (!(gc = XCreateGC (d_display,
						  sensitivePixmap,
						  GCStipple | GCForeground,
						  &values))) {
		return;
	}

	XCopyArea (d_display,
			   tempPixmap,
			   sensitivePixmap,
			   gc,
			   0,	
			   0,
			   width,
			   height,
			   0,
			   0);

	if (!(pixmap = XCreatePixmap (d_display,
								  RootWindowOfScreen (d_screen),
								  width,
								  height,
								  DefaultDepthOfScreen (d_screen)))) {
		XFreeGC (d_display, gc);
		return;
	}

	XSetFillStyle (d_display, gc, FillSolid);
	XFillRectangle (d_display, pixmap, gc, 0, 0, width, height);
	XFillRectangle (d_display, sensitivePixmap, gc, 0, 0, width, height);
/*tony
	XSetClipMask (d_display, gc, glyph.mask);
*/
	XSetClipOrigin (d_display, gc, 0, 0);
	XCopyArea (d_display, tempPixmap, pixmap, gc, 0, 0, width, height, 0, 0); 
	XSetFillStyle (d_display, gc, FillStippled);

	XCopyArea (d_display,
			   tempPixmap,
			   sensitivePixmap,
			   gc,
			   0,
			   0,
			   width,
			   height,
			   0,
			   0); 

	XFillRectangle (d_display, sensitivePixmap, gc, 0, 0, width, height);

	XtVaSetValues (d_widget, 
				   XmNlabelType, XmPIXMAP,
				   XmNlabelPixmap, pixmap,
				   XmNlabelInsensitivePixmap, sensitivePixmap,
				   XmNmarginWidth, 0,
				   XmNshadowThickness, 1,
				   XmNwidth, width + 6,
				   0);	

	XtVaGetValues (d_widget,
				   XmNwidth, &d_width,
				   XmNheight, &d_height,
				   0);

	XFreeGC (d_display, gc);
}

