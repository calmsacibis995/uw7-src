#pragma ident	"@(#)m1.2libs:Mrm/Mrmwcrwr.c	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
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
 *	This module contains all the CWR routines (Create Widget Record)
 *	which create the contents of a widget record in a resource context.
 *
 *--
 */


/*
 *
 *  INCLUDE FILES
 *
 */


#include <Mrm/MrmAppl.h>
#include <Mrm/Mrm.h>


/*
 *
 *  TABLE OF CONTENTS
 *
 *	UrmCWRInit			Initialize CWR
 *
 *	UrmCWRSetClass			Set class name/code/variety
 *
 *	UrmCWRInitArglist		Initialize arglist
 *
 *	UrmCWRSetCompressedArgTag	Set compressed tag in arglist
 *
 *	UrmCWRSetUncompressedArgTag	Set uncompressed tag in arglist
 *
 *	UrmCWRSetArgValue		Set arg value (immediate)
 *
 *	UrmCWRSetArgResourceRef		Set arg value (resource reference)
 *
 *	UrmCWRSetArgChar8Vec		Set arg value to vector of strings
 *
 *	UrmCWRSetArgCStringVec		Set arg value to compound string vector
 *
 *	UrmCWRSetArgCallback		Set arg to be callback list
 *
 *	UrmCWRSetCallbackItem		Set callback item to immediate value
 *
 *	UrmCWRSetCallbackItemRes	Set callback item to resource
 *
 *	UrmCWRSetExtraArgs		Set extra args count
 *
 *	UrmCWRInitChildren		Initialize children list
 *
 *	UrmCWRSetChild			Set child in list
 *
 *	UrmCWRSetComment		Set widget comment
 *
 *	UrmCWRSetCreationCallback	Set widget creation callback
 *
 *	UrmCWR__AppendString		Append a string to the record
 *
 *	UrmCWR__GuaranteeSpace		Guarantee enough space in the record
 *
 *	UrmCWR__AppendResource		Append a resource descriptor
 *
 */


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



Cardinal UrmCWRInit 

#ifndef _NO_PROTO
    (URMResourceContextPtr	context_id,
    String			name,
    MrmCode			access,
    MrmCode			lock)
#else
(context_id, name, access, lock)
    URMResourceContextPtr	context_id ;
    String			name ;
    MrmCode			access ;
    MrmCode			lock ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmCWRInit initializes a context for creating a new widget record.
 *	It initializes a widget record in the context (reusing the already
 *	allocate buffer if possible). It stores the widget name, and correctly
 *	initializes the size field of the RGMWidgetRecord struct. This is used
 *	and maintained by all UrmCWR... routines to locate the next
 *	available location in the record. The access and locking attributes
 *	of the record are also set.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	resource context to initialize for the widget (must
 *			be already allocated with UrmGetResourceContext
 *	name		name of widget
 *	access		URMaPublic or URMaPrivate
 *	lock		TRUE if widget is to be write-protected
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS	operation succeeded
 *	MrmBAD_CONTEXT	resource context invalid
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
RGMWidgetRecordPtr	widgetrec ;	/* widget record in context */
MrmOffset		offset ;	/* offset for name */

/*
 * Validate record
 */
if ( ! UrmRCValid(context_id) )
    return Urm__UT_Error
        ("UrmCWRInit", "Invalid resource context",
        NULL, context_id, MrmBAD_CONTEXT) ;

/*
 * Set validation code, and set record size to size of header
 */
if ( UrmRCSize(context_id) <= _FULLWORD(RGMWidgetRecordHdrSize) )
    {
    result = UrmResizeResourceContext (context_id, _FULLWORD(RGMWidgetRecordHdrSize)) ;
    if ( result != MrmSUCCESS ) return result ;
    }
widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer(context_id) ;
widgetrec->validation = URMWidgetRecordValid ;
widgetrec->size = _FULLWORD (RGMWidgetRecordHdrSize) ;
widgetrec->access = access ;
widgetrec->lock = lock ;
UrmRCSetSize (context_id, widgetrec->size) ;

/*
 * Move in the name
 */
result = UrmCWR__AppendString (context_id, name, &offset) ;
if ( result != MrmSUCCESS ) return result ;
widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer(context_id) ;
widgetrec->name_offs = offset ;

/*
 * Null out all other fields and offsets
 */
widgetrec->type = 0 ;
widgetrec->class_offs = 0 ;
widgetrec->arglist_offs = 0 ;
widgetrec->children_offs = 0 ;
widgetrec->comment_offs = 0 ;
widgetrec->creation_offs = 0 ;
widgetrec->variety = 0 ;
widgetrec->annex = 0 ;

/*
 * Successfully initialized
 */
return MrmSUCCESS ;

}



Cardinal UrmCWRSetClass 
#ifndef _NO_PROTO
    (URMResourceContextPtr	context_id,
    MrmCode			type,
    String			class,
    unsigned long		variety)
#else
(context_id, type, class, variety)
    URMResourceContextPtr	context_id ;
    MrmCode			type ;
    String			class ;
    unsigned long		variety ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmSetClass appends the class specification to the widget record.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	resource context containing widget record
 *	type		Class type, from URMwc...
 *	class		string identifying class if not a toolkit/SDT widget.
 *			This string is stored only if type == URMwcUnknown
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		operation succeeded
 *	MrmBAD_CONTEXT		context not valid
 *	MrmBAD_WIDGET_REC	widget record not valid
 *	MrmBAD_CLASS_TYPE	unknown class type
 *	MrmNO_CLASS_NAME	empty class name for user (URMwcUnknown) class
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
RGMWidgetRecordPtr	widgetrec ;	/* widget record in context */
MrmOffset		offset ;	/* class name offset */


/*
 * Validate record
 */
UrmCWR__ValidateContext
    (context_id, "UrmCWRSetClass", URMWidgetRecordValid) ;

/*
 * Validate the class code. If unknown, must have a non-null class name.
 * Else needs only the code.
 */
if ( type == UilMrmUnknownCode )
    {
    if ( strlen(class) <= 0 )
	return Urm__UT_Error
	    ("UrmCWRSetClass", "Null user-defined class name",
	     NULL, context_id, MrmNO_CLASS_NAME) ;
    result = UrmCWR__AppendString (context_id, class, &offset) ;
    if ( result != MrmSUCCESS ) return result ;
    widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer (context_id) ;
    widgetrec->type = URMwcUnknown ;
    widgetrec->class_offs = offset ;
    widgetrec->variety = variety;
    return MrmSUCCESS ;
    }
else
    {
    if ( type < UilMrmMinValidCode )
	return Urm__UT_Error
	    ("UrmCWRSetClass", "Invalid class code",
	     NULL, context_id, MrmBAD_CLASS_TYPE) ;
    widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer(context_id) ;
    widgetrec->type = type ;
    widgetrec->class_offs = 0 ;
    widgetrec->variety = variety;
    return MrmSUCCESS ;
    }

}



Cardinal UrmCWRInitArglist (context_id, nargs)
    URMResourceContextPtr	context_id ;
    Cardinal			nargs ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmCWRInitArglist appends an empty RGMArgListDesc struct to the
 *	record. The arglist contains narg RGMArgument descriptors in a vector
 *	following the arglist header. These are then filled in by
 *	UrmCWRSet*ArgTag, and UrmCWRSet*ArgValue and its analogs.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	resource context containing widget record
 *	nargs		number of arguments in arglist
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		operation succeeded
 *	MrmBAD_CONTEXT		context not valid
 *	MrmBAD_WIDGET_REC	widget record not valid
 *	MrmTOO_MANY		number of arguments exceeds internal limit
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
RGMWidgetRecordPtr	widgetrec ;	/* widget record in context */
MrmSize			descsiz ;	/* descriptor size */
MrmOffset		offset ;	/* arglist descriptor offset */
RGMArgListDescPtr	argdesc ;	/* pointer to arglist in record */
int			ndx ;		/* loop index */

/*
 * Validate record
 */
UrmCWR__ValidateContext
    (context_id, "UrmCWRInitArglist", URMWidgetRecordValid) ;

/*
 * Error check that the number of arguments is reasonable
 */
if ( nargs > RGMListSizeMax )
    return Urm__UT_Error
        ("UrmCWRInitArgList", "Too many arguments",
        NULL, context_id, MrmTOO_MANY) ;

/*
 * compute the size required for the descriptor and the argument list,
 * and acquire the memory in the record. Note their is one argument
 * preallocated in the descriptor.
 */
descsiz = sizeof(RGMArgListDesc) + (nargs-1)*sizeof(RGMArgument) ;
result = UrmCWR__GuaranteeSpace (context_id, descsiz, &offset, (char **)&argdesc) ;
if ( result != MrmSUCCESS ) return result ;
widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer (context_id) ;
widgetrec->arglist_offs = offset ;

