/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)Dt:Property.h	1.4.1.1"
#endif

#ifndef __Property_h
#define __Property_h

/* attributes */
/* system reserved bits */
#define DT_PROP_ATTR_SYS	(0xff)
#define DT_PROP_ATTR_MENU	(1 << 0)
#define DT_PROP_ATTR_DONTCOPY	(1 << 1)
#define DT_PROP_ATTR_INSTANCE	(1 << 2)
#define DT_PROP_ATTR_LOCKED	(1 << 3)
#define DT_PROP_ATTR_DONTCHANGE	(1 << 4)
#define DT_PROP_ATTR_TRANSLATED	(1 << 5)
#define DT_PROP_ATTR_BACKUP	(1 << 6)

typedef struct {
	DtAttrs		attrs;		/* attributes */
	char 		*name;		/* name */
	char		*value;		/* value */
} DtPropRec, *DtPropPtr;

typedef struct {
	DtPropPtr	ptr;		/* ptr to list */
	unsigned int	count;		/* # of entries in list */
} DtPropList, *DtPropListPtr;

#endif /* __Property_h */
