#ifndef _LABEL_H_

#ident	"@(#)debugger:gui.d/common/Label.h	1.2"

// provide internationalized label support for framework
// classes

#include "gui_label.h"

struct Label {
	short		catindex;
	unsigned char	local_set;
	const char	*label;
};

// table of labels
class LabelTab {
	Label		*labtab;
	int		cat_available;
public:
			LabelTab();
			~LabelTab() {}
	void		init();
	const char	*get_label(LabelId);
};

extern LabelTab		labeltab;
extern LabelId		program_label;

#define _LABEL_H_
#endif