/*
 * initialize the descriptor and all the arguments
 */
argdesc->count = nargs ;
argdesc->extra = 0 ;
argdesc->annex1 = NULL ;
for ( ndx=0 ; ndx<nargs ; ndx++ )
    {
    argdesc->args[ndx].tag_code = 0 ;
    argdesc->args[ndx].stg_or_relcode.tag_offs = 0 ;
    argdesc->args[ndx].arg_val.datum.ival = NULL ;
    }

/*
 * successfully added
 */
return MrmSUCCESS ;

}



Cardinal UrmCWRSetCompressedArgTag 
#ifndef _NO_PROTO
    (URMResourceContextPtr	context_id,
    Cardinal			arg_ndx,
    MrmCode			tag,
    MrmCode			related_tag)
#else
(context_id, arg_ndx, tag, related_tag)
    URMResourceContextPtr	context_id ;
    Cardinal			arg_ndx ;
    MrmCode			tag ;
    MrmCode			related_tag;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmCWRSetCompressedArgTag sets the tag of the specified argument
 *	in the arglist. UrmCWRSetCompressedTag is provided for callers who
 *	already know the compressed code for the tag.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	resource context containing widget record
 *	arg_ndx		the 0-based index of the argument in the arglist
 *	tag		code specifying tag for argument from a compression
 *			code set
 *	related_tag	code for a related argument from the same compression
 *			code set
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		operation succeeded
 *	MrmBAD_CONTEXT		resource context invalid
 *	MrmBAD_WIDGET_REC	widget record not valid
 *	MrmNULL_DESC		arglist descriptor is null
 *	MrmOUT_OF_BOUNDS	arg_ndx out of bounds
 *	MrmBAD_COMPRESS		invalid compressed string code
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
RGMArgListDescPtr	argdesc ;	/* arglist desc in record */
RGMArgumentPtr		argptr ;	/* argument being set */


/*
 * Validate record, arglist descriptor, and argument number
 */
UrmCWR__ValidateContext (context_id, "UrmCWRSetCompressedArgTag", NULL) ;
UrmCWR__BindArgPtrs
    (context_id, "UrmCWRSetCompressedArgTag", arg_ndx, &argdesc, &argptr) ;

/*
 * Validate the compressed code and set the argument.
 */
if ( tag < UilMrmMinValidCode )
    return Urm__UT_Error
        ("UrmCWRSetCompressedArgTag", "Invalid compression code",
        NULL, context_id, MrmBAD_COMPRESS) ;

argptr->tag_code = tag ;
argptr->stg_or_relcode.related_code = related_tag;

/*
 * successfully added
 */
return MrmSUCCESS ;

}



Cardinal UrmCWRSetUncompressedArgTag (context_id, arg_ndx, tag)
    URMResourceContextPtr	context_id ;
    Cardinal			arg_ndx ;
    String			tag ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmCWRSetUncompressedArgTag sets the tag of the specified argument
 *	in thearglist.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	resource context containing widget record
 *	arg_ndx		the 0-based index of the argument in the arglist
 *	tag		string nameing the argument
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		operation succeeded
 *	MrmBAD_CONTEXT		resource context invalid
 *	MrmBAD_WIDGET_REC	widget record not valid
 *	MrmNULL_DESC		arglist descriptor is null
 *	MrmOUT_OF_BOUNDS	arg_ndx out of bounds
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
RGMArgListDescPtr	argdesc ;	/* arglist desc in record */
RGMArgumentPtr		argptr ;	/* argument being set */
MrmOffset		offset ;	/* tag string offset */


/*
 * Validate record, arglist descriptor, and argument number
 */
UrmCWR__ValidateContext (context_id, "UrmCWRSetUncompressedArgTag", NULL) ;
UrmCWR__BindArgPtrs
    (context_id, "UrmCWRSetUncompressedArgTag", arg_ndx, &argdesc, &argptr) ;

/*
 * Append the tag string to the record and set the argument
 */
result = UrmCWR__AppendString (context_id, tag, &offset) ;
if ( result != MrmSUCCESS ) return result ;

UrmCWR__BindArgPtrs
    (context_id, "UrmCWRSetUncompressedArgTag", arg_ndx, &argdesc, &argptr) ;

argptr->tag_code = UilMrmUnknownCode ;
argptr->stg_or_relcode.tag_offs = offset ;

/*
 * successfully added
 */
return MrmSUCCESS ;

}



Cardinal UrmCWRSetArgValue 
#ifndef _NO_PROTO
    (URMResourceContextPtr	context_id,
    Cardinal			arg_ndx,
    MrmCode			type,
    unsigned			arg_val)
#else
(context_id, arg_ndx, type, arg_val)
    URMResourceContextPtr	context_id ;
    Cardinal			arg_ndx ;
    MrmCode			type ;
    unsigned			arg_val ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmCWRSetArgValue is used to supply the value for an argument for
 *	all representation types listed below. For others, see below. The usage
 *	of the arg_val parameter depends on the value of the type parameter, as
 *	follows; arg_val is CAST in order to access the caller's value for the
 *	argument:
 *
 *
 *	MrmRtypeInteger (value):	arg_val is CAST to an integer
 *
 *	MrmRtypeBoolean (value):	arg_val is CAST to a boolean value
 *
 *	MrmRtypeSingleFloat (value):	arg_val is CAST to a float
 *
 *	MrmRtypeChar8 (reference):	arg_val is CAST to a pointer to a
 *	MrmRtypeAddrName (reference):	string
 *	MrmRtypeTransTable (reference):	
 *	MrmRtypeClassRecName (reference):
 *	MrmRtypeKeysym(reference):
 *
 *	MrmRtypeCString (reference):	arg_val is CAST to a pointer to a
 *					compound string
 *
 *	MrmRtypeFloat (reference):	arg_val is CAST to a pointer to a
 *					double constant
 *
 *	MrmRtypeNull			No value, enter null.
 *
 *	MrmRtypeColorTable		Allocate and copy the color table
 *
 *	MrmRtypeIconImage		Allocate and copy the icon. Currently,
 *					the color table must be allocated
 *					with the icon imate.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	resource context containing widget record
 *	arg_ndx		the 0-based index of the argument in the arglist
 *	type		the representation type for the value, from RGMrType...
 *	arg_val		a longword which will be CAST and used as required to
 *			access the value to be stored (copied) into the widget record.
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		operation succeeded
 *	MrmBAD_CONTEXT		resource context invalid
 *	MrmBAD_WIDGET_REC	widget record not valid
 *	MrmNULL_DESC		arglist descriptor is null
 *	MrmOUT_OF_BOUNDS	arg_ndx out of bounds
 *	MrmBAD_ARG_TYPE		unknown or unhandled representation type
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
RGMArgListDescPtr	argdesc ;	/* arglist desc in record */
RGMArgumentPtr		argptr ;	/* argument being set */
MrmOffset		offset, dumoff ;	/* non-immediate value offset */
double			*dblptr ;	/* pointer to double constant */
char			*dumaddr;	/* dummy address for GuaranteeSpace */
Cardinal		diff;		/* alignment diff for dblptr */
RGMColorTablePtr	src_ct ;	/* arg_val as color table pointer */
RGMColorTablePtr	dst_ct ;	/* color table allocated in record */
RGMIconImagePtr		src_icon ;	/* arg_val as icon pointer */
RGMIconImagePtr		dst_icon ;	/* icon allocated in record */


/*
 * Validate record, arglist descriptor, and argument number.
 * Set the argument type now.
 */
UrmCWR__ValidateContext (context_id, "UrmCWRSetArgValue", NULL) ;
UrmCWR__BindArgPtrs
    (context_id, "UrmCWRSetArgValue", arg_ndx, &argdesc, &argptr) ;

argptr->arg_val.rep_type = type ;

/*
 * Set the argument value depending on the type
 */
