#pragma ident	"@(#)m1.2libs:Mrm/Mrmwcrw.c	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.2
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif

/*
*  (c) Copyright 1989, 1990, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */

/*
 *++
 *  FACILITY:
 *
 *      UIL Resource Manager (URM):
 *
 *  ABSTRACT:
 *
 *	This module contains the routine which implement widget creation
 *	and management at runtime from a widget stored in a resource context.
 *
 *--
 */


/*
 *
 *  INCLUDE FILES
 *
 */
#include <stdio.h>
#include <Mrm/MrmAppl.h>
#include "Mrm.h"
#include <X11/keysym.h>
#include <Xm/XmosP.h>		/* For ALLOCATE/DEALLOCATE_LOCAL */
#include <Xm/RowColumn.h>	/* For XmGetTearOffControl */
#include <Xm/DisplayP.h>	/* For XmDisplay */
/*
 *
 *  TABLE OF CONTENTS
 *
 *	UrmCreateWidgetTree		Create a widget and its subtree
 *
 *	UrmCreateWidgetInstance		Create a widget instance
 *
 *	Urm__CW_CreateArglist		Create a widget arglist
 *
 *	Urm__CW_FixupCallback		Complete a callback item
 *
 *	Urm__CW_EvaluateResource	Evaluate a resource ref value
 *
 */

#ifdef _NO_PROTO
static void DisplayDestroyCallback ();

#else	/* _NO_PROTO */

static void DisplayDestroyCallback ( 
			Widget w, 
			XtPointer client_data, 
			XtPointer call_data );
#endif	/* _NO_PROTO */

/*
 *
 *  DEFINE and MACRO DEFINITIONS
 *
 */



/*
 *
 *  EXTERNAL VARIABLE DECLARATIONS
 *
 */


/*
 *
 *  GLOBAL VARIABLE DECLARATIONS
 *
 */


/*
 *
 *  OWN VARIABLE DECLARATIONS
 *
 */



Cardinal UrmCreateWidgetTree
#ifndef _NO_PROTO
    (URMResourceContextPtr	context_id,
    Widget			parent,
    MrmHierarchy		hierarchy_id,
    IDBFile			file_id,
    String			ov_name,
    ArgList			ov_args,
    Cardinal			ov_num_args,
    MrmCode			keytype,
    String			kindex,
    MrmResource_id		krid,
    MrmFlag			manage,
    URMPointerListPtr		*svlist,
    URMResourceContextPtr	wref_id,
    Widget			*w_return)
#else
        (context_id, parent, hierarchy_id, file_id, ov_name, ov_args,
        ov_num_args, keytype, kindex, krid, manage, svlist, wref_id, w_return)
    URMResourceContextPtr	context_id ;
    Widget			parent ;
    MrmHierarchy		hierarchy_id ;
    IDBFile			file_id ;
    String			ov_name ;
    ArgList			ov_args ;
    Cardinal			ov_num_args ;
    MrmCode			keytype ;
    String			kindex ;
    MrmResource_id		krid ;
    MrmFlag			manage ;
    URMPointerListPtr		*svlist ;
    URMResourceContextPtr	wref_id ;
    Widget			*w_return ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmCreateWidgetTree is the recursive routine
 *	which recurses down a widget subtree and instantiates all widgets
 *	in the tree. The recursion process is:
 *
 *		o Create this widget.
 *		o Create a new context. Read each child of this widget
 *		  into the context in succession. Create each child,
 *		  saving its id.
 *		o manage the children
 *
 *	This routine accepts override parameters for the widget name, and
 *	to override arguments in the creation arglist. The latter are appended
 *	to the list created from the UID file, and do not replace all values.
 *	The parameters are not passed down to any children in the subtree.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	context containing widget record describing widget
 *			to create
 *	parent		id of parent widget
 *	hierarchy_id	URM hierarchy from which to read public resources
 *	file_id		URM file from which to read private resources
 *	ov_name		Name to override widget name (NULL for no override)
 *	ov_args		Override arglist, exactly as would be given to
 *			XtCreateWidget (conversion complete, etc). NULL
 *			for no override.
 *	ov_num_args	# args in ov_args; 0 for no override
 *	keytype		type of key which accessed this widget
 *	kindex		index for URMrIndex access
 *	krid		resource id for URMrRID access
 *	svlist		list of SetValues descriptors for widgets in tree
 *	wref_id		to accumulate widget reference definitions
 *	w_return	To return id of newly created widget
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmBAD_CONTEXT	invalid context
 *	MrmBAD_WIDGET_REC	invalid widget record
 *
 *  SIDE EFFECTS:
 *
 *--
 */

{

/*
 *  External Functions
 */

/*
 *  Local variables
 */
Cardinal		result ;	/* function results */
Widget			widget_id ;	/* this widget id */
URMResourceContextPtr	child_ctx ;	/* context for children */
Widget			child_id ;	/* current child */
IDBFile			loc_file_id ;	/* local file id, may be modified */
URMPointerListPtr	manage_ids ;	/* children to be managed */
RGMWidgetRecordPtr	widgetrec ;	/* the widget record in the context */
int			ndx ;		/* loop index */
RGMChildrenDescPtr	childrendesc ;	/* children list descriptor */
RGMChildDescPtr		childptr ;	/* current child */
String			child_idx = NULL ;	/* current child index */
Boolean			one_parent ;	/* to check for only one parent */
Widget			check_parent ;	/* to check for identical parent */
char			err_msg[300] ;


/*
 * Create the widget instance.
 */
result = UrmCreateOrSetWidgetInstance
    (context_id, parent, hierarchy_id, file_id,
    ov_name, ov_args, ov_num_args,
    keytype, kindex, krid, manage, svlist, wref_id, &widget_id) ;
if ( result != MrmSUCCESS ) return result ;
*w_return = widget_id ;

/*
 * Initialize a context, and create all the children. Save their ids, and
 * save the ids of all managed children in the manage list. Note there are
 * no interior returns from the processing loop, and that all locally
 * acquired resources are returned at the routine exit.
 *
 * Initialize a sibling reference context for any class which allows
 * sibling widget references.
 */
widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer (context_id) ;
if ( widgetrec->children_offs == 0) return MrmSUCCESS ;

UrmGetResourceContext ((char *(*)())NULL, (void(*)())NULL, 0, &child_ctx) ;
UrmPlistInit (10, &manage_ids) ;
childrendesc = (RGMChildrenDescPtr) ((char *)widgetrec+widgetrec->children_offs) ;

for ( ndx=0 ; ndx<childrendesc->count ; ndx++ )
    {
    childptr = &childrendesc->child[ndx] ;

/*
 * Read the next child into the child context. Continue looping if it
 * can't be found. Reading the child from a hierarchy may modify the
 * file id, but only for reading the child's subtree.
 */
    loc_file_id = file_id ;
    switch ( childptr->type )
        {
        case URMrIndex:
            child_idx = (char *) widgetrec+childptr->key.index_offs ;
            if ( childptr->access == URMaPublic )
                result = UrmHGetWidget (hierarchy_id, child_idx,
                    child_ctx, &loc_file_id) ;
            else
                result =
                    UrmGetIndexedWidget (file_id, child_idx, child_ctx) ;
            if ( result != MrmSUCCESS )
                sprintf (err_msg, "Can't find indexed widget '%s'",
                    child_idx) ;
            break ;
        case URMrRID:
            result = UrmGetRIDWidget (file_id, childptr->key.id,
                child_ctx) ;
            if ( result != MrmSUCCESS )
                sprintf (err_msg, "Can't find RID widget '%x'",
                    childptr->key.id) ;
            break ;
        default:
            result = MrmFAILURE ;
            sprintf (err_msg, "?? UNKNOWN key type %d", childptr->type) ;
            break ;
        }
    if ( result != MrmSUCCESS )
        {
        Urm__UT_Error ("UrmCreateWidgetTree",
            err_msg, NULL, NULL, result) ;
        continue ;
        }

/*
 * Create the child and its subtree. Save its id in the manage list
 * if it is to be managed.
 */
    result = UrmCreateWidgetTree
        (child_ctx, widget_id, hierarchy_id, loc_file_id, NULL, NULL, 0,
        childptr->type, child_idx, childptr->key.id, childptr->manage, 
	svlist, wref_id, &child_id)  ;
    if ( result != MrmSUCCESS ) continue ;
    if ( childptr->manage ) UrmPlistAppendPointer (manage_ids, 
						   (XtPointer)child_id) ;

/*
 * loop end
 */
    }

/*
 * Manage all children which are in the manage list. Make sure they really
 * all have the same parent; if hidden shells or something have intervened,
 * do a ManageChild on each instead of ManageChildren.
 */
if ( manage_ids->num_ptrs > 0 )
    {
    check_parent =
      XtParent
	((Widget)
	 UrmPlistPtrN
	 (manage_ids,0)) ;
    one_parent = TRUE ;
    for ( ndx=1 ; ndx<UrmPlistNum(manage_ids) ; ndx++ )
	if ( XtParent((Widget)UrmPlistPtrN(manage_ids,ndx)) != check_parent )
	    {
	    one_parent = FALSE ; 
	    break ;
	    }
    if ( one_parent )
	XtManageChildren
	    ((WidgetList)UrmPlistPtrList(manage_ids), UrmPlistNum(manage_ids)) ;
    else
	for ( ndx=0 ; ndx<UrmPlistNum(manage_ids) ; ndx++ )
	    XtManageChild ((Widget)UrmPlistPtrN(manage_ids,ndx)) ;
    }

/*
 * done. Deallocate local resources.
 */
UrmFreeResourceContext (child_ctx) ;
UrmPlistFree (manage_ids) ;

return MrmSUCCESS ;        

}


Cardinal UrmCreateOrSetWidgetInstance
#ifndef _NO_PROTO
    (URMResourceContextPtr	context_id,
    Widget			parent,
    MrmHierarchy		hierarchy_id,
    IDBFile			file_id,
    String			ov_name,
    ArgList			ov_args,
    Cardinal			ov_num_args,
    MrmCode			keytype,
    String			kindex,
    MrmResource_id		krid,
    MrmFlag			manage,
    URMPointerListPtr		*svlist,
    URMResourceContextPtr	wref_id,
    Widget			*w_return)
#else
        (context_id, parent, hierarchy_id, file_id, ov_name, ov_args,
        ov_num_args, keytype, kindex, krid, manage, svlist, wref_id, w_return)
    URMResourceContextPtr	context_id ;
    Widget			parent ;
    MrmHierarchy		hierarchy_id ;
    IDBFile			file_id ;
    String			ov_name ;
    ArgList			ov_args ;
    Cardinal			ov_num_args ;
    MrmCode			keytype ;
    String			kindex ;
    MrmResource_id		krid ;
    MrmFlag			manage ; 
    URMPointerListPtr		*svlist ;
    URMResourceContextPtr	wref_id ;
    Widget			*w_return ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmCreateOrSetWidgetInstance determines from a RGM widget
 *	record if the widget instance is real and has to be created by
 *	a call to UrmCreateWidgetInstance or is an automatic child widget
 *	and has to be set by a call to UrmSetWidgetInstance.
 *
 *	Once UrmCreateOrSetWidgetInstance has been called, then the only
 *	information in the RGM record which may still be required is the
 *	privacy information and the widget children list. This information
 *	may be copied and the resource context reused by users who are doing
 *	recursive widget access, and wish to avoid recursive accumulation
 *	of resource contexts in memory.
 *
 *	The URM hierarchy for public resources and the IDB file for private
 *	resources are required to evaluate resource references occurring in
 *	the widget arglist.
 *
 *	This routine accepts override parameters for the widget name, and
 *	to override arguments in the creation arglist. The latter are appended
 *	to the list created from the UID file, and do not replace all values.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	context containing widget record describing widget
 *			to create
 *	parent		id of parent widget
 *	hierarchy_id	URM hierarchy from which to read public resources
 *	file_id		URM file from which to read private resources
 *	ov_name		Name to override widget name (NULL for no override)
 *	ov_args		Override arglist, exactly as would be given to
 *			XtCreateWidget (conversion complete, etc). NULL
 *			for no override.
 *	ov_num_args	# args in ov_args; 0 for no override
 *	keytype		type of key which accessed this widget
 *	kindex		index for URMrIndex access
 *	krid		resource id for URMrRID access
 *	svlist		list of SetValues descriptors
 *	wref_id		structure in which to resolve references to widgets
 *			which have already been defined.
 *	w_return	To return id of newly created widget
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 * 	MrmSUCCESS	operation succeeded
 *	MrmBAD_CONTEXT	invalid context
 *	MrmBAD_WIDGET_REC	invalid widget record
 *
 *  SIDE EFFECTS:
 *
 *--
 */

{

/*
 *  External Functions
 */

/*
 *  Local variables
 */
RGMWidgetRecordPtr	widgetrec ;	/* widget record in the context */

/*
 * Validate the context and the widget record in the context.
 * Check the variety and call the appropriate set or create function.
 */
if ( ! UrmRCValid(context_id) )
    return Urm__UT_Error
        ("UrmCreateOrSetWidgetInstance", "Invalid resource context",
        NULL, NULL, MrmBAD_CONTEXT) ;
widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer (context_id) ;
if ( ! UrmWRValid(widgetrec) )
    return Urm__UT_Error
        ("UrmCreateOrSetWidgetInstance", "Invalid widget record",
        NULL, context_id, MrmBAD_WIDGET_REC) ;

if (widgetrec->variety == UilMrmWidgetVariety)
  {
    return
      UrmCreateWidgetInstance(context_id, parent, hierarchy_id, file_id,
			      ov_name, ov_args, ov_num_args, keytype, kindex, 
			      krid, svlist, wref_id, w_return);
  }
else if (widgetrec->variety == UilMrmAutoChildVariety)
  {
    return
      UrmSetWidgetInstance(context_id, parent, hierarchy_id, file_id,
			   ov_args, ov_num_args, keytype, kindex, 
			   krid, manage, svlist, wref_id, w_return);
  }
else return Urm__UT_Error("UrmCreateOrSetWidgetInstance", 
			  "Unknown widget variety",
			  NULL, context_id, MrmBAD_WIDGET_REC);
}


