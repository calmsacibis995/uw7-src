/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:olflatcvt.c	1.7"


/*
 *************************************************************************
 *
 * Description:
 *	This file contains resource converters for the flattened widget.
 *
 *	THIS IS THE STANDARD CONVERTER CHANGED FOR USE WITH WKSH.  THERE
 *	WERE SOME PROBLEMS USING THE STANDARD CONVERTER, SINCE THE STANDARD
 *	CONVERTER REQUIRED THE WIDGET TO BE IN EXISTANCE AT THE TIME OF THE
 *	CONVERSION.  THIS VERSION DOES NOT HAVE THAT REQUIREMENT, BUT IS
 *	ALSO NOT OF THE APPROPRIATE FORMAT FOR A RESOURCE CONVERTER, IT IS
 *	USED BY WKSH ONLY DURING WIDGET CREATION, THE STANDARD CONVERTER
 *	IS USED FOR XtGetValues and XtSetValues CALLS.
 *
 ******************************file*header********************************
 */

						/* #includes go here	*/

#include <stdio.h>
#include <ctype.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/FlatP.h>
#include "wksh.h"

extern Widget Toplevel;
extern char *stakalloc();

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

typedef struct {
	char *	start;
	int	length;
} StringNode, *StringArray;

#define ALLOC_STRING_ARRAY(f,stk,def_size,size)\
	auto StringNode	stk[def_size];\
	auto StringNode *	f = ((size+1) > XtNumber(stk) ?\
		(StringNode *)stakalloc((size+1)*sizeof(StringNode)) : stk)

#define SAME_STRING(s1,s2)  (!strcmp((OLconst char *)(s1),(OLconst char *)(s2)))


				/* Define parse-aiding macros		*/

#define STACK_SIZE		256
#define AT_EOL(c)		(c == '\0')
#define LITERAL			('\\')
#define FIELD_SEPARATOR		(',')
#define RECORD_LEADER		('{')
#define RECORD_TRAILER		('}')
#define IS_WHITESPACE(c)	(c == ' '|| c == '\t'|| c == '\n')
#define SCAN_WHITESPACE(s)	while(IS_WHITESPACE(*s)) ++s
#define SCAN_FOR_CHAR(s, c)	while(!AT_EOL(*s) && *s != c) ++s

	/* Scan a keyword.  The scanning ends when a field separator or
	 * a record separator is encounterd.  However, the keyword is
	 * the string starting at the first character.  It ends just
	 * prior to any whitespace preceding the two terminators above.
	 */
#define SCAN_KEYWORD(s,sn,tc)	\
    {								\
	register char *	whitespace = (char *)NULL;		\
	(sn).start  = s;					\
	while(!AT_EOL(*s)) {					\
		if (*s == LITERAL) {				\
			whitespace = (char *)NULL;		\
			++s;					\
			if (AT_EOL(*s)) {			\
				break;				\
			}					\
		} else if (*s == FIELD_SEPARATOR ||		\
			   *s == RECORD_TRAILER) {		\
			break;					\
		} else if (IS_WHITESPACE(*s)) {			\
			if (whitespace == (char *)NULL)		\
				whitespace = s;			\
		} else {					\
			whitespace = (char *)NULL;		\
		}						\
		++s;						\
	}							\
	(sn).length = (int)((whitespace!=(char*)NULL ?		\
				whitespace : s) - (sn).start);	\
    }								\
    (tc) += (sn).length + 1

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private  Procedures 
 *		2. Public  Procedures 
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static int	CopyKeyword OL_ARGS((char *, char *, int));
static int	CountMaxFields OL_ARGS((char *));
static XtArgVal	GetArgVal OL_ARGS((XtResource *, StringNode *));
XtResource *GetResource();

					/* public procedures		*/

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ****************************private*procedures***************************
 */

/*
 *************************************************************************
 * CopyKeyword - copies a keyword into a buffer and returns the actual
 * characters in the word.  Literal characters are not included in the
 * count and are not copied into the destination.
 ****************************procedure*header*****************************
 */
