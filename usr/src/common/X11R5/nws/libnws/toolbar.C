#ident	"@(#)toolbar.C	1.3"
/*----------------------------------------------------------------------------
 *
 */
#include <iostream.h>

#include <Xm/Form.h>
#include <Xm/Label.h>

#include "toolbar.h"

/*----------------------------------------------------------------------------
 *
 */
Toolbar::Toolbar (Widget			parent,
				  ToolbarType		type,
				  int				maxRowCol,
				  ToolbarPacking	packing)
{
	d_type = type;
	if (maxRowCol < 1) {
		d_maxRowCol = 1;
	}
	else {
		d_maxRowCol = maxRowCol;
	}
	d_packing = packing;

	d_helpData = 0;
	d_helpCallback = 0;

	d_numItems = 0;
	d_sizeItems = 0;
	d_items = 0;
	d_spacing = 2;

	d_emptyString = XmStringCreateSimple (" ");
	d_widget = 0;
	d_helpLabel = 0;

	if (parent) {
		if (d_widget = XmCreateForm (parent, "toolbar1", 0, 0)) {
			manage ();

			if (d_helpLabel = XmCreateLabel (d_widget, "helpLabel", 0, 0)) {
				if (d_emptyString) {
					XtVaSetValues (d_helpLabel,
								   XmNlabelString, d_emptyString,
								   0);
				}
				XtManageChild (d_helpLabel);
			}
		}
	}
}