Cardinal UrmCreateWidgetInstance
#ifndef _NO_PROTO
    (URMResourceContextPtr	context_id,
    Widget			parent,
    MrmHierarchy		hierarchy_id,
    IDBFile			file_id,
    String			ov_name,
    ArgList			ov_args,
    Cardinal			ov_num_args,
    MrmCode			keytype,
    String			kindex,
    MrmResource_id		krid,
    URMPointerListPtr		*svlist,
    URMResourceContextPtr	wref_id,
    Widget			*w_return)
#else
        (context_id, parent, hierarchy_id, file_id, ov_name, ov_args,
        ov_num_args, keytype, kindex, krid, svlist, wref_id, w_return)
    URMResourceContextPtr	context_id ;
    Widget			parent ;
    MrmHierarchy		hierarchy_id ;
    IDBFile			file_id ;
    String			ov_name ;
    ArgList			ov_args ;
    Cardinal			ov_num_args ;
    MrmCode			keytype ;
    String			kindex ;
    MrmResource_id		krid ;
    URMPointerListPtr		*svlist ;
    URMResourceContextPtr	wref_id ;
    Widget			*w_return ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmCreateWidgetInstance creates a widget instance from a RGM widget
 *	record by:
 *
 *		o Creating a legal XtCreateWidget arglist from the RGM
 *		  arglist by expanding compressed tags, evaluating values,
 *		  and doing type conversion.
 *
 *		o Deriving the correct low-level widget creation routine
 *		  from the RGM record's class specifier, and calling with
 *		  the given parent and the arglist.
 *
 *	Once UrmCreateWidgetInstance has been called, then the only
 *	information in the RGM record which may still be required is the
 *	privacy information and the widget children list. This information
 *	may be copied and the resource context reused by users who are doing
 *	recursive widget access, and wish to avoid recursive accumulation
 *	of resource contexts in memory (see next routine).
 *
 *	The URM hierarchy for public resources and the IDB file for private
 *	resources are required to evaluate resource references occurring in
 *	the widget arglist.
 *
 *	This routine accepts override parameters for the widget name, and
 *	to override arguments in the creation arglist. The latter are appended
 *	to the list created from the UID file, and do not replace all values.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	context containing widget record describing widget
 *			to create
 *	parent		id of parent widget
 *	hierarchy_id	URM hierarchy from which to read public resources
 *	file_id		URM file from which to read private resources
 *	ov_name		Name to override widget name (NULL for no override)
 *	ov_args		Override arglist, exactly as would be given to
 *			XtCreateWidget (conversion complete, etc). NULL
 *			for no override.
 *	ov_num_args	# args in ov_args; 0 for no override
 *	keytype		type of key which accessed this widget
 *	kindex		index for URMrIndex access
 *	krid		resource id for URMrRID access
 *	svlist		list of SetValues descriptors
 *	wref_id		structure in which to resolve references to widgets
 *			which have already been defined.
 *	w_return	To return id of newly created widget
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 * 	MrmSUCCESS	operation succeeded
 *	MrmBAD_CONTEXT	invalid context
 *	MrmBAD_WIDGET_REC	invalid widget record
 *
 *  SIDE EFFECTS:
 *
 *--
 */

{

/*
 *  External Functions
 */

/*
 *  Local variables
 */
Cardinal		result ;	/* function results */
RGMWidgetRecordPtr	widgetrec ;	/* widget record in the context */
RGMArgListDescPtr	argdesc = NULL ;
					/* arg list descriptor in record */
Arg			*args = NULL ;
					/* arg list argument for create */
Cardinal		num_used = 0 ;	/* number of args used in arglist */
MrmCount		num_listent = ov_num_args ;
					/* # entries in args */
WCIClassDescPtr		cldesc ;	/* class descriptor */
URMPointerListPtr	ptrlist = NULL ;
URMPointerListPtr	cblist = NULL ;
					/* to hold scratch callbacks */
					/* to hold scratch contexts */
URMPointerListPtr	ftllist = NULL ;
					/* to hold scratch fontlists */
char			*w_name ;	/* widget name */
int			ndx ;		/* loop index */
RGMCallbackDescPtr	cbptr ;		/* creation callback descriptor */
RGMCallbackItemPtr	itmptr ;	/* current callback item */
void			(* cb_rtn) () ;	/* current callback routine */
/* BEGIN OSF Fix pir 1860, 2813 */
XmAnyCallbackStruct	cb_reason; 	/* creation callback reason */
/* END OSF Fix pir 1860, 2813 */

/*
 * Validate the context and the widget record in the context.
 * Get the low-level creation routine pointer.
 */
if ( ! UrmRCValid(context_id) )
    return Urm__UT_Error
        ("UrmCreateWidgetInstance", "Invalid resource context",
        NULL, NULL, MrmBAD_CONTEXT) ;
widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer (context_id) ;
if ( ! UrmWRValid(widgetrec) )
    return Urm__UT_Error
        ("UrmCreateWidgetInstance", "Invalid widget record",
        NULL, context_id, MrmBAD_WIDGET_REC) ;

result = Urm__FindClassDescriptor
    (file_id,
     widgetrec->type,
     (XtPointer)((char *)widgetrec+widgetrec->class_offs),
     &cldesc) ;
if ( result != MrmSUCCESS )
    return result ;

/*
 * Allocate the args list, big enough for all the arguments in the widget
 * record plus all the override arguments. Also initialize a pointer list
 * to save any contexts created to evaluate resources.
 */
if ( widgetrec->arglist_offs != 0)
    {
    argdesc = (RGMArgListDescPtr) ((char *)widgetrec + widgetrec->arglist_offs) ;
    num_listent += argdesc->count + argdesc->extra ;
    UrmPlistInit (10, &ftllist) ;
    }
if ( num_listent > 0 )
    {
    args = (Arg *) XtMalloc (num_listent*sizeof(Arg)) ;
    UrmPlistInit (10, &ptrlist) ;
    }

/*
 * Set up the structure for the callback list to free memory on destory widget
 */
UrmPlistInit (10, &cblist);

/*
 * Set the arg list from the widget record argument list
 */
if ( argdesc != NULL )
    {
    Urm__CW_CreateArglist
	(parent, widgetrec, argdesc, ptrlist, cblist, ftllist,
	 hierarchy_id, file_id, args, svlist, wref_id, &num_used) ;
    }

/*
 * Copy in any override args
 */
for ( ndx=0 ; ndx<ov_num_args ; ndx++ )
    {
    args[ndx+num_used].name = ov_args[ndx].name ;
    args[ndx+num_used].value = ov_args[ndx].value ;
    }
num_used += ov_num_args ;

/*
 * Create the widget
 */
w_name =
    ov_name!=NULL ? ov_name : (char *)widgetrec+widgetrec->name_offs ;
*w_return = (*(cldesc->creator)) (parent, w_name, args, num_used) ;

/*
 * Add this widget to the widget reference structure, and update the
 * SetValues descriptors if appropriate
 */
Urm__CW_AddWRef (wref_id, w_name, *w_return) ;
if ( *svlist != NULL )
    Urm__CW_UpdateSVWidgetRef (svlist, w_name, *w_return) ;

/*
 * Call the creation callbacks if there are any.
 */
if ( widgetrec->creation_offs != 0)
  {
    if (strcmp(file_id->db_version, URM1_1version) <= 0)
      cbptr = Urm__CW_TranslateOldCallback((OldRGMCallbackDescPtr)
					   ((char *)widgetrec + 
					    widgetrec->creation_offs));
    else cbptr = (RGMCallbackDescPtr) ((char *)widgetrec + 
				       widgetrec->creation_offs) ;

    if ( ptrlist == NULL )
        UrmPlistInit (10, &ptrlist) ;
    result = Urm__CW_FixupCallback
        (parent, (XtPointer)widgetrec, cbptr, ptrlist, cblist, hierarchy_id, 
	 file_id, wref_id) ;
    if ( result == MrmSUCCESS )
        for ( ndx=0 ; ndx<cbptr->count ; ndx++ )
            {
            itmptr = &cbptr->item[ndx] ;

            cb_rtn = (void (*)()) itmptr->runtime.callback.callback ;
            if ( cb_rtn != (XtCallbackProc)NULL )
/* BEGIN OSF Fix pir 2813 */
	      {
		cb_reason.reason = MrmCR_CREATE;
		cb_reason.event = NULL;
		(*cb_rtn) (*w_return, itmptr->runtime.callback.closure, 
			   &cb_reason) ;
	      }
/* END OSF Fix pir 2813 */
            }
    else if (result == MrmUNRESOLVED_REFS)
      Urm__UT_Error("UrmCreateWidgetInstance",
		    "Unresolved Widget reference in creation callback",
		    NULL, NULL, MrmFAILURE) ;
    else return Urm__UT_Error("UrmCreateWidgetInstance",
			      "Couldn't Fixup creation callbacks",
			      NULL, NULL, MrmFAILURE);

    if (strcmp(file_id->db_version, URM1_1version) <= 0)
      XtFree((char *)cbptr);
    }

/*
 * successfully created (as far as we can tell). Deallocate all local
 * resources, including any contexts in the pointer list.
 */
if ( args != NULL ) XtFree ((char*)args) ;
if ( ptrlist != NULL )
    {
    for ( ndx=0 ; ndx<UrmPlistNum(ptrlist) ; ndx++ )
        UrmFreeResourceContext ((URMResourceContextPtr)UrmPlistPtrN(ptrlist,ndx)) ;
    UrmPlistFree (ptrlist) ;
    }

/*
 * Add a destroy callback if the widget had any callbacks or font-lists
 * associated with it.
 * Otherwise just wipe out the memory for the Plist now.
 */
if (cblist->num_ptrs > 0)
    {
    XtAddCallback (*w_return, XmNdestroyCallback, (XtCallbackProc) UrmDestroyCallback, cblist);
    }
else
    {
    UrmPlistFree (cblist);
    }

/*
** We should really let Xt take care of handling the fontlists by using its
** converters; but for the meanwhile avoid freeing the fontlists here, as the
** widget may be one which doesn't do an XmFontListCopy. Instead, later free
** our extra copy.
*/
if (ftllist != NULL)
    {
    if (UrmPlistNum(ftllist) > 0)
	XtAddCallback(*w_return, XmNdestroyCallback, 
		(XtCallbackProc) UrmDestroyCallback, ftllist);
    else
	UrmPlistFree (ftllist) ;
    }

return MrmSUCCESS ;

}


Cardinal UrmSetWidgetInstance
#ifndef _NO_PROTO
    (URMResourceContextPtr	context_id,
    Widget			parent,
    MrmHierarchy		hierarchy_id,
    IDBFile			file_id,
    ArgList			ov_args,
    Cardinal			ov_num_args,
    MrmCode			keytype,
    String			kindex,
    MrmResource_id		krid,
    MrmFlag			manage,
    URMPointerListPtr		*svlist,
    URMResourceContextPtr	wref_id,
    Widget			*w_return)
#else
        (context_id, parent, hierarchy_id, file_id, ov_args,
        ov_num_args, keytype, kindex, krid, manage, svlist, wref_id, w_return)
    URMResourceContextPtr	context_id ;
    Widget			parent ;
    MrmHierarchy		hierarchy_id ;
    IDBFile			file_id ;
    ArgList			ov_args ;
    Cardinal			ov_num_args ;
    MrmCode			keytype ;
    String			kindex ;
    MrmResource_id		krid ;
    MrmFlag			manage ; 
    URMPointerListPtr		*svlist ;
    URMResourceContextPtr	wref_id ;
    Widget			*w_return ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmSetWidgetInstance sets the appropriate resources from a RGM widget
 *	record on the appropriate automatically created child of parent by:
 *
 *		o Creating a legal XtSetValues arglist from the RGM
 *		  arglist by expanding compressed tags, evaluating values,
 *		  and doing type conversion.
 *
 *		o Finding the correct widget child of parent by uncompressing
 *		  the resource compression code found in the RGM record's
 *		  type specifier, and calling XtNameToWidget on the result.
 *
 *	Once UrmSetWidgetInstance has been called, then the only
 *	information in the RGM record which may still be required is the
 *	privacy information and the widget children list. This information
 *	may be copied and the resource context reused by users who are doing
 *	recursive widget access, and wish to avoid recursive accumulation
 *	of resource contexts in memory (see next routine).
 *
 *	The URM hierarchy for public resources and the IDB file for private
 *	resources are required to evaluate resource references occurring in
 *	the widget arglist.
 *
 *	This routine accepts override parameters to override
 *	arguments in the setvalues arglist. They are appended
 *	to the list created from the UID file, and do not replace all values.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	context containing widget record describing widget
 *			to create
 *	parent		id of parent widget
 *	hierarchy_id	URM hierarchy from which to read public resources
 *	file_id		URM file from which to read private resources
 *	ov_args		Override arglist, exactly as would be given to
 *			XtCreateWidget (conversion complete, etc). NULL
 *			for no override.
 *	ov_num_args	# args in ov_args; 0 for no override
 *	keytype		type of key which accessed this widget
 *	kindex		index for URMrIndex access
 *	krid		resource id for URMrRID access
 *	svlist		list of SetValues descriptors
 *	wref_id		structure in which to resolve references to widgets
 *			which have already been defined.
 *	w_return	To return id of newly created widget
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 * 	MrmSUCCESS	operation succeeded
 *	MrmBAD_CONTEXT	invalid context
 *	MrmBAD_WIDGET_REC	invalid widget record
 *
 *  SIDE EFFECTS:
 *
 *--
 */