switch ( type )
    {
    case MrmRtypeInteger:
    case MrmRtypeBoolean:
    case MrmRtypeSingleFloat:
        argptr->arg_val.datum.ival = (int) arg_val ;
        return MrmSUCCESS ;
    case MrmRtypeChar8:
    case MrmRtypeAddrName:
    case MrmRtypeTransTable:
    case MrmRtypeClassRecName:
    case MrmRtypeKeysym:
        result = UrmCWR__AppendString
            (context_id, (char *)arg_val, &offset) ;
        if ( result != MrmSUCCESS ) return result ;
        UrmCWR__BindArgPtrs
            (context_id, "UrmCWRSetArgValue", arg_ndx, &argdesc, &argptr) ;
        argptr->arg_val.datum.offset = offset ;
        return MrmSUCCESS ;
    case MrmRtypeCString:
        result = UrmCWR__AppendCString
            (context_id, (XmString)arg_val, &offset) ;
        if ( result != MrmSUCCESS ) return result ;
        UrmCWR__BindArgPtrs
            (context_id, "UrmCWRSetArgValue", arg_ndx, &argdesc, &argptr) ;
        argptr->arg_val.datum.offset = offset ;
        return MrmSUCCESS ;
    case MrmRtypeWideCharacter:
        result = UrmCWR__AppendWcharString
            (context_id, (wchar_t *)arg_val, &offset) ;
        if ( result != MrmSUCCESS ) return result ;
        UrmCWR__BindArgPtrs
            (context_id, "UrmCWRSetArgValue", arg_ndx, &argdesc, &argptr) ;
        argptr->arg_val.datum.offset = offset ;
        return MrmSUCCESS ;
    case MrmRtypeFloat:
        result = UrmCWR__GuaranteeSpace
            (context_id, sizeof(double), &offset, (char **) &dblptr) ;
        if ( result != MrmSUCCESS ) return result ;
 
 	/* This is necessary for machines (such as hp9000) that require
 	   doubles to be aligned on 8 byte boundaries. */
 	diff = ((long)dblptr % 8);
 	
 	if (diff != 0)
 	  {
 	    result = UrmCWR__GuaranteeSpace
 	      (context_id, diff, &dumoff, &dumaddr) ;
 	    if ( result != MrmSUCCESS ) return result ;
 	  }	    
 	dblptr = (double *)((char *)dblptr + diff);
 	
 	*dblptr = *((double *) arg_val); 
 	
        UrmCWR__BindArgPtrs
            (context_id, "UrmCWRSetArgValue", arg_ndx, &argdesc, &argptr) ;
        argptr->arg_val.datum.offset = offset ;
        return MrmSUCCESS ;
    case MrmRtypeNull:
        argptr->arg_val.datum.ival = (int) NULL ;
        return MrmSUCCESS ;
    case MrmRtypeColorTable:
        src_ct = (RGMColorTablePtr) arg_val ;
        result = UrmCWR__GuaranteeSpace
            (context_id, UrmColorTableSize(src_ct), &offset, (char **)&dst_ct) ;
        if ( result != MrmSUCCESS )
            return result ;
        UrmCopyAllocatedColorTable (dst_ct, src_ct) ;
        UrmCWR__BindArgPtrs
            (context_id, "UrmCWRSetArgValue", arg_ndx, &argdesc, &argptr) ;
        argptr->arg_val.datum.offset = offset ;
        return MrmSUCCESS ;
    case MrmRtypeIconImage:
        src_icon = (RGMIconImagePtr) arg_val ;
        result = UrmCWR__GuaranteeSpace
            (context_id, UrmIconImageSize(src_icon), &offset, (char **)&dst_icon) ;
        if ( result != MrmSUCCESS )
            return result ;
        UrmCopyAllocatedIconImage (dst_icon, src_icon) ;
/* ??relocate pointers to offsets?? */
        UrmCWR__BindArgPtrs
            (context_id, "UrmCWRSetArgValue", arg_ndx, &argdesc, &argptr) ;
        argptr->arg_val.datum.offset = offset ;
        return MrmSUCCESS ;
    default:
        return Urm__UT_Error
            ("UrmCWRSetArgValue", "Invalid or unhandled type",
            NULL, context_id, MrmBAD_ARG_TYPE) ;
    }

}



Cardinal UrmCWRSetArgResourceRef
#ifndef _NO_PROTO
    (URMResourceContextPtr	context_id,
    Cardinal			arg_ndx,
    MrmCode			access,
    MrmGroup			group,
    MrmCode				type,
    MrmCode			key_type,
    String			index,
    MrmResource_id		resource_id)
#else
        (context_id, arg_ndx, access, group, type,
            key_type, index, resource_id)
    URMResourceContextPtr	context_id ;
    Cardinal			arg_ndx ;
    MrmCode			access ;
    MrmGroup			group ;
    MrmCode			type ;
    MrmCode			key_type ;
    String			index ;
    MrmResource_id		resource_id ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmCCWRSetArgResourceRef creates a resource reference as the value
 *	of a MrmRtypeResource argument. Only one of the index or resource_id
 *	parameters is used, depending on the value of the key_type parameter.
 *	The resource group and type are as given, but are currently meaningful
 *	only if the group is either URMgLiteral or URMgWidget. The type may
 *	by unspecified (URMtNul).
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	resource context containing widget record
 *	arg_ndx		the 0-based index of the argument in the arglist
 *	access		the access to the resource - URMaPublic or URMaPrivate
 *	group		the resource group, usually URMgLiteral or URMgWidget
 *	type		the literal type, from RGMrType... or URMwc...
 *			URMtNul is also legal
 *	key_type	the key type - URMrIndex or URMrRID
 *	index		index for URMaIndex literal
 *	resource_id	resource id for URMaRID literal
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		operation succeeded
 *	MrmBAD_CONTEXT		resource context invalid
 *	MrmBAD_WIDGET_REC	widget record not valid
 *	MrmNULL_DESC		arglist descriptor is null
 *	MrmOUT_OF_BOUNDS	arg_ndx out of bounds
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
RGMArgListDescPtr	argdesc ;	/* arglist desc in record */
RGMArgumentPtr		argptr ;	/* argument being set */
MrmOffset		offset ;	/* RGMResourceDesc offset */


/*
 * Validate record, arglist descriptor, and argument number.
 * Set the argument type.
 */
UrmCWR__ValidateContext (context_id, "UrmCWRSetArgResourceRef", NULL) ;
UrmCWR__BindArgPtrs
    (context_id, "UrmCWRSetArgResourceRef", arg_ndx, &argdesc, &argptr) ;

argptr->arg_val.rep_type = MrmRtypeResource ;

/*
 * Acquire a resource descriptor, and bind the value to it.
 */
result = UrmCWR__AppendResource
    (context_id, access, group, type, key_type, index, resource_id, &offset) ;
if ( result != MrmSUCCESS ) return result ;

UrmCWR__BindArgPtrs
    (context_id, "UrmCWRSetArgResourceRef", arg_ndx, &argdesc, &argptr) ;
argptr->arg_val.datum.offset = offset ;

/*
 * successfully added
 */
return MrmSUCCESS ;

}



Cardinal UrmCWRSetArgChar8Vec 
#ifndef _NO_PROTO
    (URMResourceContextPtr	context_id,
    Cardinal			arg_ndx,
    String			*stg_vec,
    MrmCount			num_stg)
#else
(context_id, arg_ndx, stg_vec, num_stg)
    URMResourceContextPtr	context_id ;
    Cardinal			arg_ndx ;
    String			*stg_vec ;
    MrmCount			num_stg ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine sets the value of an argument in the arglist to
 *	be a vector of ASCIZ strings.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	resource context containing widget record
 *	arg_ndx		the 0-based index of the argument in the arglist
 *	stg_vec		a vector of ASCIZ string pointers
 *	num_stg		the number of pointers in stg_vec
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		operation succeeded
 *	MrmBAD_CONTEXT		resource context invalid
 *	MrmBAD_WIDGET_REC	widget record not valid
 *	MrmNULL_DESC		arglist descriptor is null
 *	MrmOUT_OF_BOUNDS	arg_ndx out of bounds
 *	MrmVEC_TOO_BIG		vector size exceeds internal limit
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
RGMWidgetRecordPtr	widgetrec ;	/* widget record in context */
RGMArgListDescPtr	argdesc ;	/* arglist desc in record */
RGMArgumentPtr		argptr ;	/* argument being set */
RGMTextVectorPtr	vecptr ;	/* text vector in record */
MrmOffset		vecoffs ;	/* text vector offset */
MrmOffset		offset ;	/* current string offset */
MrmSize			vecsiz ;	/* # bytes for text vector */
int			ndx ;		/* loop index */


/*
 * Validate record, arglist descriptor, and argument number.
 */
UrmCWR__ValidateContext (context_id, "UrmCWRSetArgChar8Vec", NULL) ;
UrmCWR__BindArgPtrs
    (context_id, "UrmCWRSetArgChar8Vec", arg_ndx, &argdesc, &argptr) ;

/*
 * Validate vector - make sure it doesn't contain too many elements
 */
if ( num_stg > RGMListSizeMax )
    return Urm__UT_Error
        ("DwUrmCWRSetArgChar8Vec", "Vector too big",
        NULL, context_id, MrmVEC_TOO_BIG) ;

/*
 * Allocate the text vector. Then loop and append each string in
 * the input vector to the record, setting its offset in the
 * record vector. This list requires count+1 entries so it may be
 * overwritten as an in-memory list at runtime with a terminating NULL.
 * One item is already allocated, so num_stg additional entries are needed.
 */