Toolbar::~Toolbar ()
{
	if (d_items) {
		delete (d_items);
	}
	if (d_emptyString) {
		XmStringFree (d_emptyString);
	}
	if (d_widget) {
		XtDestroyWidget (d_widget);
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
Toolbar::insert (ToolbarItem* item, int index)
{
	if (item) {
		remove (item);
		item->helpCallback ((HelpCallback)updateHelpCallback, (XtPointer)this);
	}

	if (index < 0) {
		index = 0;
	}
	if (index >= d_numItems) {
		replace (item, index);
		return;
	}
	if (d_numItems >= d_sizeItems) {
		ToolbarItem**			tmp = d_items;

		d_sizeItems = d_numItems + 20;
		if (!(d_items = new ToolbarItem*[d_sizeItems])) {
			d_sizeItems = 0;
			d_numItems = 0;
			return;
		}
		int i;
		for (i = 0;i < d_numItems;i++) {
			d_items[i] = tmp[i];
		}
		delete (tmp);
		for (;i < d_sizeItems;i++) {
			d_items[i] = 0;
		}
	}
	for (int i = d_numItems;i > index;i--) {
		d_items[i] = d_items[i - 1];
	}
	d_numItems++;
	d_items[index] = item;
	if (item) {
		item->manage ();
	}
}

void
Toolbar::replace (ToolbarItem* item, int index)
{
	if (item) {
		remove (item);
		item->helpCallback ((HelpCallback)updateHelpCallback, (XtPointer)this);
	}

	if (index < 0) {
		index = 0;
	}
	if (index >= d_sizeItems) {
		ToolbarItem**			tmp = d_items;

		d_sizeItems = index + 20;
		if (!(d_items = new ToolbarItem*[d_sizeItems])) {
			d_sizeItems = 0;
			d_numItems = 0;
			return;
		}
		int i;
		for (i = 0;i < d_numItems;i++) {
			d_items[i] = tmp[i];
		}
		delete (tmp);
		for (;i < d_sizeItems;i++) {
			d_items[i] = 0;
		}
	}
	if (index >= d_numItems) {
		d_numItems = index + 1;
	}
	if (d_items[index]) {
		d_items[index]->unmanage ();
	}
	d_items[index] = item;
	if (item) {
		item->manage ();
	}
}

void
Toolbar::remove (ToolbarItem* item)
{
	Boolean						found = FALSE;

	if (!item || d_numItems <= 0) {
		return;
	}
	int i;
	for (i = 0;i < d_numItems;i++) {
		if (item == d_items[i]) {
			found = TRUE;
			break;
		}
	}
	for (;i < d_numItems - 1;i++) {
		d_items[i] = d_items[i + 1];
	}
	if (found) {
		d_numItems--;
		item->unmanage ();
	}
}

ToolbarItem*
Toolbar::remove (int index)
{
	ToolbarItem*				item;

	if (index < 0) {
		index = 0;
	}
	if (index >= d_numItems) {
		return (0);
	}
	item = d_items[index];
	for (int i = index;i < d_numItems - 1;i++) {
		d_items[i] = d_items[i + 1];
	}
	d_numItems--;
	if (item) {
		item->unmanage ();
	}
	return (item);
}

/*----------------------------------------------------------------------------
 *
 */
void
Toolbar::resize ()
{
	if (d_packing == PACKTIGHT) {
		resizeTight ();
	}
	else {
		resizeColumn ();
	}
}

void
Toolbar::resizeTight ()
{
	int		numRows;
	int		numCols;
	int*		heights;
	int*		widths;

	if (d_type == VERTICAL) {
		numCols = d_maxRowCol;
		numRows = (d_numItems / numCols) + 1;
	}
	else {
		numRows = d_maxRowCol;
		numCols = (d_numItems / numRows) + 1;
	}

	if (!(heights = new int[numRows])) {
		return;
	}
	if (!(widths = new int[numCols])) {
		delete (heights);
		return;
	}

	int index;
	for (index = 0;index < numCols;index++) {
		widths[index] = 0;
	}
	for (index = 0;index < numRows;index++) {
		heights[index] = 0;
	}

	int		row;
	int		col;
	int		w;
	int		h;
	int		x;
	int		y;

	if (d_type == VERTICAL) {
		index = 0;
		for (row = 0;row < numRows && index < d_numItems;row++) {
			for (col = 0;col < numCols && index < d_numItems;col++, index++) {
				if (d_items[index]) {
					if ((w = d_items[index]->width ()) > widths[col]) {
						widths[col] = w;
					}
					if ((h = d_items[index]->height ()) > heights[row]) {
						heights[row] = h;
					}
				}
			}
		}
		y = 0;
		index = 0;
		for (row = 0;row < numRows && index < d_numItems;row++) {
			x = 0;
			for (col = 0;col < numCols && index < d_numItems;col++, index++) {
				if (d_items[index]) {
					d_items[index]->position (x, y);
				}
				x += widths[col] + d_spacing;
			}
			y += heights[row] + d_spacing;
		}
	}
	else {
		index = 0;
		for (col = 0;col < numCols && index < d_numItems;col++) {
			for (row = 0;row < numRows && index < d_numItems;row++, index++) {
				if (d_items[index]) {
					if ((h = d_items[index]->height ()) > heights[row]) {
						heights[row] = h;
					}
					if ((w = d_items[index]->width ()) > widths[col]) {
						widths[col] = w;
					}
				}
			}
		}
		x = 0;
		index = 0;
		for (col = 0;col < numCols && index < d_numItems;col++) {
			y = 0;
			for (row = 0;row < numRows && index < d_numItems;row++, index++) {
				if (d_items[index]) {
					d_items[index]->position (x, y);
				}
				y += heights[row] + d_spacing;
			}
			x += widths[col] + d_spacing;
		}
	}

	y = 0;
	for (index = 0;index < numRows;index++) {
		y += heights[index] + d_spacing;
	}
	if (d_helpLabel) {
		XtVaSetValues (d_helpLabel,
					   XmNy, y,
					   XmNtopAttachment, XmATTACH_NONE,
					   0);
	}

	delete (widths);
	delete (heights);
}

void
Toolbar::resizeColumn ()
{
	int							maxWidth = 0;
	int							maxHeight = 0;
	int							width;
	int							height;

	for (int i = 0;i < d_numItems;i++) {
		if (d_items[i]) {
			if ((width = d_items[i]->width ()) > maxWidth) {
				maxWidth = width;
			}
			if ((height = d_items[i]->height ()) > maxHeight) {
				maxHeight = height;
			}
		}
	}

	int							row;
	int							col;
	int							x;
	int							y;

	if (d_type == VERTICAL) {
		y = 0;
		for (int i = 0, row = 0;i < d_numItems;row++) {
			x = 0;
			for (col = 0;col < d_maxRowCol && i < d_numItems;col++, i++) {
				d_items[i]->position (x, y);
				x += maxWidth + d_spacing;
			}
			y += maxHeight + d_spacing;
		}
	}
	else {
		x = 0;
		for (int i = 0, col = 0;i < d_numItems;col++) {
			y = 0;
			for (row = 0;row < d_maxRowCol && i < d_numItems;row++, i++) {
				d_items[i]->position (x, y);
				y += maxHeight + d_spacing;
			}
			x += maxWidth + d_spacing;
		}
	}

	if (d_helpLabel) {
		XtVaSetValues (d_helpLabel,
					   XmNy, row * (maxHeight + d_spacing),
					   XmNtopAttachment, XmATTACH_NONE,
					   0);
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
Toolbar::helpCallback (HelpCallback helpCallback, XtPointer data)
{
	d_helpCallback = helpCallback;
	d_helpData = data;
}

/*----------------------------------------------------------------------------
 *
 */
void
Toolbar::updateHelpCallback (XtPointer data, XmString helpString)
{
	Toolbar*					area = (Toolbar*)data;

	if (area) {
		area->updateHelp (helpString);
	}
}

void
Toolbar::updateHelp (XmString helpString)
{
	if (d_helpLabel) {
		if (helpString) {
			XtVaSetValues (d_helpLabel, XmNlabelString, helpString, 0);
		}
		else {
			if (d_emptyString) {
				XtVaSetValues (d_helpLabel, XmNlabelString, d_emptyString, 0);
			}
		}
	}
	if (d_helpCallback) {
		d_helpCallback (d_helpData, helpString);
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
Toolbar::helpOn ()
{
	if (d_helpLabel) {
		XtManageChild (d_helpLabel);
	}
}

void
Toolbar::helpOff ()
{
	if (d_helpLabel) {
		XtUnmanageChild (d_helpLabel);
	}
}

/*----------------------------------------------------------------------------
 *	Public Interface Methods
 */
void
Toolbar::manage ()
{
	if (d_widget) {
		XtManageChild (d_widget);
	}
}

void
Toolbar::unmanage ()
{
	if (d_widget) {
		XtUnmanageChild (d_widget);
	}
}

/*----------------------------------------------------------------------------
 *	Public Data Access Methods
 */
Widget
Toolbar::widget ()
{
	return (d_widget);
}

void
Toolbar::spacing (int spacing)
{
	if (spacing < 0) {
		d_spacing = 0;
	}
	else {
		d_spacing = spacing;
	}
}

void
Toolbar::maxRowCol (int maxRowCol)
{
	if (maxRowCol < 1) {
		d_maxRowCol = 1;
	}
	else {
		d_maxRowCol = maxRowCol;
	}
}

void
Toolbar::packing (ToolbarPacking packing)
{
	d_packing = packing;
}

