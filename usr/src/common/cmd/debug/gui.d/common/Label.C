#ident	"@(#)debugger:gui.d/common/Label.C	1.1"

#include "Label.h"
#include "UIutil.h"
#include "UI.h"
#include "gui_msg.h"
#include "gui_label.h"
#include <unistd.h>
#include <stdio.h>

// built by awk script
extern Label	labtable[];

LabelTab::LabelTab()
{
	labtab = labtable;
}

void
LabelTab::init()
{
	// determine whether we have a catalog available -
	// otherwise always use default strings
	char	*default_string = "a";
	char buf[sizeof(LCATALOG) + 10];

	sprintf(buf, "%s:%d", LCATALOG, 1);
	cat_available = (gettxt(buf, default_string) != default_string);
}

// Searches for localized message.  If one exists, caches it, else
// caches default.
// Returns cached message.
//
const char *
LabelTab::get_label(LabelId id)
{
	Label	*lab;
	char buf[sizeof(LCATALOG) + 10];
		// enough space for catalog:number
	if ((id < LAB_none) || (id > LAB_last))
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return "";
	}

	if (id == LAB_none)
		return "";

	lab = &labtab[id];

	if (!lab->local_set)
	{
		lab->local_set = 1;
		if (cat_available)
		{
			sprintf(buf, "%s:%d", LCATALOG, lab->catindex);
			lab->label = gettxt(buf, lab->label);
		}
	}
	return lab->label;
}

LabelTab	labeltab;