{

/*
 *  External Functions
 */

/*
 *  Local variables
 */
Cardinal		result ;	/* function results */
RGMWidgetRecordPtr	widgetrec ;	/* widget record in the context */
String			c_name ;	/* child name */
String			c_name_tmp ;	/* child name - temporary */
RGMArgListDescPtr	argdesc = NULL ; /* arg list descriptor in record */
Arg			*args = NULL ;  /* arg list argument for create */
Cardinal		num_used = 0 ;	/* number of args used in arglist */
MrmCount		num_listent = ov_num_args ; /* # entries in args */
URMPointerListPtr	ptrlist = NULL ;/* to hold scratch contexts */
URMPointerListPtr	cblist = NULL ; /* to hold scratch callbacks */
URMPointerListPtr	ftllist = NULL ;/* to hold scratch fontlists */
int			ndx ;		/* loop index */
RGMCallbackDescPtr	cbptr ;		/* creation callback descriptor */
RGMCallbackItemPtr	itmptr ;	/* current callback item */
void			(* cb_rtn) () ;	/* current callback routine */
/* BEGIN OSF Fix pir 1860, 2813 */
XmAnyCallbackStruct	cb_reason; 	/* creation callback reason */
/* END OSF Fix pir 1860, 2813 */

/*
 * Validate the context and the widget record in the context.
 */
if ( ! UrmRCValid(context_id) )
    return Urm__UT_Error
        ("UrmSetWidgetInstance", "Invalid resource context",
        NULL, NULL, MrmBAD_CONTEXT) ;
widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer (context_id) ;
if ( ! UrmWRValid(widgetrec) )
    return Urm__UT_Error
        ("UrmSetWidgetInstance", "Invalid widget record",
        NULL, context_id, MrmBAD_WIDGET_REC) ;

result = Urm__UncompressCode
    (file_id,
     widgetrec->type,
     &c_name) ;
if ( result != MrmSUCCESS )
    return Urm__UT_Error("UrmSetWidgetInstance", "Unknown child type",
			 NULL, context_id, result) ;

/* Find the widget */
if (strcmp(c_name, "TearOffControl") == 0) /* Special case */
  *w_return = XmGetTearOffControl(parent);
else 
  {
    /* Need to add * for ScrolledText and ScrolledList */
    c_name_tmp = (String)ALLOCATE_LOCAL((strlen(c_name) + 2) * sizeof(char));
    sprintf(c_name_tmp, "*%s", c_name);
    *w_return = XtNameToWidget(parent, c_name_tmp);

    /* Deal with ScrollBars for ScrolledList and ScrolledText subclasses. */
    if ((*w_return == NULL) &&
	((strcmp(c_name, "VertScrollBar") == 0) ||
	 (strcmp(c_name, "HorScrollBar") == 0)))
      {
	*w_return = XtNameToWidget(XtParent(parent), c_name_tmp);
      }

    DEALLOCATE_LOCAL(c_name_tmp);
  }

if (*w_return == NULL)
    return Urm__UT_Error("UrmSetWidgetInstance", "Child of parent not found",
			 NULL, context_id, MrmFAILURE) ;
  
/*
 * Allocate the args list, big enough for all the arguments in the widget
 * record plus all the override arguments. Also initialize a pointer list
 * to save any contexts created to evaluate resources.
 */
if ( widgetrec->arglist_offs != 0)
    {
    argdesc = (RGMArgListDescPtr) ((int)widgetrec + widgetrec->arglist_offs) ;
    num_listent += argdesc->count + argdesc->extra ;
    UrmPlistInit (10, &ftllist) ;
    }
if ( num_listent > 0 )
    {
    args = (Arg *) XtMalloc (num_listent*sizeof(Arg)) ;
    UrmPlistInit (10, &ptrlist) ;
    }

/*
 * Set up the structure for the callback list to free memory on destroy widget
 */
UrmPlistInit (10, &cblist);

/*
 * Set the arg list from the widget record argument list
 */
if ( argdesc != NULL )
    {
    Urm__CW_CreateArglist
	(parent, widgetrec, argdesc, ptrlist, cblist, ftllist,
	 hierarchy_id, file_id, args, svlist, wref_id, &num_used) ;
    }

/*
 * Copy in any override args
 */
for ( ndx=0 ; ndx<ov_num_args ; ndx++ )
    {
    args[ndx+num_used].name = ov_args[ndx].name ;
    args[ndx+num_used].value = ov_args[ndx].value ;
    }
num_used += ov_num_args ;

/*
 * Set the widget values
 */

XtSetValues(*w_return, args, num_used) ;

/*
 * Call the creation callbacks if there are any.
 */
if ( widgetrec->creation_offs != 0)
  {
    if (strcmp(file_id->db_version, URM1_1version) <= 0)
      cbptr = Urm__CW_TranslateOldCallback((OldRGMCallbackDescPtr)
					   ((char *)widgetrec + 
					    widgetrec->creation_offs));
    else cbptr = (RGMCallbackDescPtr) ((char *)widgetrec + 
				       widgetrec->creation_offs) ;

    if ( ptrlist == NULL )
        UrmPlistInit (10, &ptrlist) ;
    result = Urm__CW_FixupCallback
        (parent, (XtPointer)widgetrec, cbptr, ptrlist, cblist, hierarchy_id, 
	 file_id, wref_id) ;
    if ( result == MrmSUCCESS )
        for ( ndx=0 ; ndx<cbptr->count ; ndx++ )
            {
            itmptr = &cbptr->item[ndx] ;

            cb_rtn = (void (*)()) itmptr->runtime.callback.callback ;
            if ( cb_rtn != (XtCallbackProc)NULL )
/* BEGIN OSF Fix pir 2813 */
	      {
		cb_reason.reason = MrmCR_CREATE;
		cb_reason.event = NULL;
		(*cb_rtn) (*w_return, itmptr->runtime.callback.closure, 
			   &cb_reason) ;
	      }
/* END OSF Fix pir 2813 */
            }
    else if (result == MrmUNRESOLVED_REFS)
      Urm__UT_Error("UrmCreateWidgetInstance",
		    "Unresolved Widget reference in creation callback",
		    NULL, NULL, MrmFAILURE) ;
    else return Urm__UT_Error("UrmCreateWidgetInstance",
			      "Couldn't Fixup creation callbacks",
			      NULL, NULL, MrmFAILURE);

    if (strcmp(file_id->db_version, URM1_1version) <= 0)
      XtFree((char *)cbptr);
    }

/*
 * Unmanage the child if so desired. 
 */
if (!manage) XtUnmanageChild(*w_return);

/*
 * successfully set (as far as we can tell). Deallocate all local
 * resources, including any contexts in the pointer list.
 */
if ( args != NULL ) XtFree ((char*)args) ;
if ( ptrlist != NULL )
    {
    for ( ndx=0 ; ndx<UrmPlistNum(ptrlist) ; ndx++ )
        UrmFreeResourceContext ((URMResourceContextPtr)UrmPlistPtrN(ptrlist,ndx)) ;
    UrmPlistFree (ptrlist) ;
    }

/*
 * Add a destroy callback if the widget had any callbacks or font-lists
 * associated with it.
 * Otherwise just wipe out the memory for the Plist now.
 */
if (cblist->num_ptrs > 0)
    {
    XtAddCallback (*w_return, XmNdestroyCallback, (XtCallbackProc) UrmDestroyCallback, cblist);
    }
else
    {
    UrmPlistFree (cblist);
    }

/*
** We should really let Xt take care of handling the fontlists by using its
** converters; but for the meanwhile avoid freeing the fontlists here, as the
** widget may be one which doesn't do an XmFontListCopy. Instead, later free
** our extra copy.
*/
if (ftllist != NULL)
    {
    if (UrmPlistNum(ftllist) > 0)
	XtAddCallback(*w_return, XmNdestroyCallback, 
		(XtCallbackProc) UrmDestroyCallback, ftllist);
    else
	UrmPlistFree (ftllist) ;
    }

return MrmSUCCESS ;

}


void Urm__CW_CreateArglist
#ifndef _NO_PROTO
    (Widget		parent,
    RGMWidgetRecordPtr		widgetrec,
    RGMArgListDescPtr		argdesc,
    URMPointerListPtr		ctxlist,
    URMPointerListPtr		cblist,
    URMPointerListPtr		ftllist,
    MrmHierarchy		hierarchy_id,
    IDBFile			file_id,
    ArgList			args,
    URMPointerListPtr		*svlist,
    URMResourceContextPtr	wref_id,
    Cardinal			*num_used)
#else
        (parent, widgetrec, argdesc, ctxlist, cblist, ftllist,
	 hierarchy_id, file_id, args, svlist, wref_id, num_used)
    Widget			parent ;
    RGMWidgetRecordPtr		widgetrec ;
    RGMArgListDescPtr		argdesc ;
    URMPointerListPtr		ctxlist ;
    URMPointerListPtr           cblist;
    URMPointerListPtr		ftllist ;
    MrmHierarchy		hierarchy_id ;
    IDBFile			file_id ;
    ArgList			args ;
    URMPointerListPtr		*svlist ;
    URMResourceContextPtr	wref_id ;
    Cardinal			*num_used ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Urm__CW_CreateArglist reads the arglist descriptor in an RGM widget
 *	record and produces a legal arglist for XtCreateWidget in the args
 *	parameter. Any argument which encounters an error, or which must
 *	be done with a SetValues, does not appear in the list.
 *
 *  FORMAL PARAMETERS:
 *
 *	parent		parent of the widget being created
 *	widgetrec	widget record pointer
 *	argdesc		arglist descriptor in widget record
 *	ctxlist		A pointer list to save contexts created to
 *			evaluate literals.
 *	ftllist		A pointer list to save fontlists created for use
 *			as resource values, and which must be freed
 *	hierarchy_id	URM hierarchy from which to read public resources
 *	file_id		URM file from which to read private resources
 *	args		buffer in which the arglist array of longwords is to
 *			be created. Caller guarantees that it is big enough
 *			(since caller knows number of arguments).
 *	svlist		SetValues descriptor list. This routine will add
 *			any SetValues widget arguments to this list.
 *	wref_id		reference structure from which references to
 *			previously created widgets in the tree can be
 *			resolved.
 *	num_used	Returns number of arguments actually set in args
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *  SIDE EFFECTS:
 *
 *--
 */

