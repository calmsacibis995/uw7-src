#ifndef	NOIDENT
#ident	"@(#)flat:FColorsP.h	1.4"
#endif

#ifndef _OL_FCOLORSP_H
#define _OL_FCOLORSP_H

#include <Xol/FButtonsP.h>	/* superclass' header */
#include <Xol/FColors.h>	/* public header */

/*
 * Class structure:
 */

typedef struct _FlatColorsClassPart {
    char no_class_fields;			/* Makes compiler happy */
}			FlatColorsClassPart;

typedef struct _FlatColorsClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	FlatClassPart		flat_class;
	FlatRowColumnClassPart	row_column_class;
	FlatButtonsClassPart	exclusives_class;
	FlatColorsClassPart	colors_class;
}			FlatColorsClassRec;

extern FlatColorsClassRec flatColorsClassRec;

/*
 * Item structure:
 */

typedef struct _FlatColorsItemPart {
	int			unused;
}			FlatColorsItemPart;

typedef struct _FlatColorsItemRec {
	FlatItemPart		flat;
	FlatRowColumnItemPart	row_column;
	FlatButtonsItemPart	exclusives;
	FlatColorsItemPart	colors;
}			FlatColorsItemRec, *FlatColorsItem;

/*
 * Instance structure:
 */

typedef struct _FlatColorsPart {
	int			unused;
}			FlatColorsPart;

typedef struct _FlatColorsRec {
	CorePart		core;
	PrimitivePart		primitive;
	FlatPart		flat;
	FlatRowColumnPart	row_column;
	FlatButtonsPart		exclusives;
	FlatColorsPart		colors;

			/* Hide the default item in the widget instance */
	FlatColorsItemRec	default_item;
}			FlatColorsRec;

#endif /* _OL_FCOLORSP_H */
