#ident	"@(#)toolbar.h	1.2"
/*----------------------------------------------------------------------------
 *
 */
#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <Xm/Xm.h>

#include "toolbarItem.h"

/*----------------------------------------------------------------------------
 *
 */
class Toolbar {
public:								// Public Data Types
	enum 						ToolbarType {
		VERTICAL,
		HORIZONTAL
	};
	enum 						ToolbarPacking {
		PACKTIGHT,
		PACKCOLUMN
	};

public:								// Constructors/Destructors
								Toolbar (Widget			parent,
										 ToolbarType	type = VERTICAL,
										 int			maxRowCol = 0,
										 ToolbarPacking	pack = PACKCOLUMN);
								~Toolbar ();

private:							// Private Data
	Widget						d_widget;
	Widget						d_helpLabel;
	XmString					d_emptyString;

	ToolbarItem**				d_items;
	int							d_numItems;
	int							d_sizeItems;

	ToolbarType					d_type;
	ToolbarPacking				d_packing;
	int							d_maxRowCol;
	int							d_spacing;

	XtPointer					d_helpData;
	HelpCallback				d_helpCallback;

private:							// Private Methods
	void						updateHelp (XmString str);
	void						resizeTight ();
	void						resizeColumn ();

public:								// Public Interface Methods
	void						replace (ToolbarItem* item, int index);
	void						insert (ToolbarItem* item, int index);
	void						remove (ToolbarItem* item);
	ToolbarItem*				remove (int index);
	void						resize ();
	static void					updateHelpCallback (XtPointer	data,
													XmString	helpString);
	void						helpCallback (HelpCallback	helpCallback,
											  XtPointer		data);

	void						helpOn ();
	void						helpOff ();

	void						manage ();
	void						unmanage ();

public:								// Public Data Access Methods
	Widget						widget ();

	void						spacing (int spacing);
	void						maxRowCol (int maxRowCol);
	void						packing (ToolbarPacking packing);
};

#endif	// TOOLBAR_H