{

/*
 *  External Functions
 */

/*
 * Local structures
 */
typedef struct {
	RGMIconImagePtr	icon ;		/* icon to be converted		    */
	RGMArgumentPtr	pixarg ;	/* argument in widget record	    */
	String		filename;	/* file name if pixtype is bitmap   */
	MrmType		pixtype ;	/* MrmRtypeIconImage or		    */
					/* MrmRtypeXBitmapFile		    */
} _SavePixmapItem, *_SavePixmapItemPtr ;

/*
 *  Local variables
 */
Cardinal		result ;	/* function results */
int			ndx, cbndx;	/* loop indices */
RGMArgumentPtr		argptr ;	/* current argument descriptor */
MrmType			reptype ;	/* arg value representation type */
int			argval ;	/* arg value as it is in record */
int			vec_count ;	/* count of items in the vector */
int			val ;		/* value as immediate or pointer */
RGMCallbackDescPtr	cbptr ;		/* val as callback descriptor */
RGMCallbackItemPtr	items;		/* Callback items as RGM items */
XtCallbackRec		*callbacks;	/* Callback items as Xt callbacks */
RGMIconImagePtr		icon ;		/* val as icon image */
RGMResourceDescPtr	resptr ;	/* values as resource reference */
String			ref_name ;	/* referenced widget name */
Widget			ref_id ;	/* referenced widget id */
IDBFile			act_file ;	/* file from which literals read */
RGMTextVectorPtr	vecptr ;	/* text vector arg value */
char			err_msg[300] ;
_SavePixmapItem		pixargs[10] ;	/* to save pixmap args */
Cardinal		pixargs_cnt = 0 ;
					/* # pixargs saved */
_SavePixmapItemPtr	savepix ;	/* current saved pixmap entry */
Screen			*screen ;	/* screen for pixmaps */
Display			*display ;	/* display for pixmaps */
int			fgint = -1 ;	/* foreground for pixmaps. -1 means
					   not set */
int			bgint = -1 ;	/* background for pixmaps */
Pixmap			pixmap ;	/* result of icon conversion */
Cardinal		uncmp_res ;	/* string uncompression result */
String                  resource_name ; /* resource name for comparison */
int			vec_size ;
RGMFontListPtr		fontlist;	/* for converting old style fontlist */

/*
 * Loop through all the arguments in descriptor. An entry is made in the
 * in the arglist for each entry which can be successfully evaluated,
 * fixed up, and converted.
 *
 * Some arguments may be affected by other arg values in the arglist. These
 * are deferred so that these other values will
 * be available if they are present in the arglist. This removes order
 * dependency. All such arguments are handled as special cases:
 *	IconImages: saved in pixargs vector
 *
 * Ordering may have an important effect on finding widget references. In
 * particular, references to widgets in submenus depends on the submenus
 * being created before the reference, as the referenced widget then
 * appears in the widget reference structure. This is currently done
 * by the compiler, which orders submenus first in an arglist.
 */
for ( ndx=0 ; ndx<argdesc->count ; ndx++ )
    {
    argptr = &argdesc->args[ndx] ;
    reptype = argptr->arg_val.rep_type ;

/*
 * Create the value. Some representation types and arguments require
 * special handling. First, the immediate value or pointer is evaluated.
 * then special handling is done. If no special handling is required, then
 * the value is fixed up, converted, and put in the args list.
 *
 * Icon images are loaded (i.e. brought into memory and all pointer
 * fixups done), but they are then treated as SetValues args, and saved
 * for processing after the widget is created.
 */
    argval = Urm__CW_EvaluateValOrOffset (reptype, (XtPointer)widgetrec,
        argptr->arg_val.datum.ival, argptr->arg_val.datum.offset) ;
    val = argval ;
    switch ( reptype )
        {
        case MrmRtypeCallback:
	  if (strcmp(file_id->db_version, URM1_1version) <= 0)
	    cbptr = Urm__CW_TranslateOldCallback((OldRGMCallbackDescPtr)val);
	  else cbptr = (RGMCallbackDescPtr)val;

            result = Urm__CW_FixupCallback(parent, (XtPointer)widgetrec, cbptr,
                ctxlist, cblist, hierarchy_id, file_id, wref_id) ;
	    switch (result)
	      {
	      case MrmSUCCESS:
		/* Move individual items so array functions as callback list */
		items = cbptr->item;
		callbacks = (XtCallbackRec *)((RGMCallbackDescPtr)val)->item;
		
		for (cbndx = 0; cbndx <= cbptr->count; cbndx++)
		  /* <= so that null item is copied. */
		  {
		    callbacks[cbndx].callback = (XtCallbackProc)
		      items[cbndx].runtime.callback.callback;
		    callbacks[cbndx].closure = (XtPointer)
		      items[cbndx].runtime.callback.closure;
		  }
		
		val = (int)callbacks;
		break;
	      case MrmUNRESOLVED_REFS:
		Urm__CW_AppendCBSVWidgetRef
		  (file_id, svlist, cbptr, argptr->tag_code,
		   (String) ((int)widgetrec+argptr->stg_or_relcode.tag_offs));
		/* No break */
	      default:
		continue;
	      }
	  if (strcmp(file_id->db_version, URM1_1version) <= 0)
	    XtFree((char *)cbptr);
	  break ;
        case MrmRtypeResource:
            resptr = (RGMResourceDescPtr) val ;
            switch ( resptr->res_group )
                {
                case URMgWidget:
		    if ( ((unsigned char)resptr->cvt_type==RGMwrTypeSubTree) ||
			  Urm__IsSubtreeResource(file_id,argptr->tag_code) )
			{
			result = Urm__CW_LoadWidgetResource
			    (parent, widgetrec, resptr, ctxlist,
			     hierarchy_id, file_id, svlist,
			     wref_id, &val) ;
			if ( result != MrmSUCCESS ) continue ;
			}
		    else
			{
			if ( resptr->type != URMrIndex )
			    {
			    Urm__UT_Error ("Urm__CW_CreateArglist",
					   "Widget reference not Indexed",
					   NULL, NULL, MrmFAILURE) ;
			    continue;
			    }
			ref_name = (String) resptr->key.index;
			result = Urm__CW_FindWRef
			    (wref_id, ref_name, &ref_id) ;
			if ( result != MrmSUCCESS )
			    {
			    Urm__CW_AppendSVWidgetRef
				(file_id, svlist, ref_name, argptr->tag_code,
				 (String)widgetrec+
					  argptr->stg_or_relcode.tag_offs);
			    continue ;
			    }
			val = (int) ref_id ;
			}
                    break ;
                case URMgLiteral:
                    result = Urm__CW_ReadLiteral
			(resptr, hierarchy_id, file_id, ctxlist,
			 &reptype, &argval, &vec_count, &act_file, &vec_size) ;
                    val = argval ;
                    if ( result != MrmSUCCESS ) continue ;
                    if ( reptype == MrmRtypeIconImage )
                        {
                        savepix = &pixargs[pixargs_cnt] ;
                        savepix->icon = (RGMIconImagePtr) val ;
                        savepix->pixarg = argptr ;
			savepix->pixtype = reptype ;
                        pixargs_cnt += 1 ;
                        continue ;
                        }
                    if ( reptype == MrmRtypeXBitmapFile )
                        {
                        savepix = &pixargs[pixargs_cnt] ;
                        savepix->filename = (String) val ;
                        savepix->pixarg = argptr ;
			savepix->pixtype = reptype ;
                        pixargs_cnt += 1 ;
                        continue ;
                        }

		    if ((reptype == MrmRtypeFontList) &&
			(strcmp(file_id->db_version, URM1_1version) <= 0))
		      {
			int count = ((OldRGMFontListPtr)val)->count;

			fontlist = (RGMFontListPtr)
			  XtMalloc(sizeof(RGMFontList) +
				   (sizeof(RGMFontItem) * (count - 1)));
			result = Urm__CW_FixupValue((int)fontlist, reptype, 
						    (XtPointer)val, file_id);
			val = (int)fontlist;
		      }
		    else
		      result = Urm__CW_FixupValue (val, reptype, 
						   (XtPointer)val, file_id) ;
                    if ( result != MrmSUCCESS ) continue ;

		    result = Urm__CW_ConvertValue
			(&val, reptype, resptr->cvt_type,
			 XtDisplay(parent), hierarchy_id, ftllist) ;
                    if ( result != MrmSUCCESS ) continue ;
		    if ( argptr->tag_code == UilMrmUnknownCode )
		      {
			resource_name = (char *)
			  ((int)widgetrec+argptr->stg_or_relcode.tag_offs) ;
		      }
		    else
		      {
			uncmp_res = Urm__UncompressCode
			  (file_id, argptr->tag_code, &resource_name) ;
			if ( uncmp_res != MrmSUCCESS )
			  {
			    sprintf (err_msg,
				     "Couldn't uncompress string code %d",
				     argptr->tag_code) ;
			    Urm__UT_Error ("Urm__CW_CreateArglist",
					   err_msg, NULL, NULL, uncmp_res) ;
			  }
		      }

		    if ( strcmp(resource_name, XmNuserData) == 0)
		      {
			switch (reptype)
			  {
			  case MrmRtypeChar8Vector:
			  case MrmRtypeCStringVector:
			    vec_size -= (sizeof ( RGMTextVector ) - 
					 sizeof ( RGMTextEntry ));
			    break;
			  default:
			    break;
			  }
			Urm__CW_SafeCopyValue (&val, reptype, cblist, 
					       vec_count, vec_size);
		      }
                    break ;

                default:
                    Urm__UT_Error ("Urm__CW_CreateArglist",
                        "Unhandled resource group", NULL, NULL, MrmFAILURE) ;
                    continue ;
                }
            break ;
        case MrmRtypeIconImage:
            icon = (RGMIconImagePtr) val ;
            result = Urm__CW_LoadIconImage (icon, (XtPointer)widgetrec,
                hierarchy_id, file_id, ctxlist) ;
            if ( result != MrmSUCCESS ) continue ;
            savepix = &pixargs[pixargs_cnt] ;
            savepix->icon = icon ;
            savepix->pixarg = argptr ;
	    savepix->pixtype = reptype ;
            pixargs_cnt += 1 ;
            continue ;
        case MrmRtypeXBitmapFile:
            savepix = &pixargs[pixargs_cnt] ;
            savepix->filename = (String) val ;
            savepix->pixarg = argptr ;
	    savepix->pixtype = reptype ;
            pixargs_cnt += 1 ;
            continue ;
        default:
            result = Urm__CW_FixupValue (val, reptype, 
					 (XtPointer)widgetrec, file_id) ;
            if ( result != MrmSUCCESS ) continue ;
            result = Urm__CW_ConvertValue
		(&val, reptype, 0, XtDisplay(parent),
		 hierarchy_id, ftllist) ;
            if ( result != MrmSUCCESS ) continue ;
            break ;
        }

    args[*num_used].value = (XtArgVal)val ;

/*
 * Create the tag string in the name slot of the current entry. Also
 * do any special processing based on tag code:
 *	- Retain values of foreground and background if they are
 *	  explicitly set
 * 	- Set the count for some lists
 *
 * 'argval' has preserved the pointer to RGM structures which may have been
 * replaced in 'val' by a pointer to structures (lists) required by
 * the toolkit
 */
    if ( argptr->tag_code == UilMrmUnknownCode )
	{
	args[*num_used].name = (char *)
	  widgetrec+argptr->stg_or_relcode.tag_offs ;
	*num_used += 1 ;
	}
    else
	{
	uncmp_res = Urm__UncompressCode
	    (file_id, argptr->tag_code, &(args[*num_used].name)) ;
	if ( uncmp_res == MrmSUCCESS )
	    *num_used += 1 ;
	else
	    {
	    sprintf (err_msg, "Couldn't uncompress string code %d",
		     argptr->tag_code) ;
	    Urm__UT_Error ("Urm__CW_CreateArglist",
			   err_msg, NULL, NULL, uncmp_res) ;
	    }
	}

/*
 * Special processing:
 *	retain the value pointer for foreground or background. Note reference
 *	to name in arglist from previous operation.
 */
    if ( strcmp(args[*num_used-1].name,XmNforeground) == 0 )
	fgint = val ;
    if ( strcmp(args[*num_used-1].name,XmNbackground) == 0 )
	bgint = val ;

/*
 * Create an additional arglist entry for the count field for any argument
 * which has a related argument (which is always a counter)
 */
    if ( argptr->tag_code != UilMrmUnknownCode )
	if ( argptr->stg_or_relcode.related_code != 0)
	    {
	    switch ( reptype )
		{
		case MrmRtypeChar8Vector:
		case MrmRtypeCStringVector:
	            vecptr = (RGMTextVectorPtr) argval;
		    args[*num_used].value = (XtArgVal)vecptr->count;
		    break;
		}
	    uncmp_res = Urm__UncompressCode
		(file_id, argptr->stg_or_relcode.related_code,
		 &args[*num_used].name);
	    if ( uncmp_res == MrmSUCCESS )
		*num_used += 1;
	    else
		{
		sprintf (err_msg, "Couldn't uncompress string code %d",
			 argptr->tag_code) ;
		Urm__UT_Error ("Urm__CW_CreateArglist",
			       err_msg, NULL, NULL, uncmp_res) ;
		}
	    }

    }	/* Loop end */


/*
 * Now set any pixmap arguments. This requires finding the display, screen,
 * foreground, and background values for this widget. These values are
 * available from the parent widget and the arglist.
 */
if ( pixargs_cnt > 0 )
    {
    Urm__CW_GetPixmapParms (parent, &screen, &display, &fgint, &bgint) ;
    for ( ndx=0,savepix=pixargs ; ndx<pixargs_cnt ; ndx++,savepix++ )
        {
	if ( savepix->pixtype == MrmRtypeXBitmapFile ) {
	    result = Urm__CW_ReadBitmapFile
		(savepix->filename, screen, 
		 (Pixel)fgint, (Pixel)bgint, &pixmap, parent);
	    if ( result != MrmSUCCESS ) continue ;
	    }
	else {
	    /*
	    **  Create a pixmap from an Icon definition
	    */
	    result = UrmCreatePixmap (savepix->icon, screen, display,
		(Pixel)fgint, (Pixel)bgint, &pixmap, parent) ;
	    if ( result != MrmSUCCESS ) continue ;
	    }

	/*
	**  Place resultant Pixmap in arglist
	*/
        args[*num_used].value = (XtArgVal) pixmap ;
        argptr = savepix->pixarg ;
        if ( argptr->tag_code == UilMrmUnknownCode )
            args[*num_used].name = (char *) (widgetrec+argptr->stg_or_relcode.tag_offs) ;
        else
            Urm__UncompressCode
		(file_id, argptr->tag_code, &(args[*num_used].name)) ;
        *num_used += 1 ;
        }
    }

/*
 * arglist creation complete.
 */

}



int Urm__CW_EvaluateValOrOffset 

#ifndef _NO_PROTO
    (MrmType			reptype,
    XtPointer			bufptr,
    int				ival,
    MrmOffset			offset)
#else
(reptype, bufptr, ival, offset)
    MrmType			reptype ;
    XtPointer			bufptr ;
    int				ival ;
    MrmOffset			offset ;
#endif
/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	Return either a immediate value (ival) or a widget record memory
 *	pointer depending on the representation type.
 *
 *  FORMAL PARAMETERS:
 *
 *	reptype		representation type, from RGMrType...
 *	bufptr		buffer address for offset
 *	ival		immediate value
 *	offset		offset in widget record
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *  SIDE EFFECTS:
 *
 *--
 */

{

/*
 *  External Functions
 */

/*
 *  Local variables
 */


switch ( reptype )
    {
    case MrmRtypeInteger:
    case MrmRtypeBoolean:
    case MrmRtypeSingleFloat:
        return ival ;
    case MrmRtypeNull:
        return 0;
    default:
        return (int) ((char *)bufptr+offset) ;
    }

}



