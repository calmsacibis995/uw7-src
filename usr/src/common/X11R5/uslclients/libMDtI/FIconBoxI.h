#ifndef EXM_FICONBOXI_H_
#define EXM_FICONBOXI_H_

#ifdef NOIDENT
#pragma ident	"@(#)libMDtI:FIconBoxI.h	1.3"
#endif

#include "FIconBoxP.h"

extern void ExmFIconDrawProc(Widget, ExmFlatItem, ExmFlatDrawInfo *);
extern void ExmFIconDrawIcon(Widget, DmGlyphPtr, Drawable, GC, Boolean,
			     Pixmap, int, int);

#endif /* EXM_FICONBOXI_H_ */