static int
CopyKeyword OLARGLIST((dest, src, raw_length))
	OLARG( char *,	dest)
	OLARG( char *,	src)
	OLGRA( int,	raw_length)
{
	char *	old_dest = dest;

	if (raw_length) {
		for ( ; raw_length; --raw_length, ++src)
		{
			if (*src == LITERAL) {
				--raw_length;
				++src;
				*dest++ = (*src == 'n' ? '\n' :
					   *src == 't' ? '\t' :
					   *src == 'b' ? '\b' : *src);
			}
			else {
				*dest++ = *src;
			}
		}
		*dest++ = '\0';
	}

	return((int)(dest - old_dest));
} /* END OF CopyKeyword() */

/*
 *************************************************************************
 * CountMaxFields - counts the number of field separators to determine
 * the maximum number of fields within the string.
 ****************************procedure*header*****************************
 */
static int
CountMaxFields OLARGLIST((s))
	OLGRA( register char *,	s)
{
	int	hits;

	for (hits=0; *s; ++s)
		if (*s == FIELD_SEPARATOR || *s == RECORD_TRAILER)
			++hits;
	return(hits);
} /* END OF CountMaxFields() */

/*
 *************************************************************************
 * WkshCvtStringToFlatItemFields -
 ****************************procedure*header*****************************
 */
/* ARGSUSED */

Boolean
WkshCvtStringToFlatItemFields(from, retitemfields, numitemfields, class)
char *from;
String **retitemfields;
int *numitemfields;
classtab_t *class;
{
	String *	item_fields;
	int		i = CountMaxFields((char *)(from));
	ALLOC_STRING_ARRAY(fields,stack,20,i);
	StringNode	*orig_fields = fields;
	int		elements;
	int		num_recs;
	int		total_chars;
	char *		name;
	char *		s;
	static OLconst char *	proc_name = "CvtStringToFlatItemFields";
	Cardinal	size;

	if (!ParseString(proc_name, (char *)from,
			fields, &elements, &num_recs, &total_chars))
	{
		return(False);
	}
	else if (num_recs != 1)
	{
		XtWarningMsg("tooManyRecords",
			"Convert", (const char *)"OlToolkitWarning",
			"database\
 XtNnumItemFields value has more than 1 record specification", NULL, 0);
		return(False);
	}

	/* If no fields were specified, return NULL.	*/

	if (num_recs == 0)
	{
		*retitemfields = (String *)NULL;
		*numitemfields = 0;
		return;
	}

	size		= (Cardinal)((elements * sizeof(String)) + total_chars);
	item_fields	= (String *)XtMalloc(size);

			/* Set the starting location where we can write
			 * the strings.					*/

	s = (char *)item_fields + (sizeof(String) * elements);

	for (i = 0; i < elements; ++i, ++fields)
	{
		if (fields->length == 0)
		{
			XtWarningMsg("nullItemField",
				"Converter", (const char *)"OlToolkitWarning",
				"cannot\
 have NULL itemField name", NULL, 0);
			break;
		}

		name = s;
		s += CopyKeyword(name, fields->start, fields->length);

		if (GetResource((FlatWidgetClass)class->class, (String)name) != (XtResource *)NULL)
		{
			item_fields[i] = name;
		}
		else
		{
			XtWarningMsg("badItemResource",
				"Converter", (const char *)"OlToolkitWarning",
				"no such\
 resource", NULL, 0);
			break;
		}
	}
	
			/* if 'i' is less than elements, an error
			 * occurred, so free memory and set the
			 * size to indicate the conversion failed.	*/

	if (i < elements)
	{
		XtFree((char *)item_fields);
		return(False);
	}


	*numitemfields = (Cardinal)elements;
	*retitemfields = item_fields;
	return;
} /* END OF CvtStringToFlatItemFields() */