vecsiz = sizeof(RGMTextVector) + (num_stg)*sizeof(RGMTextEntry) ;
result = UrmCWR__GuaranteeSpace (context_id, vecsiz, &vecoffs, (char **)&vecptr) ;
if ( result != MrmSUCCESS ) return result ;

UrmCWR__BindArgPtrs
    (context_id, "UrmCWRSetArgChar8Vec", arg_ndx, &argdesc, &argptr) ;
argptr->arg_val.rep_type = MrmRtypeChar8Vector ;
argptr->arg_val.datum.offset = vecoffs ;

vecptr->validation = URMTextVectorValid ;
vecptr->count = num_stg ;

for ( ndx=0 ; ndx<num_stg ; ndx++ )
    {
    result = UrmCWR__AppendString (context_id, stg_vec[ndx], &offset) ;
    if ( result != MrmSUCCESS ) return result ;
    widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer(context_id) ;
    UrmCWR__BindArgPtrs
        (context_id, "UrmCWRSetArgChar8Vec", arg_ndx, &argdesc, &argptr) ;
    vecptr = (RGMTextVectorPtr) ((char *)widgetrec + vecoffs) ;
    vecptr->item[ndx].text_item.rep_type = MrmRtypeChar8 ;
    vecptr->item[ndx].text_item.offset = offset ;
    }

/*
 * set terminating null
 */
vecptr->item[num_stg].pointer = NULL ;

/*
 * Successfully added
 */
return MrmSUCCESS ;

}



Cardinal UrmCWRSetArgCStringVec 
#ifndef _NO_PROTO
    (URMResourceContextPtr	context_id,
    Cardinal			arg_ndx,
    XmString			*cstg_vec,
    MrmCount			num_cstg)
#else
(context_id, arg_ndx, cstg_vec, num_cstg)
    URMResourceContextPtr	context_id ;
    Cardinal			arg_ndx ;

    XmString			*cstg_vec ;
    MrmCount			num_cstg ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine sets the value of an argument in the arglist to
 *	be a vector of compound strings
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	resource context containing widget record
 *	arg_ndx		the 0-based index of the argument in the arglist
 *	cstg_vec	a vector of compound string pointers
 *	num_cstg	the number of pointers in cstg_vec
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		operation succeeded
 *	MrmBAD_CONTEXT		resource context invalid
 *	MrmBAD_WIDGET_REC	widget record not valid
 *	MrmNULL_DESC		arglist descriptor is null
 *	MrmOUT_OF_BOUNDS	arg_ndx out of bounds
 *	MrmVEC_TOO_BIG		vector size exceeds internal limit
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
RGMWidgetRecordPtr	widgetrec ;	/* widget record in context */
RGMArgListDescPtr	argdesc ;	/* arglist desc in record */
RGMArgumentPtr		argptr ;	/* argument being set */
RGMTextVectorPtr	vecptr ;	/* text vector in record */
MrmOffset		vecoffs ;	/* text vector offset */
MrmOffset		offset ;	/* current compound string offset */
MrmSize			vecsiz ;	/* # bytes for text vector */
int			ndx ;		/* loop index */


/*
 * Validate record, arglist descriptor, and argument number.
 */
UrmCWR__ValidateContext (context_id, "UrmCWRSetArgCStringVec", NULL) ;
UrmCWR__BindArgPtrs
    (context_id, "UrmCWRSetArgCStringVec", arg_ndx, &argdesc, &argptr) ;

/*
 * Validate vector - make sure it doesn't contain too many elements
 */
if ( num_cstg > RGMListSizeMax )
    return Urm__UT_Error
        ("DwUrmCWRSetArgCStringVec", "Vector too big",
        NULL, context_id, MrmVEC_TOO_BIG) ;

/*
 * Allocate the text vector. Then loop and append each string in
 * the input vector to the record, setting its offset in the
 * record vector. This list requires count+1 entries so it may be
 * overwritten as an in-memory list at runtime with a terminating NULL.
 * One item is already allocated, so num_cstg additional entries are needed.
 */
vecsiz = sizeof(RGMTextVector) + (num_cstg)*sizeof(RGMTextEntry) ;
result = UrmCWR__GuaranteeSpace (context_id, vecsiz, &vecoffs, (char **)&vecptr) ;
if ( result != MrmSUCCESS ) return result ;

UrmCWR__BindArgPtrs
    (context_id, "UrmCWRSetArgCStringVec", arg_ndx, &argdesc, &argptr) ;
argptr->arg_val.rep_type = MrmRtypeCStringVector ;
argptr->arg_val.datum.offset = vecoffs ;

vecptr->validation = URMTextVectorValid ;
vecptr->count = num_cstg ;

for ( ndx=0 ; ndx<num_cstg ; ndx++ )
    {
    result = UrmCWR__AppendCString (context_id, cstg_vec[ndx], &offset) ;
    if ( result != MrmSUCCESS ) return result ;
    widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer(context_id) ;
    UrmCWR__BindArgPtrs
        (context_id, "UrmCWRSetArgCStringVec", arg_ndx, &argdesc, &argptr) ;
    vecptr = (RGMTextVectorPtr) ((char *)widgetrec + vecoffs) ;
    vecptr->item[ndx].text_item.rep_type = MrmRtypeCString ;
    vecptr->item[ndx].text_item.offset = offset ;
    }

/*
 * set terminating null
 */
vecptr->item[num_cstg].pointer = NULL ;

/*
 * Successfully added
 */
return MrmSUCCESS ;

}



Cardinal UrmCWRSetArgCallback (context_id, arg_ndx, nitems, cb_offs_return)
    URMResourceContextPtr	context_id ;
    Cardinal			arg_ndx ;
    Cardinal			nitems ;
    MrmOffset			*cb_offs_return ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmCWRSetArgCallback creates a callback descriptor for a
 *	MrmRtypeCallback argument. The callback descriptor is created with a
 *	vector of callback item descriptors whose length is nitems. The
 *	callback items may then be filled in with UrmCWRSetCallbackItem
 *	and UrmCWRSetCallbackItemRes. The offset of the new callback
 *	descriptor is returned for use in the set callback item routines.
 *
 *  FORMAL PARAMETERS:
 *
 *	context		resource context containing widget record
 *	arg_ndx		the 0-based index of the argument in the arglist
 *	nitems		number of items to appear in the callback list
 *	cb_offs_return	to return offset of RGMCallbackDesc struct created in
 *			widget record
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		operation succeeded
 *	MrmBAD_CONTEXT		resource context invalid
 *	MrmBAD_WIDGET_REC	widget record not valid
 *	MrmNULL_DESC		arglist descriptor is null
 *	MrmOUT_OF_BOUNDS	arg_ndx out of bounds
 *	MrmTOO_MANY		number of items exceeds internal limit
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
RGMArgListDescPtr	argdesc ;	/* arglist desc in record */
RGMArgumentPtr		argptr ;	/* argument being set */
MrmSize			descsiz ;	/* descriptor size */
MrmOffset		offset ;	/* RGMCallbackDesc offset */
RGMCallbackDescPtr	cbdesc ;	/* resource descriptor */
int			ndx ;		/* loop index */


/*
 * Validate record, arglist descriptor, and argument number.
 * Set the argument type.
 */
UrmCWR__ValidateContext
    (context_id, "UrmCWRSetArgCallback", URMWidgetRecordValid) ;
UrmCWR__BindArgPtrs
    (context_id, "UrmCWRSetArgCallback", arg_ndx, &argdesc, &argptr) ;

argptr->arg_val.rep_type = MrmRtypeCallback ;

/*
 * Confirm that the number of items is reasonable, then size and
 * allocate a callback descriptor. Initialize it. Note an extra
 * item is added for a terminating NULL for runtime use as an
 * in-memory list.
 */
if ( nitems > RGMListSizeMax )
    return Urm__UT_Error
        ("UrmCWRSetArgCallback", "Too many items",
        NULL, context_id, MrmTOO_MANY) ;

descsiz = sizeof(RGMCallbackDesc) + (nitems)*sizeof(RGMCallbackItem) ;
result = UrmCWR__GuaranteeSpace (context_id, descsiz, &offset, (char **)&cbdesc) ;
if ( result != MrmSUCCESS ) return result ;

UrmCWR__BindArgPtrs
    (context_id, "UrmCWRSetArgCallback", arg_ndx, &argdesc, &argptr) ;
argptr->arg_val.datum.offset = offset ;

cbdesc->validation = URMCallbackDescriptorValid ;
cbdesc->count = nitems ;
cbdesc->unres_ref_count = 0 ;
for ( ndx=0 ; ndx<nitems ; ndx++ )
    {
    cbdesc->item[ndx].cb_item.routine = 0 ;
    cbdesc->item[ndx].cb_item.rep_type = 0 ;
    cbdesc->item[ndx].cb_item.datum.ival = NULL ;
    }

