#ifndef	NOIDENT
#ident	"@(#)exclusives:ExclusiveP.h	1.20"
#endif

/* 
 * File:	ExclusivesP.h - Private definitions for Exclusives widget
 *
 *	Copyright (c) 1989 AT&T
 */

#ifndef _OlExclusivesP_h
#define _OlExclusivesP_h
 
#include <Xol/ManagerP.h>	/* include superclasses' header */
#include <Xol/Exclusives.h>

/* New fields for the Exclusives widget class record */

typedef struct _ExclusivesClass {
    char no_class_fields;		/* Makes compiler happy */
} ExclusivesClassPart;

   /* Full class record declaration */

typedef struct _ExclusivesClassRec {
    CoreClassPart	core_class;
    CompositeClassPart	composite_class;
    ConstraintClassPart	constraint_class;
    ManagerClassPart	manager_class;
    ExclusivesClassPart	exclusives_class;
} ExclusivesClassRec;

extern ExclusivesClassRec exclusivesClassRec;

		    /* New fields for the Exclusives widget record: */
typedef struct {

		   				/* fields for resources */
    OlDefine		layout;			/* public */
    int			measure;
    Boolean		noneset;
    Boolean		recompute_size;

    Boolean		is_default;		/* fields for resources */
    XtPointer		default_data;		/* private */
    Widget		preview;	
    Boolean		trigger;	
    int			shell_behavior;	
    int			reset_set;
    int			reset_default;
    XtCallbackList	postselect;
					/* fields for internal management */ 
    Widget		default_child;
    Widget		set_child;
    Widget		looks_set;
    int			usr_set;
    Dimension		max_height;
    Dimension		max_width;
    Dimension		normal_height;
    Dimension		normal_width;

} ExclusivesPart;

   /* Full widget declaration */

typedef struct _ExclusivesRec {
    CorePart		core;
    CompositePart	composite;
    ConstraintPart	constraint;
    ManagerPart		manager;
    ExclusivesPart	exclusives;
} ExclusivesRec;

#endif /* _OlExclusivesP_h */