/*
 *************************************************************************
 * WkshCvtStringToFlatItems -
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
Boolean
WkshCvtStringToFlatItems(from, retitems, numitems, itemfields, numitemfields, class)
char *from;
XtArgVal **retitems;
int *numitems;
String **itemfields;
int *numitemfields;
classtab_t *class;
{
	static XtPointer	items;
	XtArgVal *	argvals;
	char *		s;
	char *		orig_s;
	int		elements;
	int		num_recs;
	int		total_chars;
	int		i;
	int		j = CountMaxFields((char *)(from));
	ALLOC_STRING_ARRAY(fields,stack,50,j);
	StringNode	*orig_fields = fields;
	static OLconst char *	proc_name = "WKSHCvtStringToFlatItems";
	XtResource *	rsc_stack[20];
	XtResource **	rscs;
	Cardinal	size;

	if (*numitemfields == 0) {
		XtWarningMsg("NumItemFieldsConflict",
			"Convert", (const char *)"OlToolkitWarning",
			"itemFields resource should be set first", NULL, 0);
		return(FALSE);
	}
	if (!ParseString(proc_name, from,
			fields, &elements, &num_recs, &total_chars)) {
		return(False);
	} else if (*numitemfields != elements)
	{
		XtWarningMsg("itemsAndNumItemFieldsConflict",
			"Convert", (const char *)"OlToolkitWarning",
			"number of\
 XtNnumItemFields conflicts with XtNitems' fields per record", NULL, 0);
		return(False);
	}

	/* If there are no items or no itemFields, return NULL.	*/

	if (num_recs == 0) {
		return(False);
	}

	size	= (Cardinal) ((elements * num_recs * sizeof(XtArgVal)) +
				total_chars);
	items	= (XtPointer)XtMalloc(size);

			/* Populate an array with the addresses of
			 * the itemField resource structures.		*/

	rscs = (elements <= XtNumber(rsc_stack) ? rsc_stack :
		(XtResource **)XtMalloc((Cardinal)
				(elements*sizeof(XtResource*))));

	for (i=0; i < *numitemfields; ++i) {
		rscs[i] = GetResource((FlatWidgetClass)class->class, (*itemfields)[i]);
	}

	/* Set the starting location where we can write
	 * the strings.					*/

	s = (char *)items + (sizeof(XtArgVal) * num_recs * elements);
	orig_s = s;

	for (i=0, argvals=(XtArgVal *)items; i < num_recs; ++i) {
		for (j = 0; j < elements; ++j, ++fields) {
			if (rscs[j] != (XtResource *)NULL) {
				/* If this resource is a String, make
				 * a copy of it; else, convert it.	*/

				if (strcmp((OLconst char *)XtRString,
					(OLconst char *)rscs[j]->resource_type) == 0) {
					*argvals = (XtArgVal)s;
					s += CopyKeyword(s, fields->start,
							fields->length);
				} else {
					*argvals = GetArgVal(rscs[j], fields);
				}
			} else {
				*argvals = (XtArgVal)0;
			}
			++argvals;
		}
	}
	
	*numitems = (Cardinal)num_recs;
	*retitems = (XtArgVal *)items;
	return(TRUE);
} /* END OF CvtStringToFlatItems() */

/*
 *************************************************************************
 * GetArgVal - converts a string into an XtArgVal and returns it.
 * Note: this procedure is not called the the resource type is XtRString.
 ****************************procedure*header*****************************
 */