Cardinal Urm__CW_FixupValue 

#ifndef _NO_PROTO
    (int			val,
    MrmType			reptype,
    XtPointer			bufptr,
    IDBFile			file_id)
#else
(val, reptype, bufptr, file_id)
    int				val ;
    MrmType			reptype ;
    XtPointer			bufptr ;
    IDBFile			file_id ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine does any fixups required on a value. The fixups are
 *	usually relocation of pointers within the object located by the
 *	value interpreted as a pointer to a data structure.
 *
 *  FORMAL PARAMETERS:
 *
 *	val		value of an argument (may be a pointer)
 *	reptype		vaue representation type, from RGMrType...
 *	bufptr		the buffer (base address) for any fixed-up
 *			values
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS ;
 *
 *  SIDE EFFECTS:
 *
 *--
 */

{

/*
 *  External Functions
 */

/*
 *  Local variables
 */
RGMTextVectorPtr	vecptr ;	/* text vector arg value */
int			fixndx ;	/* list fixup loop index */
RGMFontItemPtr		fontitem;	/* resource as font item */
OldRGMFontItemPtr	olditem; 	/* old style font item */
RGMFontListPtr		fontlist ;	/* resource as font list */
OldRGMFontListPtr	oldlist ;	/* resource as old style font list */

switch ( reptype )
    {
    case MrmRtypeChar8Vector:
    case MrmRtypeCStringVector:
        vecptr = (RGMTextVectorPtr) val ;
        for ( fixndx=0 ; fixndx<vecptr->count ; fixndx++ )
            vecptr->item[fixndx].pointer = (XtPointer)
                ((char *)bufptr+vecptr->item[fixndx].text_item.offset) ;
        break ;
    case MrmRtypeFont:
    case MrmRtypeFontSet:
	fontitem = (RGMFontItemPtr) val;
	fontitem->cset.charset = (/*XmStringCharset*/String)
	    bufptr+fontitem->cset.cs_offs;
	fontitem->font.font = (String)
	    bufptr+fontitem->font.font_offs;
	break;
    case MrmRtypeFontList:
	if (strcmp(file_id->db_version, URM1_1version) <= 0)
	  /* Converting an old style fontlist */
	  {
	    oldlist = (OldRGMFontListPtr)bufptr;

	    fontlist = (RGMFontListPtr)val;
	    
	    fontlist->validation = oldlist->validation;
	    fontlist->count = oldlist->count;
	    
	    for ( fixndx=0 ; fixndx<oldlist->count ; fixndx++ )
	      {
		olditem = &oldlist->item[fixndx];
		fontitem = &fontlist->item[fixndx];
		
		fontitem->cset.charset = 
		  XtNewString(( /*XmStringCharset*/String)
			      bufptr+olditem->cset.cs_offs);
		fontitem->font.font = 
		  XtNewString((String)bufptr+olditem->font.font_offs);
		fontitem->type = MrmRtypeFont;
	      }
	  }
	else
	  {
	    fontlist = (RGMFontListPtr) val ;
	    for ( fixndx=0 ; fixndx<fontlist->count ; fixndx++ )
	      {
		fontitem = &fontlist->item[fixndx];
		fontitem->cset.charset = ( /*XmStringCharset*/String)
		  bufptr+fontitem->cset.cs_offs;
		fontitem->font.font = (String)
		  bufptr+fontitem->font.font_offs;
	      }
	  }
        break ;
    default:
        break ;
    }

return MrmSUCCESS ;

}



String Urm__CW_DisplayToString
#ifndef _NO_PROTO
    (char                       *val,
    String                      add_string,
    int                         add_string_size)
#else
        (val, add_string, add_string_size)
    char                        *val;
    String                      add_string;
    int                         add_string_size ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *      This routine converts the display into a string byte by
 *      byte. Any Null bytes are not (?) omitted in returned string.
 *
 *  FORMAL PARAMETERS:
 *
 *      val             the value to be converted (may be a pointer)
 *      add_sting       a string to be added to the returned string
 *                      after the display.
 *      add_string_size the additional string length when Calloc on
 *                       the return value is done.
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *      returns the string if one is created, otherwise returns null
 *
 *  SIDE EFFECTS:
 *
 *--
 */

{

/*
 *  External Functions
 */

/*
 *  Local variables
 */
String                  return_val;
unsigned int            dpysize = sizeof(Display *);
int                     ndx;
int                     count=0;

return_val = XtCalloc (1, dpysize + add_string_size);
if (return_val == NULL)
    {
    return (return_val);
    }

for (ndx=0 ; ndx<dpysize ; ndx++)
    {
    /* SUPPRESS 112 */
    if (val[ndx] != '\0')
        {
        /* SUPPRESS 112 */
        return_val[count] = val[ndx];
        count ++;
        }
    }

if (count == 0)
    {
    XtFree (return_val);
    return_val = NULL;
    return (return_val);
    }

strcat (&return_val[count], add_string);

return (return_val);

}


Cardinal Urm__CW_ConvertValue
#ifndef _NO_PROTO
    (int				*val,
    MrmType			reptype,
    MrmType			cvttype,
    Display			*display,
    MrmHierarchy		hierarchy_id,
    URMPointerListPtr		ftllist)
#else
	(val, reptype, cvttype, display, hierarchy_id, ftllist)
    int				*val ;
    MrmType			reptype ;
    MrmType			cvttype ;
    Display			*display ;
    MrmHierarchy		hierarchy_id ;
    URMPointerListPtr		ftllist ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine performs any type conversion required. All
 *	conversion required are specified by the representation
 *	type.
 *
 *	Where conversion of a representation is expensive, and the
 *	results of the conversion are repeatable, conversions are
 *	done once and cached in the callbacks routine hash table.
 *
 *  FORMAL PARAMETERS:
 *
 *	val		the value to be converted (may be a pointer)
 *	reptype		value representation type, from RGMrType...
 *	cvttype		conversion destination type, from RGMrType...
 *	display		needed for font and pixel creation
 *	hierarchy_id	for name lookup
 *	ftllist		A pointer list to save fontlists created for use
 *			as resource values, and which must be freed
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmFAILURE	conversion failure, don't use argument
 *
 *  SIDE EFFECTS:
 *
 *--
 */

{

/*
 *  External Functions
 */

/*
 *  Local variables
 */
Cardinal		result ;	/* function results */
XFontStruct		*font ;		/* result of conversion to font */
XFontSet		fontset ;	/* result of converstion to fontset */
char			**missing_csets;  /* For XCreateFontSet */
int			missing_cset_cnt; /* For XCreateFontSet */
char			*def_string;	/* For XCreateFontSet */
XmFontListEntry		fontset_entry; /* For creating fontlist */
XtTranslations		trans ;		/* result of parsing trans table */
WidgetClass		clrec ;		/* result of class name conversion */
XtPointer		addr ;		/* result of variable conversion */
String			fontstg ;	/* font id string */
RGMFontItemPtr		fontptr ;	/* val as font descriptor */
RGMFontListPtr		fontlist ;	/* val as font list */
XmFontList		dfontlist ;	/* converted font list */
RGMColorDescPtr		colorptr ;	/* val as color descriptor */
Pixel			pix ;		/* result of color/pixel conversion */
int			ndx ;		/* conversion loop index */
KeySym			xkey;		/* result of keysym conversion */
char			err_msg[300] ;

XmString                cstg ;          /* to copy compound strings */
String			dpyandfontstr;


switch ( reptype )
    {
    case MrmRtypeChar8:
        {
        switch ( cvttype )
	    {
	    case MrmRtypeCString:
/*
 * WARNING: memory leak created...
 */
	        cstg = XmStringLtoRCreate (((String)*val),
					   XmSTRING_DEFAULT_CHARSET);
	        *val = (int) cstg;
		if ( cstg == NULL )
		    {
		    sprintf
			(err_msg,
			 "Couldn't convert ASCIZ '%s' to compound string",
			 (String)(*val)) ;
		    return Urm__UT_Error ("Urm__CW_ConvertValue",
					  err_msg, NULL, NULL, MrmFAILURE) ;
		    }
		break;
	    case MrmRtypeTransTable:
/*
 * WARNING: memory leak created...
 */
		trans = XtParseTranslationTable ((String)(*val)) ;
		if ( trans == NULL )
		    {
		    sprintf
			(err_msg,
			 "Couldn't parse translation table '%s'",
			 (String)(*val)) ;
		    return Urm__UT_Error ("Urm__CW_ConvertValue",
					  err_msg, NULL, NULL, MrmFAILURE) ;
		    }
		*val = (int) trans ;
		break ;
	    }
	break;
	}
    case MrmRtypeChar8Vector:
    case MrmRtypeCStringVector:
	{
	RGMTextVectorPtr	vecptr ;	/* val as text vector */
	
        vecptr = (RGMTextVectorPtr) (*val) ;
        *val = (int) vecptr->item ;
        break ;
	}
    case MrmRtypeIntegerVector:
	{
	RGMIntegerVectorPtr	vecptr ;	/* val as integer vector */

        vecptr = (RGMIntegerVectorPtr) (*val) ;
        *val = (int) vecptr->item ;
        break ;
	}
    case MrmRtypeAddrName:
	result = Urm__LookupNameInHierarchy
	    (hierarchy_id, (String)(*val), &addr) ;
	if ( result != MrmSUCCESS )
	    {
	    sprintf (err_msg, "Couldn't convert identifier '%s'",
		     (String)(*val)) ;
	    return Urm__UT_Error ("Urm__CW_ConvertValue",
					   err_msg, NULL, NULL, result) ;
	    }
	*val = (int) addr ;
        break ;
    case MrmRtypeIconImage:
        return Urm__UT_Error ("Urm__CW_ConvertValue",
            "Internal error: case MrmRtypeIconImage reached",
            NULL, NULL, MrmFAILURE) ;
    case MrmRtypeXBitmapFile:
        return Urm__UT_Error ("Urm__CW_ConvertValue",
            "Internal error: case MrmRtypeXBitmapFile reached",
            NULL, NULL, MrmFAILURE) ;
    case MrmRtypeFont:
    case MrmRtypeFontSet:
        fontptr = (RGMFontItemPtr) (*val) ;
        fontstg = fontptr->font.font;
        dpyandfontstr = Urm__CW_DisplayToString ((char*)&display,
                                          fontstg, strlen(fontstg) + 1);
        if ( dpyandfontstr == NULL)
            {
            return Urm__UT_Error ("Urm__CW_ConvertValue",
                "Internal error: Couldn't convert Display to String",
                NULL, NULL, MrmFAILURE) ;
            }
	switch (reptype)
	  {
	  case MrmRtypeFont:
	    result = 
	      Urm__WCI_LookupRegisteredName(dpyandfontstr, (XtPointer *)&font);
	    
	    if ( result != MrmSUCCESS )
	      {
		font = XLoadQueryFont (display, fontstg);
		if ( font == NULL )
		  {
		    sprintf (err_msg, "Couldn't convert font '%s'", fontstg);
		    return Urm__UT_Error
		      ("Urm__CW_ConvertValue",
		       err_msg, NULL, NULL, MrmNOT_FOUND) ;
		  }
		Urm__WCI_RegisterNames (&dpyandfontstr, (XtPointer *)&font, 1);
			{
			XmDisplay dd = (XmDisplay) XmGetXmDisplay(display);
			if (dd)
			 XtAddCallback((Widget)dd,XtNdestroyCallback,
			  DisplayDestroyCallback, (XtPointer)
				XtNewString(dpyandfontstr));
			}
	      }
	    break;	    

	  case MrmRtypeFontSet:
	    result = 
	      Urm__WCI_LookupRegisteredName(dpyandfontstr, 
					    (XtPointer *)&fontset);
	    
	    if ( result != MrmSUCCESS )
	      {
		fontset = XCreateFontSet(display, fontstg, &missing_csets,
					 &missing_cset_cnt, &def_string);
		if (fontset == NULL)
		  {
		    sprintf(err_msg, "Couldn't convert fontset '%s'", 
			    fontstg);
		    return Urm__UT_Error
		      ("Urm__CW_ConvertValue",
		       err_msg, NULL, NULL, MrmNOT_FOUND) ;
		  }
		Urm__WCI_RegisterNames(&dpyandfontstr, 
				       (XtPointer *)&fontset, 1);
	      }
	    break;
	  }
        XtFree (dpyandfontstr);
	if ( cvttype == MrmRtypeFontList )
	    {
	      switch(reptype)
		{
		case MrmRtypeFont:
		  dfontlist = XmFontListCreate (font, fontptr->cset.charset) ;
		  break;
		  
		case MrmRtypeFontSet:
		  fontset_entry = XmFontListEntryCreate(fontptr->cset.charset,
							XmFONT_IS_FONTSET,
							fontset);
		  dfontlist = XmFontListAppendEntry(NULL, fontset_entry);
		  break;
		}
		  
	      if ( ftllist != NULL )
	        {
		UrmPlistAppendPointer (ftllist, (XtPointer)(long)reptype);
		UrmPlistAppendPointer (ftllist, (XtPointer)dfontlist);
		}
	      *val = (int) dfontlist ;
	    }
	else
	    *val = (int) font ;
        break ;
    case MrmRtypeFontList:
        fontlist = (RGMFontListPtr) (*val) ;
        dfontlist = NULL ;
        for ( ndx=0 ; ndx<fontlist->count ; ndx++ )
	  {
            fontstg = fontlist->item[ndx].font.font;
            dpyandfontstr = Urm__CW_DisplayToString((char*)&display,
						    fontstg, 
						    strlen(fontstg) + 1);
            if ( dpyandfontstr == NULL)
	      {
                return Urm__UT_Error ("Urm__CW_ConvertValue",
				      "Internal error: Couldn't convert Display to String",
				      NULL, NULL, MrmFAILURE) ;
	      }

	    switch (fontlist->item[ndx].type)
	      {
	      case MrmRtypeFont:
		result = 
		  Urm__WCI_LookupRegisteredName(dpyandfontstr, 
						(XtPointer *)&font);
		if ( result != MrmSUCCESS )
		  {
		    font = XLoadQueryFont (display, fontstg);
		    if ( font == NULL )
		      {
			sprintf (err_msg, "Couldn't convert font '%s'", 
				 fontstg);
			return Urm__UT_Error
			  ("Urm__CW_ConvertValue",
			   err_msg, NULL, NULL, MrmNOT_FOUND) ;
		      }
		    Urm__WCI_RegisterNames(&dpyandfontstr, 
					   (XtPointer *)&font, 1);
			{
			XmDisplay dd = (XmDisplay) XmGetXmDisplay(display);
			if (dd)
			 XtAddCallback((Widget)dd,XtNdestroyCallback,
			  DisplayDestroyCallback, (XtPointer)
				XtNewString(dpyandfontstr));
			}
		  }
		break;	    

	      case MrmRtypeFontSet:
		result = 
		  Urm__WCI_LookupRegisteredName(dpyandfontstr, 
						(XtPointer *)&fontset);
		if ( result != MrmSUCCESS )
		  {
		    fontset = XCreateFontSet(display, fontstg, &missing_csets,
					     &missing_cset_cnt, &def_string);
		    if (fontset == NULL)
		      {
			sprintf(err_msg, "Couldn't convert fontset '%s'", 
				fontstg);
			return Urm__UT_Error
			  ("Urm__CW_ConvertValue",
			   err_msg, NULL, NULL, MrmNOT_FOUND) ;
		      }

		    if (missing_csets != NULL)
		      {
			sprintf(err_msg,
				"Couldn't open one or more fonts for fontset '%s'",
				fontstg);
			XFreeStringList(missing_csets);
		      }
		    Urm__WCI_RegisterNames(&dpyandfontstr, 
					   (XtPointer *)&fontset, 1);
		  }
		break;
	      }

	    XtFree (dpyandfontstr);
	    switch(fontlist->item[ndx].type)
	      {
	      case MrmRtypeFont:
		if ( dfontlist == NULL )
		  dfontlist = XmFontListCreate
		    (font, fontlist->item[ndx].cset.charset) ;
		else
		  dfontlist = XmFontListAdd
		    (dfontlist, font, fontlist->item[ndx].cset.charset) ;
		if ( dfontlist == NULL )
		  {
		    sprintf (err_msg, 
			     "Couldn't add fontlist font '%s' to list",
			     fontlist->item[ndx].font.font) ;
		    return Urm__UT_Error ("Urm__CW_ConvertValue",
					  err_msg, NULL, NULL, MrmFAILURE) ;
		  }	
		break;
	      case MrmRtypeFontSet:
		fontset_entry = 
		  XmFontListEntryCreate(fontlist->item[ndx].cset.charset,
					XmFONT_IS_FONTSET,
					fontset);
		dfontlist = XmFontListAppendEntry(NULL, fontset_entry);
		if ( dfontlist == NULL )
		  {
		    sprintf (err_msg, 
			     "Couldn't add fontlist fontset '%s' to list",
			     fontlist->item[ndx].font.font) ;
		    return Urm__UT_Error ("Urm__CW_ConvertValue",
					  err_msg, NULL, NULL, MrmFAILURE) ;
		  }	
		break;
	      }
	  }
	
	*val = (int) dfontlist ;
	/*
	 * Save only the final fontlist to be freed later. All intermediate
	 * ones are freed by XmFontListAdd
	 */
	if ( ftllist != NULL )
	  {
	  UrmPlistAppendPointer (ftllist, (XtPointer)(long)reptype);
	  UrmPlistAppendPointer (ftllist, (XtPointer)dfontlist);
	  }
	break ;
    case MrmRtypeColor:
	colorptr = (RGMColorDescPtr) (*val) ;
	switch (colorptr->desc_type)
	  {
	  case URMColorDescTypeName:
	    result = Urm__UT_GetNamedColorPixel
	      (display, (Colormap)0, colorptr, &pix) ;
	    if ( result != MrmSUCCESS )
	      {
		sprintf (err_msg, "Couldn't convert color/pixel '%s'",
			 (String)(colorptr->desc.name)) ;
		return Urm__UT_Error ("Urm__CW_ConvertValue",
				      err_msg, NULL, NULL, MrmNOT_FOUND) ;
	      }
	    break;
	  case URMColorDescTypeRGB:
	    result = Urm__UT_GetColorPixel
	      (display, (Colormap)0, colorptr, &pix) ;
	    if ( result != MrmSUCCESS )
	      {
		sprintf (err_msg, "Couldn't convert RGB color/pixel '%d,%d,%d'",
			 colorptr->desc.rgb.red, 
			 colorptr->desc.rgb.green, 
			 colorptr->desc.rgb.blue) ;
		return Urm__UT_Error ("Urm__CW_ConvertValue",
				      err_msg, NULL, NULL, MrmNOT_FOUND) ;
	      }
	    break;
	  default:
	    sprintf
	      (err_msg,"Invalid color descriptor type");
	    return Urm__UT_Error ("Urm__CW_ConvertValue",
				  err_msg, NULL, NULL, MrmFAILURE) ;
	  };
	*val = (int) pix ;
	break ;
    case MrmRtypeTransTable:
	/*
	 * WARNING: memory leak created...
	 */
	trans = XtParseTranslationTable ((String)(*val)) ;
	if ( trans == NULL )
	  {
	    sprintf (err_msg, "Couldn't parse translation table '%s'",
		     (String)(*val)) ;
	    return Urm__UT_Error ("Urm__CW_ConvertValue",
				  err_msg, NULL, NULL, MrmFAILURE) ;
	  }
	*val = (int) trans ;
	break ;
    case MrmRtypeClassRecName:
	clrec = Urm__WCI_GetClRecOfName ((String)*val) ;
	if ( clrec == NULL )
	  {
	    sprintf (err_msg, "Couldn't convert class record name '%s'",
		     (String)(*val)) ;
	    return Urm__UT_Error ("Urm__CW_ConvertValue",
				  err_msg, NULL, NULL, MrmNOT_FOUND) ;
	  }
	*val = (int) clrec ;
	break ;
    case MrmRtypeKeysym:
	xkey = XStringToKeysym ((String)*val);
	if ( xkey == NoSymbol )
	  {
	    sprintf (err_msg, "Couldn't convert keysym string '%s' to KeySym",
		     (String)(*val)) ;
	    return Urm__UT_Error ("Urm__CW_ConvertValue",
				  err_msg, NULL, NULL, MrmNOT_FOUND) ;
	  }
	*val = (int) xkey;
	break;
      default:
	break ;
      }

return MrmSUCCESS ;

}

