#ifndef	NOIDENT
#ident	"@(#)exclusives:Exclusives.h	1.9"
#endif

#ifndef _OlExclusives_h
#define _OlExclusives_h

/*
 * Author:	Karen S. Kendler
 * Date:	16 August 1988
 * File:	Exclusives.h - Public definitions for Exclusives widget
 *
 *	Copyright (c) 1989 AT&T
 */

#include <Xol/Manager.h>	/* include superclasses' header */

extern WidgetClass     exclusivesWidgetClass;

typedef struct _ExclusivesClassRec   *ExclusivesWidgetClass;
typedef struct _ExclusivesRec        *ExclusivesWidget;

#endif /*  _OlExclusives_h  */

/* DON'T ADD STUFF AFTER THIS */