static XtArgVal
GetArgVal OLARGLIST((rsc, sn))
	OLARG( XtResource *,	rsc)
	OLGRA( StringNode *,	sn)
{
	XtArgVal	val;
	String		type = rsc->resource_type;
	char		stack[256];
	char *		name = ((sn->length + 1) > sizeof(stack) ?
				stakalloc((Cardinal)(sn->length+1)) : stack);
	XrmValue	from_val;
	XrmValue	to_val;
	Boolean		use_to_val = True;

	if (sn->length == 0)
	{
		name = "";
	}
	else
	{
		(void) CopyKeyword(name, sn->start, sn->length);
	}

	from_val.size	= (unsigned int)sizeof(String);
	from_val.addr	= (caddr_t)name;

	XtConvert(Toplevel, XtRString, &from_val, type, &to_val);

		/* If the conversion failed, use the default value.	*/

	if (to_val.size == (unsigned int)0)
	{
		if (SAME_STRING(XtRImmediate, type))
		{
			use_to_val	= False;
			val		= (XtArgVal)rsc->default_addr;
		}
		else
		{
					/* Use the default address	*/

			from_val.addr = (caddr_t)rsc->default_addr;

			XtConvert((Widget)Toplevel, XtRString, &from_val,
					type, &to_val);
		}
	}
	else
	{
		_OlConvertToXtArgVal((XtPointer)to_val.addr,
					&val, (Cardinal)to_val.size);
	}

	if (use_to_val == True)
	{
		if (to_val.size == (unsigned int)0)
		{
			val = (XtArgVal)0;
		}
		else
		{
			_OlConvertToXtArgVal((XtPointer)to_val.addr,
				&val, (Cardinal)to_val.size);
		}
	}

	return(val);
} /* END OF GetArgVal() */

/*
 *************************************************************************
 * GetResource - returns the address of the item resource associated
 * with the supplied name.
 ****************************procedure*header*****************************
 */
static XtResource *
GetResource OLARGLIST((class, name))
	OLARG( FlatWidgetClass,	class)
	OLGRA( String,		name)
{
	XrmQuarkList	qlist = class->flat_class.quarked_items;
	XrmQuark	quark = XrmStringToQuark(name);
	Cardinal	max = class->flat_class.num_item_resources;
	Cardinal	i;

	for (i=0; i < max; ++i, ++qlist)
	{
		if (quark == *qlist)
		{
			return(class->flat_class.item_resources + i);
		}
	}

	return((XtResource *)NULL);
} /* END OF GetResource() */

/*
 *************************************************************************
 * ParseString - Parses a string and pushes locations and lengths of
 * embedded strings into a supplied array.  The return the number elements
 * in each record and the number of records found.
 * If an error is encountered, a 0 is returned.
 ****************************procedure*header*****************************
 */
static int
ParseString OLARGLIST((proc_name, s, kw_array, elements, num_recs, total))
	OLARG( OLconst char *,	proc_name)
	OLARG( register char *,	s)
	OLARG( StringArray,	kw_array)
	OLARG( int *,		elements)
	OLARG( int *,		num_recs)
	OLGRA( int *,		total)			/* total chars	*/
{
	int		num;
	char *		error = (char *)NULL;
	int		e;

	*elements	= 0;
	*num_recs	= 0;
	*total		= 0;
	num		= 0;

	while (!AT_EOL(*s))
	{
		SCAN_WHITESPACE(s);

		if (RECORD_LEADER != *s)
		{
			error = "Missing record leader '{'";
			break;
		}

		++s;		/* skip record leader	*/
		e = 0;

		while(!AT_EOL(*s))
		{
			SCAN_WHITESPACE(s);

			SCAN_KEYWORD(s, kw_array[num], *total);

			++e;
			++num;

			if (*s == FIELD_SEPARATOR) {
				++s;
			} else if (*s == RECORD_TRAILER) {
				break;
			} else {
				error = "Missing field separator ',' or \
 record trailer '}'";
				break;
			}
		}

		if (error != (char *)NULL) {
			break;
		} else {
			++(*num_recs);
		}

		if (*s != RECORD_TRAILER) {
			error = "Missing Trailer";
			break;
		}

		if (*elements == 0) {
			*elements = e;
		} else if (*elements != e) {
			error = "records within list cannot have different\
 number of elements";
			break;
		}

				/* Skip the record trailer and search for
				 * the next leader.			*/
		++s;
		SCAN_FOR_CHAR(s, RECORD_LEADER);
	}

	if (error != (char *)NULL)
	{
		XtWarningMsg("parseError", "Converter",
			(const char *)"OlToolkitWarning", "Converter: flat widget parse failed", NULL, 0);
	}
	return(error ? 0 : 1);
} /* END OF ParseString() */