/*
 * Set terminating NULL
 */

cbdesc->item[nitems].runtime.callback.callback = NULL ;
cbdesc->item[nitems].runtime.callback.closure = 0 ;

/*
 * successfully added
 */
*cb_offs_return = offset ;
return MrmSUCCESS ;

}



Cardinal UrmCWRSetCallbackItem
#ifndef _NO_PROTO
    (URMResourceContextPtr	context_id,
    MrmOffset			cb_offs,
    Cardinal			item_ndx,
    String			routine,
    MrmCode			type,
    unsigned			itm_val)
#else
        (context_id, cb_offs, item_ndx, routine, type, itm_val)
    URMResourceContextPtr	context_id ;
    MrmOffset			cb_offs ;
    Cardinal			item_ndx ;
    String			routine ;
    MrmCode			type ;
    unsigned			itm_val ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmCWRSetCallbackItem sets the routine name for the callback item
 *	to the given routine, and sets the tag value to the immedidate value
 *	specified by the type and itm_val parameters. The usage of the type
 *	and itm_val parameters is identical to UrmCWRSetArgValue.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	resource context containing widget record
 *	cb_offs		offset to the callback descriptor in the widget
 *			record (NOT a memory pointer, in case a new buffer
 *			is allocated).
 *	item_ndx	the 0-based index of the item in the callback list
 *	routine		routine name associated with this item
 *	type		the representation type for the tag value, from RGMrType...
 *	itm_val		a longword which will be CAST and used as required to
 *			access the value to be stored (copied) into the widget
 *			record for this tag value.
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		operation succeeded
 *	URMBadcontext		resource context invalid
 *	MrmBAD_WIDGET_REC	widget record not valid
 *	MrmBAD_CALLBACK		invalid callback descriptor
 *	MrmOUT_OF_BOUNDS	item_ndx out of bounds
 *	MrmBAD_ARG_TYPE		unknown or unhandled representation type
 *	MrmNULL_ROUTINE		routine name is empty
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
RGMCallbackDescPtr	cbdesc ;	/* callback descriptor */
RGMCallbackItemPtr	itmptr ;	/* callback item in list */
MrmOffset		offset ;	/* appended data offset */
double			*dblptr ;	/* pointer to double constant */


/*
 * Validate context and bind pointers to callback descriptor and item
 */
UrmCWR__ValidateContext
    (context_id, "UrmCWRSetCallbackItem", URMWidgetRecordValid) ;
UrmCWR__BindCallbackPtrs (context_id, "UrmCWRSetCallbackItem",
    cb_offs, item_ndx, &cbdesc, &itmptr) ;

/*
 * Validate the routine (must be non-empty)
 */
if ( strlen(routine) <= 0 )
    return Urm__UT_Error
        ("UrmCWRSetCallbackItem", "Empty routine name",
        NULL, context_id, MrmNULL_ROUTINE) ;

/*
 * Append the routine string and set the item
 */
result = UrmCWR__AppendString (context_id, routine, &offset) ;
if ( result != MrmSUCCESS ) return result ;

UrmCWR__BindCallbackPtrs (context_id, "UrmCWRSetCallbackItem",
    cb_offs, item_ndx, &cbdesc, &itmptr) ;
itmptr->cb_item.routine = offset ;

/*
 * Set the tag to the literal value as in SetArgValue
 */
itmptr->cb_item.rep_type = type ;
switch ( type )
    {
    case MrmRtypeInteger:
    case MrmRtypeBoolean:
    case MrmRtypeSingleFloat:
        itmptr->cb_item.datum.ival = (int) itm_val ;
        return MrmSUCCESS ;
    case MrmRtypeChar8:
    case MrmRtypeAddrName:
    case MrmRtypeTransTable:
    case MrmRtypeKeysym:
        result = UrmCWR__AppendString
            (context_id, (char *)itm_val, &offset) ;
        if ( result != MrmSUCCESS ) return result ;
        UrmCWR__BindCallbackPtrs (context_id, "UrmCWRSetCallbackItem",
            cb_offs, item_ndx, &cbdesc, &itmptr) ;
        itmptr->cb_item.datum.offset = offset ;
        return MrmSUCCESS ;
    case MrmRtypeChar8Vector:
        return Urm__UT_Error
            ("UrmCWRSetCallbackItem", "Char8Vector not yet implemented",
            NULL, context_id, MrmBAD_ARG_TYPE) ;
    case MrmRtypeCString:
        result = UrmCWR__AppendCString
            (context_id, (XmString)itm_val, &offset) ;
        if ( result != MrmSUCCESS ) return result ;
        UrmCWR__BindCallbackPtrs (context_id, "UrmCWRSetCallbackItem",
            cb_offs, item_ndx, &cbdesc, &itmptr) ;
        itmptr->cb_item.datum.offset = offset ;
        return MrmSUCCESS ;
    case MrmRtypeWideCharacter:
        result = UrmCWR__AppendWcharString
            (context_id, (wchar_t *)itm_val, &offset) ;
        if ( result != MrmSUCCESS ) return result ;
        UrmCWR__BindCallbackPtrs (context_id, "UrmCWRSetCallbackItem",
            cb_offs, item_ndx, &cbdesc, &itmptr) ;
        itmptr->cb_item.datum.offset = offset ;
        return MrmSUCCESS ;
    case MrmRtypeCStringVector:
        return Urm__UT_Error
            ("UrmCWRSetCallbackItem", "CStringVector not yet implemented",
            NULL, context_id, MrmBAD_ARG_TYPE) ;
    case MrmRtypeFloat:
        result = UrmCWR__GuaranteeSpace
            (context_id, sizeof(double), &offset, (char **)&dblptr) ;
        if ( result != MrmSUCCESS ) return result ;
        *dblptr = *((double *) itm_val) ;
        UrmCWR__BindCallbackPtrs (context_id, "UrmCWRSetCallbackItem",
            cb_offs, item_ndx, &cbdesc, &itmptr) ;
        itmptr->cb_item.datum.offset = offset ;
        return MrmSUCCESS ;
    case MrmRtypeNull:
        itmptr->cb_item.datum.ival = (int) NULL ;
        return MrmSUCCESS ;
    default:
        return Urm__UT_Error
            ("UrmCWRSetCallbackItem", "Invalid or unhandled type",
            NULL, context_id, MrmBAD_ARG_TYPE) ;
    }

}



Cardinal UrmCWRSetCallbackItemRes
#ifndef _NO_PROTO
    (URMResourceContextPtr	context_id,
    MrmOffset			cb_offs,
    Cardinal			item_ndx,
    String			routine,
    MrmGroup			group,
    MrmCode			access,
    MrmCode			type,
    MrmCode			key_type,
    String			index,
    MrmResource_id		resource_id)
#else
        (context_id, cb_offs, item_ndx, routine, group, access, type,
            key_type, index, resource_id)
    URMResourceContextPtr	context_id ;
    MrmOffset			cb_offs ;
    Cardinal			item_ndx ;
    String			routine ;
    MrmGroup			group ;
    MrmCode			access ;
    MrmCode			type ;
    MrmCode			key_type ;
    String			index ;
    MrmResource_id		resource_id ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmCWRSetCallbackItemRes sets the callback item routine name for
 *	the callback item to the given routine, and sets the tag value to
 *	be a resource reference. The interpretation of the resource
 *	specification parameters is identical to UrmCWRSetArgResourceRef.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	resource context containing widget record
 *	cb_offs		offset to the callback descriptor in the widget record
 *			(NOTa memory pointer, in case a new buffer is
 *			allocated).
 *	item_ndx	the 0-based index of the item in the callback list
 *	routine		name of the routine for the callback item
 *	access		the access to the resource - URMaPublic or URMaPrivate
 *	group		the resource group. Only URMgLiteral makes sense,
 *			but the parameter is available for completeness
 *	type		the literal type, from RGMrType... (or URMtNul)
 *	key_type	the key type - URMrIndex or URMrRID
 *	index		index for URMaIndex literal
 *	resource_id	resource id for URMaRID literal
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		operation succeeded
 *	MrmBAD_CONTEXT		resource context invalid
 *	MrmBAD_WIDGET_REC	widget record not valid
 *	MrmBAD_CALLBACK		invalid callback descriptor
 *	MrmOUT_OF_BOUNDS	item_ndx out of bounds
 *	DMNullIndex		empty index string
 *	MrmNULL_ROUTINE		routine name is empty
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
RGMCallbackDescPtr	cbdesc ;	/* callback descriptor */
RGMCallbackItemPtr	itmptr ;	/* callback item in list */
MrmOffset		offset ;	/* appended data offset */


/*
 * Validate context and bind pointers to callback descriptor and item
 */