/*
** Remove the font from the hash table so it won't later cause a protocol
** error if the display is closed and reopened and the same value is fetched.
*/
static void 
DisplayDestroyCallback 
#ifdef _NO_PROTO
	( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
	( Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	String dpyandfontstr = (String) client_data;
	XFontStruct  *font ;

	if (MrmSUCCESS == Urm__WCI_LookupRegisteredName(dpyandfontstr,
                                                (XtPointer *)&font))
		XFreeFont(XtDisplay(w), font);
	Urm__WCI_UnregisterName (dpyandfontstr);
	XtFree(dpyandfontstr);
}



static char* staticNull = NULL;

void Urm__CW_SafeCopyValue 

#ifndef _NO_PROTO
    (int				*val,
    MrmType			reptype,
    URMPointerListPtr		cblist,
    int				vec_count,
    int				vec_size)
#else
(val, reptype, cblist, vec_count, vec_size)
    int				*val ;
    MrmType			reptype ;
    URMPointerListPtr		cblist;
    int				vec_count;
    int				vec_size;
#endif
/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine copies a value to an 'eternal' block in order
 *	to guarantee that callback tag values will live forever.
 *
 *  FORMAL PARAMETERS:
 *
 *	val		the value to be copied (may be a pointer)
 *	reptype		value representation type
 *	vec_count	number of elements in the vector (for vector types)
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *  SIDE EFFECTS:
 *
 *--
 */

{

/*
 *  External Functions
 */

/*
 *  Local variables
 */
int			*int_src ;	/* to copy integer values */
int			*int_dst ;
String			char8_src ;	/* to copy string values */
String			char8_dst ;
float			*single_float_src ;	/* to copy FP values */
float			*single_float_dst ;
double			*float_src ;	/* to copy FP values */
double			*float_dst ;
XmString		cstr_src ;	/* to copy compound strings */
XmString		*cstr_table_src ;  /* to copy compound strings table */
XmString		*cstr_table_dst ;
String			*char8_table_src ; /* to coy string table */
String			*char8_table_dst ;
wchar_t			*wchar_src;	/* to copy wide character strings */
wchar_t			*wchar_dst;
size_t			size;		
int			cnt ;

/*
 * Make copies of all primitive data structures. Note this has the side
 * effect of converting integer values to by-reference.
 */
switch ( reptype )
    {
    case MrmRtypeIntegerVector:
        int_src = (int *) *val ;
        int_dst = (int *) XtMalloc ((unsigned int)vec_size) ;
        UrmBCopy (int_src, int_dst, vec_size) ;
        *val = (int) int_dst ;
	if (cblist != NULL)
	  {
	    UrmPlistAppendPointer (cblist, (XtPointer)(long)reptype);
	    UrmPlistAppendPointer (cblist, (XtPointer)*val);
	  }
        break ;
    case MrmRtypeCStringVector:
        cstr_table_src = (XmString *)*val ;
	cstr_table_dst = (XmString *) XtMalloc (vec_size) ;
	UrmBCopy (cstr_table_src, cstr_table_dst, vec_size) ;
	for (cnt=0; cnt<vec_count; cnt++)
	    {
	    cstr_table_dst[cnt] = (XmString) ((int) cstr_table_dst + 
				    ((int) cstr_table_src[cnt] - 
				     (int) cstr_table_src)) ;
	    }
        *val = (int) cstr_table_dst ;
	if (cblist != NULL)
	    {
	    UrmPlistAppendPointer (cblist,  (XtPointer)(long)reptype);
	    UrmPlistAppendPointer (cblist,  (XtPointer)*val);
	    }
        break ;
    case MrmRtypeChar8Vector:
        char8_table_src = (String *)*val ;
        char8_table_dst = (String *) XtMalloc (vec_size) ;
        UrmBCopy (char8_table_src, char8_table_dst, vec_size) ;
	for (cnt=0; cnt<vec_count; cnt++)
	    {
	    char8_table_dst[cnt] = (String) ((int) char8_table_dst + 
				    ((int) char8_table_src[cnt] - 
				     (int) char8_table_src)) ;
	    }
        *val = (int) char8_table_dst ;
	if (cblist != NULL)
	    {
	    UrmPlistAppendPointer (cblist,  (XtPointer)(long)reptype);
	    UrmPlistAppendPointer (cblist,  (XtPointer)*val);
	    }
        break ;
    case MrmRtypeInteger:
    case MrmRtypeBoolean:
        int_dst = (int *) XtMalloc (sizeof(int)) ;
        *int_dst = *val ;
        *val = (int) int_dst ;
	if (cblist != NULL)
	  {
	    UrmPlistAppendPointer (cblist, (XtPointer)(long)reptype);
	    UrmPlistAppendPointer (cblist, (XtPointer)*val);
	  }
        break ;
    case MrmRtypeChar8:
        char8_src = (String) *val ;
        char8_dst = (String) XtMalloc (strlen(char8_src)+1) ;
        strcpy (char8_dst, char8_src) ;
        *val = (int) char8_dst ;
	if (cblist != NULL)
	  {
	    UrmPlistAppendPointer (cblist, (XtPointer)(long)reptype);
	    UrmPlistAppendPointer (cblist, (XtPointer)*val);
	  }
        break ;
    case MrmRtypeSingleFloat:
        single_float_src = (float *) *val ;
        single_float_dst = (float *) XtMalloc (sizeof(float)) ;
        *single_float_dst = *single_float_src ;
        *val = (int) single_float_dst ;
	if (cblist != NULL)
	  {
	    UrmPlistAppendPointer (cblist, (XtPointer)(long)reptype);
	    UrmPlistAppendPointer (cblist, (XtPointer)*val);
	  }
        break ;
    case MrmRtypeFloat:
        float_src = (double *) *val ;
        float_dst = (double *) XtMalloc (sizeof(double)) ;
        *float_dst = *float_src ;
        *val = (int) float_dst ;
	if (cblist != NULL)
	  {
	    UrmPlistAppendPointer (cblist, (XtPointer)(long)reptype);
	    UrmPlistAppendPointer (cblist, (XtPointer)*val);
	  }
        break ;
    case MrmRtypeNull:
	*val = (int) &staticNull ;
	/*
 	* Don't add anything to a callback list for the null type
 	*/
        break ;
    case MrmRtypeCString:
        cstr_src = XmStringCopy ((XmString)(*val)) ;
        *val = (int) cstr_src ;
	if (cblist != NULL)
	  {
	    UrmPlistAppendPointer (cblist, (XtPointer)(long)reptype);
	    UrmPlistAppendPointer (cblist, (XtPointer)*val);
	  }
        break ;
    case MrmRtypeWideCharacter:
        wchar_src = (wchar_t *) *val ;

	for (cnt = 0; ; cnt++) if (wchar_src[cnt] == 0) break;
	size = (cnt+1) * sizeof(wchar_t);
	
        wchar_dst = (wchar_t *) XtMalloc (size) ;
	memcpy(wchar_dst, wchar_src, size) ;
        *val = (int) wchar_dst ;
	if (cblist != NULL)
	  {
	    UrmPlistAppendPointer (cblist, (XtPointer)(long)reptype);
	    UrmPlistAppendPointer (cblist, (XtPointer)*val);
	  }
        break ;
    default:
        break ;
    }

return ;

}



void UrmDestroyCallback
#ifndef _NO_PROTO
    (Widget                     w,
    URMPointerListPtr           list_id,
    XmAnyCallbackStruct         * reason)
#else
(w, list_id, reason)
    Widget                      w ;
    URMPointerListPtr           list_id ;
    XmAnyCallbackStruct         * reason ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *      This routine destorys all the tags which where created for a given
 *      widgets callbacks. All the tags are stored in a pointer list. The first
 *      item on the list is the MRMtype followed by a pointer to the actual
 *      data. Use XmStringFree for comound string types,
 *	XmFontListFree for font-lists, and XtFree for all other
 *      types.
 *
 *  FORMAL PARAMETERS:
 *
 *      w       The widget we are freeing the memory for
 *      list_id The pointer to the list of tags
 *      reason  Standard callback reason (not used)
 *
 *  IMPLICIT INPUTS:
 *
 *      {@tbs@}
 *
 *  IMPLICIT OUTPUTS:
 *
 *      {@tbs@}
 *
 *  FUNCTION VALUE:
 *
 *      {@tbs@}
 *
 *  SIDE EFFECTS:
 *
 *      none known
 *
 *--
 */

{

/*
 *  External Functions
 */

/*
 *  Local variables
 */
MrmType                 reptype ;
int                     ndx ;

for (ndx=0 ; ndx<list_id->num_ptrs ; ndx++)
    {
    reptype = (MrmType)(long)list_id->ptr_vec[ndx];
    ndx++;
    switch ( reptype )
        {
        case MrmRtypeCString:
            XmStringFree ((XmString)list_id->ptr_vec[ndx]);
            break ;
	case MrmRtypeFontList:
		XmFontListFree ((XmFontList)list_id->ptr_vec[ndx]);
		break;
        default:
            XtFree (list_id->ptr_vec[ndx]);
            break ;
        }
    }

UrmPlistFree (list_id);

return;

}


Cardinal Urm__CW_ReadLiteral
	(resptr, hierarchy_id, file_id, ctxlist,
	 type, val, vec_count, act_file_id, vec_size)
    RGMResourceDescPtr		resptr ;
    MrmHierarchy		hierarchy_id ;
    IDBFile			file_id ;
    URMPointerListPtr		ctxlist ;
    MrmType			*type ;
    int				*val ;
    int				*vec_count ;
    IDBFile			*act_file_id ;
    int				*vec_size ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine reads a literal resource from either a hierarchy or
 *	a file. It returns the resource type and either a pointer to the
 *	value or its immediate value.
 *
 *  FORMAL PARAMETERS:
 *
 *	resptr		resource reference to literal
 *	hierarchy_id	hierarchy from which to read public resource
 *	file_id		file from which to read private resource
 *	ctxlist		list in which to save resource contexts
 *	type		to return representation type
 *	val		to return immediate value or pointer
 *	vec_count	to return number of items if type is a vector
 *	act_file_id	to return id of file from which literal was read
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *  SIDE EFFECTS:
 *
 *	MrmSUCCESS	operation succeeded
 *	other		some failure, usually reading the literal
 *
 *--
 */

{

/*
 *  External Functions
 */

/*
 *  Local variables
 */
Cardinal		result ;	/* function results */
URMResourceContextPtr	context_id ;	/* context for reading literal */
char			err_msg[300] ;
int			*bufptr ;	/* context buffer */

/*
 * Acquire a context and read the literal into it.
 */
UrmGetResourceContext ((char *(*)())NULL, (void(*)())NULL, 0, &context_id) ;
switch ( resptr->type )
    {
    case URMrIndex:
        if ( resptr->access == URMaPublic )
            result = Urm__HGetIndexedLiteral
                (hierarchy_id, resptr->key.index, context_id, act_file_id) ;
        else
            result = UrmGetIndexedLiteral
                (file_id, resptr->key.index, context_id) ;
        if ( result != MrmSUCCESS )
            {
            UrmFreeResourceContext (context_id) ;
            sprintf (err_msg, "Can't find indexed literal '%s'",
                resptr->key.index) ;
            return Urm__UT_Error ("Urm__CW_ReadLiteral", err_msg,
                NULL, NULL, result) ;
            }
        break ;
    case URMrRID:
        result = UrmGetRIDLiteral (file_id, resptr->key.id, context_id) ;
	*act_file_id = file_id ;
        if ( result != MrmSUCCESS )
            {
            UrmFreeResourceContext (context_id) ;
            sprintf (err_msg, "Can't find RID literal '%x'",
                resptr->key.id) ;
            return Urm__UT_Error ("Urm__CW_ReadLiteral", err_msg,
                NULL, NULL, result) ;
            }
        break ;
    default:
        result = MrmFAILURE ;
        UrmFreeResourceContext (context_id) ;
        sprintf ( err_msg, "Unknown literal key type %d",
            resptr->type) ;
        return Urm__UT_Error ("Urm__CW_ReadLiteral", err_msg,
            NULL, NULL, result) ;
    }

/*
 * return the rep type, size, and value. Save the resource context.
 */
*type = UrmRCType (context_id) ;
*vec_size = UrmRCSize(context_id);
*vec_count = 0;
bufptr = (int *) UrmRCBuffer (context_id) ;
*val = Urm__CW_EvaluateValOrOffset (*type, (XtPointer)bufptr, *bufptr, 0) ;
UrmPlistAppendPointer (ctxlist, (XtPointer)context_id) ;

/*
 * Handle literals which may have further embedded literal references. Note
 * that the file for private references is the local file, possibly changed
 * by the HGetIndexedLiteral
 */
switch ( *type )
    {
    case MrmRtypeIntegerVector:
	*vec_count = ((RGMIntegerVectorPtr)*val)->count ;
	break;
    case MrmRtypeChar8Vector:
    case MrmRtypeCStringVector:
	*vec_count = ((RGMTextVectorPtr)*val)->count ;
	break;
    case MrmRtypeIconImage:
        result = Urm__CW_LoadIconImage ((RGMIconImagePtr)*val,
            (XtPointer)*val, hierarchy_id, *act_file_id, ctxlist) ;
        if ( result != MrmSUCCESS ) return result ;
        break ;
    }

return MrmSUCCESS ;

}



Cardinal Urm__CW_LoadIconImage
        (iconptr, bufptr, hierarchy_id, file_id, ctxlist)
    RGMIconImagePtr		iconptr ;
    XtPointer			bufptr ;
    MrmHierarchy		hierarchy_id ;
    IDBFile			file_id ;
    URMPointerListPtr		ctxlist ;
/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine completes the loading and fixup of a URM
 *	icon image. Iconimages may have several other literal
 *	resource references embedded within their definition.
 *	These are all read and turned into memory references.
 *
 *  FORMAL PARAMETERS:
 *
 *	iconptr		The (unfixedup) icon image now in memory
 *	bufptr		buffer for offsets
 *	hierarchy_id	hierarchy from which to read public resource
 *	file_id		file from which to read private resource
 *	ctxlist		list in which to save resource contexts
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	other		failures from ReadLiteral
 *
 *  SIDE EFFECTS:
 *
 *--
 */

{

/*
 *  External Functions
 */

/*
 *  Local variables
 */
Cardinal		result ;	/* function results */
RGMResourceDescPtr	resptr ;	/* to read resource literals */
RGMColorTablePtr	ctable ;	/* color table in icon image */
Cardinal		ndx ;		/* loop index */
RGMColorTableEntryPtr	citem ;		/* color table entry */
MrmType			cttype = MrmRtypeColorTable ;
					/* expected type */
XtPointer		ctbufptr ;	/* buffer base addr for table */
MrmType			ctype ;		/* color entry type */
IDBFile			act_file ;	/* file from which literals read */
char			err_msg[300] ;
int			vec_size ;

/*
 * Fixup pointers as required. Read the color table if it is a resource. 
 * Note that bufptr is reset to a color table resource read in in order
 * provide the correct relocation for color items.
 */
iconptr->pixel_data.pdptr = (char *) bufptr+iconptr->pixel_data.pdoff ;
switch ( iconptr->ct_type )
    {
    case MrmRtypeColorTable:
        iconptr->color_table.ctptr = (RGMColorTablePtr)
            ((char *)bufptr+iconptr->color_table.ctoff) ;
        ctbufptr = bufptr ;
        break ;
    case MrmRtypeResource: {
	int vec_count;
        resptr = (RGMResourceDescPtr)
            ((char *)bufptr+iconptr->color_table.ctoff) ;
        result = Urm__CW_ReadLiteral
	    (resptr, hierarchy_id, file_id, ctxlist,
	     &cttype, (int *)(&iconptr->color_table.ctptr),
	     &vec_count, &act_file, &vec_size) ;
        if ( result != MrmSUCCESS ) return result ;
        if ( cttype != MrmRtypeColorTable )
            {
            sprintf (err_msg, "Invalid ColorTable literal type %d", cttype) ;
            return Urm__UT_Error ("Urm__CW_LoadIconImage",
                err_msg, NULL, NULL, MrmNOT_VALID) ;
            }        
        ctbufptr = (XtPointer) iconptr->color_table.ctptr ;
        break ;
	}
    default:
        sprintf (err_msg, "Invalid ColorTable type code %d",
            iconptr->ct_type) ;
        return Urm__UT_Error ("Urm__CW_LoadIconImage",
            err_msg, NULL, NULL, MrmNOT_VALID) ;
    }

/*
 * Load any resource colors in the color table.
 */
ctable = iconptr->color_table.ctptr ;
for ( ndx=URMColorTableUserMin ; ndx<ctable->count ; ndx++ )
    {
    citem = &ctable->item[ndx] ;
    switch ( citem->type )
        {
        case MrmRtypeColor:
            citem->color_item.cptr = (RGMColorDescPtr)
                ((char *)ctbufptr+citem->color_item.coffs) ;
            break ;
        case MrmRtypeResource: {
	    int vec_count;
            resptr = (RGMResourceDescPtr)
                ((char *)ctbufptr+citem->color_item.coffs) ;
            ctype = MrmRtypeColor ;
            result = Urm__CW_ReadLiteral
		(resptr, hierarchy_id, file_id,
                ctxlist, &ctype, (int *)(&citem->color_item.cptr),
		 &vec_count, &act_file, &vec_size) ;
            if ( result != MrmSUCCESS ) return result ;
            if ( ctype != MrmRtypeColor )
                {
                sprintf (err_msg, "Invalid Color literal type %d", ctype) ;
                return Urm__UT_Error ("Urm__CW_LoadIconImage",
                    err_msg, NULL, NULL, MrmNOT_VALID) ;
                }        
                break ;
	    }
        default:
            sprintf ( err_msg, "Invalid ColorItem type code %d", citem->type) ;
            return Urm__UT_Error ("Urm__CW_LoadIconImage",
                err_msg, NULL, NULL, MrmNOT_VALID) ;
        }
    }
return MrmSUCCESS ;

}



Cardinal Urm__CW_FixupCallback
  (parent, bufptr, cbdesc, ctxlist, cblist, hierarchy_id, file_id, wref_id)
    Widget			parent ;
    XtPointer			bufptr ;
    RGMCallbackDescPtr		cbdesc ;
    URMPointerListPtr		ctxlist ;
    URMPointerListPtr		cblist ;
    MrmHierarchy		hierarchy_id ;
    IDBFile			file_id ;
    URMResourceContextPtr	wref_id;
/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine fixes up a callback list in a record to function
 *	as the callback list passed to create. It must turn routine
 *	names into addresses, and evaluate tag values.
 *
 *  FORMAL PARAMETERS:
 *
 *	parent		this widget's parent
 *	bufptr		buffer (base address) for resolving offsets
 *	cbdesc		Callback descriptor in record. Its pointers
 *			must be fixed up and its tag values evaluated.
 *	ctxlist		A pointer list to save contexts created to
 *			evaluate literals.
 *	hierarchy_id	URM hierarchy from which to read public resources
 *	file_id		URM file from which to read private resources
 *	wref_id		reference structure from which references to
 *			previously created widgets in the tree can be
 *			resolved.
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		operation succeeded
 *	MrmUNRESOLVED_REFS	unresolved references to widgets remain
 *	other			conversion or resource evaluation failure
 *
 *  SIDE EFFECTS:
 *
 *--
 */

{

/*
 *  External Functions
 */

/*
 *  Local variables
 */
Cardinal		result ;	/* function results */
int			ndx ;		/* loop index */
RGMCallbackItemPtr	itmptr ;	/* current list item */
String			rtn_name ;	/* routine name in item */
MrmType			reptype ;	/* arg value representation type */
RGMResourceDescPtr	resptr ;	/* resource descriptor in tag */
IDBFile			act_file ;	/* file from which literals read */
XtPointer		rtn_addr ;	/* routine address */
int			tag_val ;	/* to save value */
int			vec_count ;	/* number of items in the vector */
char			err_msg[300] ;
MrmCount		unres_ref_count = 0;
/* number of unresolved widget references in callback list */
String			ref_name;	/* referenced widget name */
Widget			ref_id ;	/* referenced widget id */
int			vec_size ;
RGMFontListPtr  	fontlist;	/* for converting old style fontlist */

/*
 * Loop through all the items in the callback list
 */
for ( ndx=0 ; ndx<cbdesc->count ; ndx++ )
    {
    itmptr = &cbdesc->item[ndx] ;

/*
 * Set the routine pointer to the actual routine address. This routine name
 * must be a registered URM callback.
 */
    rtn_name = (String) bufptr + itmptr->cb_item.routine ;
    result = Urm__LookupNameInHierarchy (hierarchy_id, rtn_name, &rtn_addr) ;
    if ( result != MrmSUCCESS )
        {
        sprintf (err_msg, "Callback routine '%s' not registered", rtn_name) ;
        return Urm__UT_Error ("Urm__CW_FixupCallback",
            err_msg, NULL, NULL, result) ;
        }

/*
 * Evaluate the tag value, and set in the item.
 */
    reptype = itmptr->cb_item.rep_type ;
    tag_val = Urm__CW_EvaluateValOrOffset (reptype, bufptr,
        itmptr->cb_item.datum.ival, itmptr->cb_item.datum.offset) ;
    switch ( reptype )
        {
        case MrmRtypeResource:
            resptr = (RGMResourceDescPtr) tag_val ;
            switch ( resptr->res_group )
                {
                case URMgWidget:
		  /* Do we need to worry about subtree resources here? */
		  if (resptr->type != URMrIndex)
		    {
		      Urm__UT_Error("Urm__CW_FixupCallback",
				    "Widget reference not Indexed",
				    NULL, NULL, MrmNOT_VALID);
		      continue;
		    }
		  ref_name = (String) resptr->key.index;
		  /* See if reference can be resolved immediatetly. */
		  result = Urm__CW_FindWRef(wref_id, ref_name, &ref_id) ;
		  if ( result == MrmSUCCESS ) tag_val = (int)ref_id;
		  else {	/* Save to resolve later */
		    itmptr->runtime.resolved = FALSE;
		    itmptr->runtime.wname = Urm__UT_AllocString(ref_name);
		    tag_val = 0;
		    unres_ref_count++;
		  }
		  break;
		  
                case URMgLiteral:
		  result = Urm__CW_ReadLiteral
		    (resptr, hierarchy_id, file_id, ctxlist,
		     &reptype, &tag_val, &vec_count, &act_file, &vec_size);
		  if ( result != MrmSUCCESS ) continue ;

		  if ((reptype == MrmRtypeFontList) &&
		      (strcmp(file_id->db_version, URM1_1version) <= 0))
		    {
		      int count = ((OldRGMFontListPtr)tag_val)->count;

		      fontlist = (RGMFontListPtr)
			XtMalloc(sizeof(RGMFontList) +
				 (sizeof(RGMFontItem) * (count - 1)));
		      result = Urm__CW_FixupValue((int)fontlist, reptype, 
						  (XtPointer)tag_val, file_id);
		      XtFree((char *)tag_val);
		      tag_val = (int)fontlist;
		    }
		  else
		    result = Urm__CW_FixupValue (tag_val, reptype, 
						 (XtPointer)tag_val, file_id) ;

                    if ( result != MrmSUCCESS ) continue ;
                    result = Urm__CW_ConvertValue
			(&tag_val, reptype, (MrmType)0, XtDisplay(parent),
			 hierarchy_id, NULL) ;
                    if ( result != MrmSUCCESS ) continue ;

		    switch (reptype)
			{
			case MrmRtypeChar8Vector:
			case MrmRtypeCStringVector:
			    vec_size -= (sizeof ( RGMTextVector ) - 
					 sizeof ( RGMTextEntry ));
			    break;
			default:
			    break;
			}

                    Urm__CW_SafeCopyValue (&tag_val, reptype, cblist, 
					   vec_count, vec_size) ;
		  itmptr->runtime.resolved = TRUE;
                    break ;
                default:
                    return Urm__UT_Error ("Urm__CW_FixupCallback",
                        "Unhandled resource group", NULL, NULL, MrmFAILURE) ;
                }
            break ;
        default:
            result = Urm__CW_FixupValue (tag_val, reptype, bufptr, file_id) ;
            if ( result != MrmSUCCESS ) continue ;
            result = Urm__CW_ConvertValue
		(&tag_val, reptype, (MrmType)0, XtDisplay(parent),
		 hierarchy_id, NULL) ;
            Urm__CW_SafeCopyValue (&tag_val, reptype, cblist, 0, 0) ;
	    itmptr->runtime.resolved = TRUE;
            break ;
        }

    itmptr->runtime.callback.callback = (XtCallbackProc)rtn_addr;
    itmptr->runtime.callback.closure = (XtPointer) tag_val ;
  }

cbdesc->unres_ref_count = unres_ref_count;
if (unres_ref_count == 0)
  /*
   * callback list successfully fixed up
   */
  return MrmSUCCESS ;
else return MrmUNRESOLVED_REFS;

}


Cardinal Urm__CW_LoadWidgetResource
        (parent, widgetrec, resptr, ctxlist, hierarchy_id,
            file_id, svlist, wref_id, val)
    Widget			parent ;
    RGMWidgetRecordPtr		widgetrec ;
    RGMResourceDescPtr		resptr ;
    URMPointerListPtr		ctxlist ;
    MrmHierarchy		hierarchy_id ;
    IDBFile			file_id ;
    URMPointerListPtr		*svlist ;
    URMResourceContextPtr	wref_id ;
    int				*val ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine evaluates a resource reference to a widget by loading
 *	the widget definition and instantiating the widget tree. It returns
 *	the widget id.
 *	This routine evaluates a resource reference, resulting in setting
 *
 *  FORMAL PARAMETERS:
 *
 *	parent		parent of the widget being created
 *	widgetrec	widget record pointer
 *	resptr		the resource to be evaluated
 *	ctxlist		A pointer list to save contexts created to
 *			evaluate literals.
 *	hierarchy_id	URM hierarchy from which to read public resources
 *	file_id		URM file from which to read private resources
 *	wref_id		widget reference structure
 *	svlist		SetValues descriptor list
 *	val		to return the value (widget id)
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	other		load or instantiate failure
 *
 *  SIDE EFFECTS:
 *
 *--
 */

{

/*
 *  External Functions
 */

/*
 *  Local variables
 */
Cardinal		result ;	/* function results */
URMResourceContextPtr	context_id ;	/* context for widget record */
IDBFile			loc_fileid = file_id ;	/* file id from HGet */
char			err_msg[300] ;	/* to format error messages */


/*
 * Acquire a context, then load the widget and instantiate the tree.
 * An HGet call may replace the file for private references.
 */
UrmGetResourceContext ((char *(*)())NULL, (void(*)())NULL, 0, &context_id) ;
switch ( resptr->type )
    {
    case URMrIndex:
        if ( resptr->access == URMaPublic )
            result = UrmHGetWidget
                (hierarchy_id, resptr->key.index, context_id, &loc_fileid) ;
        else
            result = UrmGetIndexedWidget
                (file_id, resptr->key.index, context_id) ;
        if ( result != MrmSUCCESS )
            sprintf (err_msg, "Can't find indexed widget resource '%s'",
                resptr->key.index) ;
        break ;
    case URMrRID:
        result = UrmGetRIDWidget (file_id, resptr->key.id, context_id) ;
        if ( result != MrmSUCCESS )
            sprintf (err_msg, "Can't find RID widget resource '%x'",
                resptr->key.id) ;
        break ;
    default:
        result = MrmFAILURE ;
        sprintf ( err_msg, "Unknown resource key type %d", resptr->type) ;
    }
if ( result != MrmSUCCESS )
    {
    UrmFreeResourceContext (context_id) ;
    return Urm__UT_Error ("Urm__CW_LoadWidgetResource",
        err_msg, NULL, NULL, result) ;
    }

/*
 * Now create the widget subtree. The pointer result is the widget id of
 * the widget we now have (the root of the tree).
 */
result = UrmCreateWidgetTree
    (context_id, parent, hierarchy_id, loc_fileid, NULL, NULL, 0,
    resptr->type, resptr->key.index, resptr->key.id, TRUE,
     (URMPointerListPtr *)svlist, wref_id, (Widget *)val) ;
if ( result != MrmSUCCESS )
    Urm__UT_Error ("Urm__CW_LoadWidgetResource",
        "Couldn't instantitate widget tree", NULL, NULL, result) ;
UrmFreeResourceContext (context_id) ;
return result ;

}



void Urm__CW_GetPixmapParms (w, screen, display, fgint, bgint)
    Widget			w ;
    Screen			**screen ;
    Display			**display ;
    int				*fgint ;
    int				*bgint ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine returns parameters needed to create a pixmap from
 *	an IconImage. It extracts the screen and display from the given
 *	widget. It then determines the foreground and background colors
 *	for the widget. The setting of these values is idiosyncratic, due
 *	to the fact that all widgets have a Background attribute from core,
 *	but are not guaranteed to have a Foreground attribute.
 *		- if value is already set, do nothing
 *		- else choose the value for the widget if available
 *		- else choose white/black PixelOfScreen
 *		- make sure we haven't ended up with identical values
 *		  for both foreground and background. If we have, accept
 *		  the background value and set the foreground to something
 *		  else (black or white).
 *
 *  FORMAL PARAMETERS:
 *
 *	w		widget to use for default values
 *	screen		to return screen for pixmap
 *	display		to return display for pixmap
 *	fgint		to return foreground value for pixmap. A value of
 *			-1 on input means this must be set; else ignored
 *	bgint		to return background value for pixmap. -1 is used
 *			as above to signal value needed
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *  SIDE EFFECTS:
 *
 *--
 */

{

/*
 *  External Functions
 */

/*
 *  Local variables
 */
Arg			pixarg[2] ;	/* to read FG/BG values */
Cardinal		pcnt = 0 ;	/* # entries in arglist */

/*
 * Screen and display come straight from widget
 */
*screen = XtScreen (w) ;
*display = XtDisplay (w) ;

/*
 * Else load the foreground and background pixel values from the
 * widget. Fallback to Black/WhitePixelOfScreen if the widget
 * doesn't have these values.
 */
if ( *fgint == -1 )
    {
    XtSetArg (pixarg[pcnt], XmNforeground, fgint) ;
    pcnt += 1 ;
    }
if ( *bgint == -1 )
    {
    XtSetArg (pixarg[pcnt], XmNbackground, bgint) ;
    pcnt += 1 ;
    }
if ( pcnt > 0 )
    XtGetValues (w, pixarg, pcnt) ;

/*
 * Fall back on ...PixelOfScreen
 */
if ( *fgint == -1 )
    *fgint = (int) BlackPixelOfScreen (*screen) ;
if ( *bgint == -1 )
    *bgint = (int) WhitePixelOfScreen (*screen) ;

/*
 * Make sure we haven't ended with identical values
 */
if ( *fgint == *bgint )
    {
    if ( *bgint == (int) BlackPixelOfScreen(*screen) )
	*fgint = (int) WhitePixelOfScreen (*screen) ;
    else
	*fgint = (int) BlackPixelOfScreen (*screen) ;
    }

}


RGMCallbackDescPtr Urm__CW_TranslateOldCallback (oldptr)
     OldRGMCallbackDescPtr	oldptr;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine translates an RGMCallbackDescPtr stored in a 1.1 uid
 *	file into the equivalent 1.2+ structure.  This routine allocates
 *	memory which must later be freed using XtFree.
 *
 *  FORMAL PARAMETERS:
 *
 *	oldptr		Pointer into the buffer where the 1.1 callback
 *			descriptor starts.
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	This function returns a pointer to a new RGMCallbackDesc containing
 *	all the information from the structure stored in the uid file.
 *
 *  SIDE EFFECTS:
 *
 *	Memory is allocated which must be freed using XtFree.
 *
 *--
 */

{

/*
 *  External Functions
 */

/*
 *  Local variables
 */
  RGMCallbackDescPtr	cbptr;	/* pointer to new callback descriptor */
  RGMCallbackItemPtr	itmptr;	/* current callback item */
  OldRGMCallbackItemPtr	olditmptr;	/* callback item being converted */
  int			ndx;	/* loop index */
  
  cbptr = (RGMCallbackDescPtr) XtMalloc(sizeof(RGMCallbackDesc) +
					oldptr->count*sizeof(RGMCallbackItem));
  
  cbptr->validation = oldptr->validation;
  cbptr->count = oldptr->count;

  /* Loop through all items in old callback list copying to new. */
  for (ndx = 0; ndx <= cbptr->count; ndx++)
    /* <= so that null item is copied. */
    {
      olditmptr = &oldptr->item[ndx];
      itmptr = &cbptr->item[ndx];
      
      itmptr->cb_item.routine = olditmptr->cb_item.routine;
      itmptr->cb_item.rep_type = olditmptr->cb_item.rep_type;
      itmptr->cb_item.datum = olditmptr->cb_item.datum;
    }
  
  return cbptr;
}

