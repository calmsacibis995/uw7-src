#ifndef	NOIDENT
#ident	"@(#)nonexclus:Nonexclusi.h	1.10"
#endif

#ifndef _OlNonexclusives_h
#define _OlNonexclusives_h

/*
 * Author:	Karen S. Kendler
 * Date:	16 August 1988
 * File:	Nonexclusives.h - Public definitions for Nonexclusives widget
 *	Copyright (c) 1989 AT&T		
 *
 */

#include <Xol/Manager.h>	/* include superclasses' header */

extern WidgetClass     nonexclusivesWidgetClass;

typedef struct _NonexclusivesClassRec   *NonexclusivesWidgetClass;
typedef struct _NonexclusivesRec        *NonexclusivesWidget;

#endif /*  _OlNonexclusives_h  */

/* DON'T ADD STUFF AFTER THIS */