UrmCWR__ValidateContext
    (context_id, "UrmCWRSetCallbackItemRes", URMWidgetRecordValid) ;
UrmCWR__BindCallbackPtrs (context_id, "UrmCWRSetCallbackItemRes",
    cb_offs, item_ndx, &cbdesc, &itmptr) ;

/*
 * Validate the routine (must be non-empty)
 */
if ( strlen(routine) <= 0 )
    return Urm__UT_Error
        ("UrmCWRSetCallbackItemRes", "Empty routine name",
        NULL, context_id, MrmNULL_ROUTINE) ;

/*
 * Append the routine string and set the item
 */
result = UrmCWR__AppendString (context_id, routine, &offset) ;
if ( result != MrmSUCCESS ) return result ;

UrmCWR__BindCallbackPtrs (context_id, "UrmCWRSetCallbackItemRes",
    cb_offs, item_ndx, &cbdesc, &itmptr) ;
itmptr->cb_item.routine = offset ;

/*
 * Acquire and set a resource descriptor
 */
result = UrmCWR__AppendResource
    (context_id, access, group, type, key_type, index, resource_id, &offset) ;
if ( result != MrmSUCCESS ) return result ;

UrmCWR__BindCallbackPtrs (context_id, "UrmCWRSetCallbackItemRes",
    cb_offs, item_ndx, &cbdesc, &itmptr) ;
itmptr->cb_item.rep_type = MrmRtypeResource ;
itmptr->cb_item.datum.offset = offset ;

/*
 * Successfully added
 */
return MrmSUCCESS ;

}



Cardinal UrmCWRSetExtraArgs (context_id, nextra)
    URMResourceContextPtr	context_id ;
    Cardinal			nextra ;
/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine sets the extra args count in the arglist descriptor
 *	to the given value
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	resource context containing widget record
 *	nextra		# of extra args which should be allocated by URM
 *			at runtime to deal with args URM creates which
 *			are related to those in the arglist (already
 *			set to 0).
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		operation succeeded
 *	MrmBAD_CONTEXT		resource context invalid
 *	MrmBAD_WIDGET_REC	widget record not valid
 *	MrmNULL_DESC		arglist descriptor is null
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
RGMArgListDescPtr	argdesc ;	/* arglist desc in record */
RGMArgumentPtr		argptr ;	/* argument being set */


/*
 * Validate record, arglist descriptor. Bind usual pointers, although
 * argptr not used. Set the extra args field.
 */
UrmCWR__ValidateContext (context_id, "UrmCWRSetExtraArgs", NULL) ;
UrmCWR__BindArgPtrs
    (context_id, "UrmCWRSetExtraArgs", 0, &argdesc, &argptr) ;

argdesc->extra = nextra ;
return MrmSUCCESS ;

}



Cardinal UrmCWRInitChildren (context_id, nchildren)
    URMResourceContextPtr	context_id ;
    Cardinal			nchildren ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmCWRInitChildren appends an empty RGMChildrenDesc struct to the
 *	record, preparing it for setting children. The descriptor is
 *	allocated a vector of child descriptors whose length is given by the
 *	nchildren parameter. Each child is then set with UrmCWRSetChild.
 *
 *	The children descriptor's access field is set from the record's
 *	access field.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	resource context containing widget record
 *	nchildren	number of children in list
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		operation succeeded
 *	MrmBAD_CONTEXT		resource context invalid
 *	MrmBAD_WIDGET_REC	widget record not valid
 *	MrmTOO_MANY		number of children exceeded internal limit
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
RGMWidgetRecordPtr	widgetrec ;	/* widget record in context */
MrmSize			descsiz ;	/* descriptor size */
MrmOffset		offset ;	/* children descriptor offset */
RGMChildrenDescPtr	listdesc ;	/* pointer to desc in record */
int			ndx ;		/* loop index */

/*
 * Validate record
 */
UrmCWR__ValidateContext
    (context_id, "UrmCWRInitChildren", URMWidgetRecordValid) ;

/*
 * Error check that the number of children is reasonable
 */
if ( nchildren > RGMListSizeMax )
    return Urm__UT_Error
        ("UrmCWRInitChildren", "Too many children",
        NULL, context_id, MrmTOO_MANY) ;

/*
 * compute the size required for the descriptor and the child list,
 * and acquire the memory in the record. Note there is one child
 * preallocated in the descriptor.
 */
descsiz = sizeof(RGMChildrenDesc) + (nchildren-1)*sizeof(RGMChildDesc) ;
result = UrmCWR__GuaranteeSpace (context_id, descsiz, &offset, (char **)&listdesc) ;
if ( result != MrmSUCCESS ) return result ;
widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer (context_id) ;
widgetrec->children_offs = offset ;

/*
 * initialize the descriptor and all the arguments
 */
listdesc->count = nchildren ;
listdesc->annex1 = NULL ;
for ( ndx=0 ; ndx<nchildren ; ndx++ )
    {
    listdesc->child[ndx].manage = 0 ;
    listdesc->child[ndx].access = 0 ;
    listdesc->child[ndx].type = 0 ;
    listdesc->child[ndx].annex1 = NULL ;
    listdesc->child[ndx].key.id = NULL ;
    }

/*
 * successfully added
 */
return MrmSUCCESS ;

}



Cardinal UrmCWRSetChild
#ifndef _NO_PROTO
    (URMResourceContextPtr	context_id,
    Cardinal			child_ndx,
    Boolean			manage,
    MrmCode			access,
    MrmCode			key_type,
    String			index,
    MrmResource_id		resource_id)
#else
        (context_id, child_ndx, manage, access, key_type, index, resource_id)
    URMResourceContextPtr	context_id ;
    Cardinal			child_ndx ;
    Boolean			manage ;
    MrmCode			access ;
    MrmCode			key_type ;
    String			index ;
    MrmResource_id		resource_id ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmCWRSetChild sets a child descriptor in the children list for
 *	the widget record. Only one of the index or resource_id parameters
 *	is used, depending on the value of the key_type parameter.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	resource context containing widget record
 *	child_ndx	index of child descriptor in children list descriptor
 *	manage		true/false indicating child is managed with parent at
 *			initialization time
 *	access		the access to the child - URMaPublic or URMaPrivate
 *	key_type	the key type - URMrIndex or URMrRID
 *	index		index for URMrIndex child
 *	resource_id	resource id for URMaRID child
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		operation succeeded
 *	MrmBAD_CONTEXT		resource context invalid
 *	MrmBAD_WIDGET_REC	widget record not valid
 *	MrmNULL_DESC		children descriptor is null
 *	MrmOUT_OF_BOUNDS	child_ndx was out of bounds
 *	MrmNULL_INDEX		empty index string for URMrIndex
 *	MrmBAD_KEY_TYPE		key_type was an unknown type
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
RGMWidgetRecordPtr	widgetrec ;	/* widget record in context */
MrmOffset		offset ;	/* child index offset */
RGMChildrenDescPtr	listdesc ;	/* pointer to desc in record */
RGMChildDescPtr		childptr ;	/* child in list */

/*
 * Validate record
 */
UrmCWR__ValidateContext
    (context_id, "UrmCWRSetChild", URMWidgetRecordValid) ;

widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer(context_id) ;
if ( widgetrec->children_offs == NULL )
    return Urm__UT_Error
        ("UrmCWRSetChild", "Null children list descriptor",
        NULL, context_id, MrmNULL_DESC) ;
listdesc = (RGMChildrenDescPtr) ((char *)widgetrec + widgetrec->children_offs) ;

if ( child_ndx >= listdesc->count )
    return Urm__UT_Error
        ("UrmCWRSetChild", "Child index out of bounds",
        NULL, context_id, MrmOUT_OF_BOUNDS) ;
childptr = &listdesc->child[child_ndx] ;

/*
 * set child descriptor
 */
childptr->manage = manage ;
childptr->access = access ;
childptr->type = key_type ;
switch ( childptr->type )
    {
    case URMrIndex:
        if ( strlen(index) <= 0 )
            return Urm__UT_Error
                ("UrmCWRSetChild", "Null index",
                NULL, context_id, MrmNULL_INDEX) ;
        result = UrmCWR__AppendString (context_id, index, &offset) ;
        if ( result != MrmSUCCESS ) return result ;
        widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer (context_id) ;
        listdesc =
            (RGMChildrenDescPtr) ((char *)widgetrec + widgetrec->children_offs) ;
        childptr = &listdesc->child[child_ndx] ;
        childptr->key.index_offs = offset ;
        return MrmSUCCESS ;
    case URMrRID:
        childptr->key.id = resource_id ;
        return MrmSUCCESS ;
    default:
        return Urm__UT_Error
            ("UrmCWRSetChild", "Invalid key type",
            NULL, context_id, MrmBAD_KEY_TYPE) ;
    }

}



