#ifndef	NOIDENT
#ident	"@(#)olhelp:MagP.h	1.7"
#endif

#ifndef _MagP_h_
#define _MagP_h_

/*
 *************************************************************************
 *
 * Date:	March 1989
 *
 * Description:
 *		Private header file for the Magnifier widget
 *
 *******************************file*header*******************************
 */

#include <Xol/PrimitiveP.h>	/* include superclasses's header */
#include <Xol/Mag.h>

/*
 ***********************************************************************
 *
 * Widget Private Data
 *
 ***********************************************************************
 */

			/* New fields for the widget class record	*/

typedef struct {
    char no_class_fields;		/* Makes compiler happy */
} MagClassPart;

				/* Full class record declaration 	*/

typedef struct _MagClassRec {
    CoreClassPart	core_class;
    PrimitiveClassPart	primitive_class;
    MagClassPart	mag_class;
} MagClassRec;

extern MagClassRec magClassRec;

/*
 ***********************************************************************
 *
 * Instance (widget) structure 
 *
 ***********************************************************************
 */

				/* New fields for the widget record	*/

typedef struct {
	Position	mouseX;		/* X Coordinate of mouse	*/
	Position	mouseY;		/* Y Coordinate of mouse	*/
	GC		copy_GC;	/* used to copy RootWindow	*/
	GC		hold_GC;	/* used to copy cached image	*/
	GC		mag_GC;		/* magnifier GC			*/
	Pixmap		mag_pixmap;	/* magnifier pixmap		*/
	Pixmap		hold_pixmap;	/* cached image pixmap		*/
	Pixmap		pixmap;		/* application-supplied pixmap	*/
} MagPart;

					/* Full Widget declaration	*/

typedef struct _MagRec {
    CorePart		core;
    PrimitivePart	primitive;
    MagPart		mag;
} MagRec;

#endif /* _MagP_h_ */