Cardinal UrmCWRSetComment (context_id, comment)
    URMResourceContextPtr	context_id ;
    String			comment ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmSetComment appends a comment to the widget record.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	resource context containing widget record
 *	comment		comment string for this widget
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		operation succeeded
 *	MrmBAD_CONTEXT		resource context invalid
 *	MrmBAD_WIDGET_REC	widget record not valid
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
RGMWidgetRecordPtr	widgetrec ;	/* widget record in context */
MrmOffset		offset ;	/* comment offset */


/*
 * Validate record
 */
UrmCWR__ValidateContext
    (context_id, "UrmCWRSetComment", URMWidgetRecordValid) ;

/*
 * Append the comment if it is non-empty
 */
if ( strlen(comment) <= 0 )
    {
    widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer(context_id) ;
    widgetrec->comment_offs = 0 ;
    return MrmSUCCESS ;
    }

result = UrmCWR__AppendString (context_id, comment, &offset) ;
if ( result != MrmSUCCESS ) return result ;
widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer (context_id) ;
widgetrec->comment_offs = offset ;

/*
 * successfully added
 */
return MrmSUCCESS ;

}



Cardinal UrmCWRSetCreationCallback (context_id, nitems, cb_offs_return)
    URMResourceContextPtr	context_id ;
    Cardinal			nitems ;
    MrmOffset			*cb_offs_return ;

/*
 *++
 *
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmCWRSetCreationCallback initialize a callback descriptor for the
 *	URM creation callback for this widget. It is similar to
 *	UrmCWRSetArgCallback in returning the offset of a RGMCallbackDesc
 *	struct which may then be completed with calls to
 *	UrmCWRSetCallbackItem or UrmCWRSetCallbackItemRes.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	resource context containing widget record
 *	nitems		number of items to appear in the callback list
 *	cb_offs_return	to return offset of RGMCallbackDesc struct created in
 *			widget record
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		operation succeeded
 *	MrmBAD_CONTEXT		resource context invalid
 *	MrmBAD_WIDGET_REC	widget record not valid
 *	MrmTOO_MANY		number of items exceeds internal limit
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
RGMWidgetRecordPtr	widgetrec ;	/* widget record in context */
MrmSize			descsiz ;	/* descriptor size */
MrmOffset		offset ;	/* RGMCallbackDesc offset */
RGMCallbackDescPtr	cbdesc ;	/* resource descriptor */
int			ndx ;		/* loop index */


/*
 * Validate record, arglist descriptor, and argument number.
 * Set the argument type.
 */
UrmCWR__ValidateContext
    (context_id, "UrmCWRSetCreationCallback", URMWidgetRecordValid) ;

/*
 * Confirm that the number of items is reasonable, then size and
 * allocate a callback descriptor. Initialize it. As in SetArgCallbac,
 * an extra item is added for runtime use.
 */
if ( nitems > RGMListSizeMax )
    return Urm__UT_Error
        ("UrmCWRSetCreationCallback", "Too many items",
        NULL, context_id, MrmTOO_MANY) ;

descsiz = sizeof(RGMCallbackDesc) + (nitems)*sizeof(RGMCallbackItem) ;
result = UrmCWR__GuaranteeSpace (context_id, descsiz, &offset, (char **)&cbdesc) ;
if ( result != MrmSUCCESS ) return result ;

widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer (context_id) ;
widgetrec->creation_offs = offset ;

cbdesc->validation = URMCallbackDescriptorValid ;
cbdesc->count = nitems ;
cbdesc->unres_ref_count = 0 ;
for ( ndx=0 ; ndx<nitems ; ndx++ )
    {
    cbdesc->item[ndx].cb_item.routine = 0 ;
    cbdesc->item[ndx].cb_item.rep_type = 0 ;
    cbdesc->item[ndx].cb_item.datum.ival = NULL ;
    }

/*
 * Set terminating NULL
 */

cbdesc->item[nitems].runtime.callback.callback = NULL ;
cbdesc->item[nitems].runtime.callback.closure = 0 ;

/*
 * successfully added
 */
*cb_offs_return = offset ;
return MrmSUCCESS ;

}



Cardinal UrmCWR__AppendString (context_id, stg, offset)
    URMResourceContextPtr	context_id ;
    String			stg ;
    MrmOffset			*offset ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine appends a string into the widget record at the
 *	next available location in the record. It resizes the context
 *	if necessary.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	context containing the widget record
 *	stg		the string to append. Nothing is done if empty
 *	offset		To return offset in record. 0 is returned if
 *			the string is empty.
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	See UrmResizeResourceContext
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
int			len ;		/* string length */
String			stgadr ;	/* destination address in record */


/*
 * Offset zero for NULL pointer
 */
if ( stg == NULL )
    {
    *offset = 0;
    return MrmSUCCESS ;
    }

/*
 * Guarantee space and copy in string
 */
len = strlen (stg) + 1 ;
result = UrmCWR__GuaranteeSpace (context_id, len, offset, &stgadr) ;
if ( result != MrmSUCCESS ) return result ;
memcpy (stgadr, stg, len) ;
return MrmSUCCESS ;

}



Cardinal UrmCWR__AppendCString (context_id, cstg, offset)
    URMResourceContextPtr	context_id ;

    XmString			cstg ;
    MrmOffset			*offset ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine appends a compound string into the widget record at the
 *	next available location in the record. It resizes the context
 *	if necessary.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	context containing the widget record
 *	cstg		the compound string to append. Nothing is done if empty
 *	offset		To return offset in record. 0 is returned if
 *			the string is empty.
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	See UrmResizeResourceContext
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
int			len ;		/* length of string */
char			*cstgadr ;	/* destination address in record */


/*
 * Offset zero for empty string or NULL pointer
 */
if ( cstg == NULL )
    {
    *offset = 0;
    return MrmSUCCESS ;
    }


len = XmStringLength ( cstg) ;

if ( len <= 0 )
    {
    *offset = 0;
    return MrmSUCCESS ;
    }

/*
 * Guarantee space and copy in string
 */
result = UrmCWR__GuaranteeSpace (context_id, len, offset, &cstgadr) ;
if ( result != MrmSUCCESS ) return result ;
memcpy (cstgadr, (char *)cstg, len) ;
return MrmSUCCESS ;

}



Cardinal UrmCWR__AppendWcharString (context_id, wcs, offset)
    URMResourceContextPtr	context_id ;
    wchar_t			*wcs ;
    MrmOffset			*offset ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine appends a wide character string into the widget record at
 *	the next available location in the record. It resizes the context
 *	if necessary.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	context containing the widget record
 *	wcs		the wide character string to append. 
 *			Nothing is done if empty.
 *	offset		To return offset in record. 0 is returned if
 *			the string is empty.
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	See UrmResizeResourceContext
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
MrmSize			len ;		/* string length */
wchar_t			*wcsadr ;	/* destination address in record */
int			cnt;

/*
 * Offset zero for NULL pointer
 */
if ( wcs == NULL )
    {
    *offset = 0;
    return MrmSUCCESS ;
    }

/*
 * Guarantee space and copy in string
 */
for (cnt = 0; ; cnt++) if (wcs[cnt] == 0) break;
len = (cnt+1) * sizeof(wchar_t);
result = UrmCWR__GuaranteeSpace (context_id, len, offset, (char **)&wcsadr) ;
if ( result != MrmSUCCESS ) return result ;
memcpy (wcsadr, wcs, len) ;
return MrmSUCCESS ;
}


Cardinal UrmCWR__GuaranteeSpace 
#ifndef _NO_PROTO
    (URMResourceContextPtr	context_id,
    MrmSize			delta,
    MrmOffset			*offset,
    char			**addr)
#else
(context_id, delta, offset, addr)
    URMResourceContextPtr	context_id ;
    MrmSize			delta ;
    MrmOffset			*offset ;
    char			**addr ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine guarantees that there is enough space in the
 *	widget record in the context for an append operation. It
 *	returns the address in the record of the available space. It
 *	also resets the widget record size to include the delta space.
 *	It guarantees that all space so granted is on a fullword boundary.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	context containing the widget record
 *	delta		the number of additional bytes needed for the append
 *	offset		to return the offset in the record
 *	addr		to return the address in the record
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
Cardinal		result ;	/* function results */
RGMWidgetRecordPtr	widgetrec ;	/* widget record in context */

widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer(context_id) ;
if ( ! UrmWRValid(widgetrec) )
    return Urm__UT_Error ("UrmCWR__GuaranteeSpace",
        "Invalid widget record", NULL, context_id, MrmBAD_RECORD) ;

delta = _FULLWORD (delta) ;
result = UrmResizeResourceContext (context_id, widgetrec->size+delta) ;
if ( result != MrmSUCCESS ) return result ;

widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer(context_id) ;
*offset = widgetrec->size ;
*addr = (char *) widgetrec+widgetrec->size ;
widgetrec->size += delta ;
UrmRCSetSize (context_id, widgetrec->size) ;

return MrmSUCCESS ;

}



Cardinal UrmCWR__AppendResource
#ifndef _NO_PROTO
    (URMResourceContextPtr	context_id,
    MrmCode			access,
    MrmCode			group,
    MrmCode			type,
    MrmCode			key_type,
    String			index,
    MrmResource_id		resource_id,
    MrmOffset			*offset)
#else
        (context_id, access, group, type, key_type, index, resource_id, offset)
    URMResourceContextPtr	context_id ;
    MrmCode			access ;
    MrmCode			group ;
    MrmCode			type ;
    MrmCode			key_type ;
    String			index ;
    MrmResource_id		resource_id ;
    MrmOffset			*offset ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	UrmCCWR__AppendResource creates a resource reference and appends
 *	it to the record. Only one of the index or resource_id
 *	parameters is used, depending on the value of the key_type parameter.
 *	The only currently meaningful groups are URMgLiteral and URMgWidget.
 *	The resource type is taken from the type parameter (which may be
 *	URMtNul).
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	resource context containing widget record
 *	access		the access to the resource - URMaPublic or URMaPrivate
 *	group		resource group, usually URMgLiteral or URMgWidget
 *	type		conversion type for argument values, from RGMrType...
 *	key_type	the key type - URMrIndex or URMrRID
 *	index		index for URMrIndex literal
 *	resource_id	resource id for URMaRID literal
 *	offset		to return offset of descriptor
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		operation succeeded
 *	MrmBAD_CONTEXT		resource context invalid
 *	MrmBAD_WIDGET_REC	widget record not valid
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
MrmSize			descsiz ;	/* descriptor size */
RGMResourceDescPtr	resdesc ;	/* resource descriptor */


/*
 * Acquire and set a resource descriptor. If an RID reference, no extra is
 * needed. If an index, extra space for the string is required. Since
 * one character is allocated already, no extra is needed for the NUL.
 */
switch ( key_type )
    {
    case URMrIndex:
        if ( strlen(index) <= 0 )
            return Urm__UT_Error
                ("UrmCWR__AppendResource", "Null index",
                NULL, context_id, MrmNULL_INDEX) ;
        descsiz = sizeof(RGMResourceDesc) + strlen(index) ;
        result = UrmCWR__GuaranteeSpace
            (context_id, descsiz, offset, (char **)&resdesc) ;
        if ( result != MrmSUCCESS ) return result ;

        resdesc->size = descsiz ;
        resdesc->access = access ;
        resdesc->type = URMrIndex ;
        resdesc->res_group = group ;
        resdesc->cvt_type = type ;
        resdesc->annex1 = NULL ;
        strcpy (resdesc->key.index, index) ;
        return MrmSUCCESS ;
    case URMrRID:
        descsiz = sizeof(RGMResourceDesc) ;
        result = UrmCWR__GuaranteeSpace
            (context_id, descsiz, offset, (char **)&resdesc) ;
        if ( result != MrmSUCCESS ) return result ;
        resdesc->size = descsiz ;
        resdesc->access = access ;
        resdesc->type = URMrRID ;
        resdesc->res_group = group ;
        resdesc->cvt_type = type ;
        resdesc->annex1 = NULL ;
        resdesc->key.id = resource_id ;
        return MrmSUCCESS ;
    default:
        return Urm__UT_Error
            ("UrmCWR__AppendResource", "Invalid key type",
            NULL, context_id, MrmBAD_KEY_TYPE) ;
    }

}



Cardinal UrmCWR__ValidateContext (context_id, routine, validation)
    URMResourceContextPtr	context_id ;
    String			routine ;
    unsigned			validation ;

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine checks that the context is valid and contains
 *	a valid widget record. It signals and returns an error
 *	otherwise.
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	resource context containing record
 *	validation	URMWidgetRecordValid if record must be a valid
 *			widget record.
 *	routine		Name of calling routine for error signal.
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		Validation succeeded
 *	MrmBAD_CONTEXT		Invalid resource context
 *	MrmBAD_WIDGET_REC	Invalid widget record in context
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
RGMWidgetRecordPtr	widgetrec ;	/* widget record in context */


if ( ! UrmRCValid(context_id) )
    return Urm__UT_Error (routine, "Invalid resource context",
        NULL, NULL, MrmBAD_CONTEXT) ;
widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer(context_id) ;
if ( ! UrmWRValid(widgetrec) )
    return Urm__UT_Error (routine, "Invalid widget record",
        NULL, context_id, MrmBAD_WIDGET_REC) ;
return MrmSUCCESS ;

}



Cardinal UrmCWR__BindArgPtrs (context_id, routine, argndx, descptr, argptr)
    URMResourceContextPtr	context_id ;
    String			routine ;
    Cardinal			argndx ;
    RGMArgListDescPtr		*descptr ;
    RGMArgumentPtr		*argptr ;
/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine bind an arglist descriptor pointer and an argument pointer
 *	to the arglist descriptor and designated argument item in a
 *	widget  record. It also performs the following validations:
 *		non-null arglist descriptor
 *		argument index in bounds
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	resource context containing record
 *	routine		name of calling routine for error signal
 *	argndx		0-based argument index in list
 *	descptr		to return pointer to arglist descriptor
 *	argptr		to return pointer to designated argument item
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		operation succeeded
 *	MrmNULL_DESC		arglist descriptor is null
 *	MrmOUT_OF_BOUNDS	argument index out of bounds
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
RGMWidgetRecordPtr	widgetrec ;	/* widget record in context */


/*
 * Pick up a widget  record, and set descriptor pointer.
 */
widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer(context_id) ;
if ( UrmWRValid(widgetrec) )
    *descptr = (RGMArgListDescPtr) ((char *)widgetrec+widgetrec->arglist_offs) ;
else
    return Urm__UT_Error (routine, "Invalid widget record",
        NULL, context_id, MrmBAD_RECORD) ;

/*
 * Validate argument index and set pointer
 */
if ( argndx >= (*descptr)->count )
    return Urm__UT_Error (routine, "Arg index out of bounds",
        NULL, context_id, MrmOUT_OF_BOUNDS) ;
*argptr = &(*descptr)->args[argndx] ;

return MrmSUCCESS ;

}



Cardinal UrmCWR__BindCallbackPtrs
#ifndef _NO_PROTO
    (URMResourceContextPtr	context_id,
    String			routine,
    MrmOffset			descoffs,
    Cardinal			itemndx,
    RGMCallbackDescPtr		*descptr,
    RGMCallbackItemPtr		*itmptr)
#else
        (context_id, routine, descoffs, itemndx, descptr, itmptr)
    URMResourceContextPtr	context_id ;
    String			routine ;
    MrmOffset			descoffs ;
    Cardinal			itemndx ;
    RGMCallbackDescPtr		*descptr ;
    RGMCallbackItemPtr		*itmptr ;
#endif

/*
 *++
 *
 *  PROCEDURE DESCRIPTION:
 *
 *	This routine binds pointers to a callback descriptor in a widget
 *	record, and a designated item in the callback list. It also
 *	performs the following validations:
 *		valid callback descriptor
 *		item index in bounds
 *
 *  FORMAL PARAMETERS:
 *
 *	context_id	context containing widget record
 *	routine		name of calling routine for error signal
 *	descoffs	offset of descriptor in record
 *	itemndx		0-based index of item in callback list
 *	descptr		to return callback descriptor pointer
 *	itmptr		to return callback item pointer
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *	MrmSUCCESS		operation succeeded
 *	MrmBAD_CALLBACK		invalid callback descriptor
 *	MrmOUT_OF_BOUNDS	item index out of bounds
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
RGMWidgetRecordPtr	widgetrec ;	/* widget record in context */

/*
 * Pick up a widget  record, and set descriptor pointer.
 */
widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer(context_id) ;
if ( UrmWRValid(widgetrec) )
    *descptr = (RGMCallbackDescPtr) ((char *)widgetrec+descoffs) ;
else
    return Urm__UT_Error (routine, "Invalid widget/gadget record",
        NULL, context_id, MrmBAD_RECORD) ;

if ( (*descptr)->validation != URMCallbackDescriptorValid )
    return Urm__UT_Error (routine, "Invalid callback descriptor",
        NULL, context_id, MrmBAD_CALLBACK) ;

/*
 * validate item index and compute item pointer
 */
if ( itemndx >= (*descptr)->count )
    return Urm__UT_Error (routine, "Callback item index out of bounds",
        NULL, context_id, MrmOUT_OF_BOUNDS) ;
*itmptr = &(*descptr)->item[itemndx] ;
return MrmSUCCESS ;

}

