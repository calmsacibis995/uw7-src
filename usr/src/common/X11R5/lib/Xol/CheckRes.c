#ifndef	NOIDENT
#ident	"@(#)olmisc:CheckRes.c	1.3"
#endif

#if	defined(__STDC__)
# include "stdarg.h"
#else
# include "varargs.h"
#endif

#include "string.h"

#include "X11/IntrinsicP.h"
#include "X11/ConstrainP.h"
#include "X11/StringDefs.h"

#include "Xol/OpenLookP.h"
#include "Xol/Error.h"

/*
 * Convenient macros:
 */

#define CORE_C(wc) (((WidgetClass)(wc))->core_class)
#define CONSTRAINT_C(wc) (((ConstraintWidgetClass)(wc))->constraint_class)
#define CORE_P(w) ((Widget)(w))->core

	/*
	 * Why -offset-1? Because the resource list is in "internal"
	 * form, with the offset marked this way to indicate so.
	 */
#define OFFSET(O) (-O-1)

	/*
	 * Assume from/to are disjoint memory areas.
	 */
#define COPY(from,to,offset,size) \
	memcpy ((char *)to + offset, (char *)from + offset, size)

#define STREQU(A,B) (strcmp((A),(B)) == 0)

/*
 * Private routines:
 */

static void		CheckReadOnlyResources OL_ARGS((
	Widget			w,
	Arg *			args,
	Cardinal		num_args,
	XtPointer		new,
	XtPointer		current,
	XtResourceList		resources,
	Cardinal		num_resources
));
static void		CheckUnsettableResources OL_ARGS((
	Arg *			args,
	Cardinal		num_args,
	XtPointer		new,
	XtPointer		current,
	XtResourceList		resources,
	Cardinal		num_resources,
	va_list			ap
));
static Boolean		FindOffsetAndSize OL_ARGS((
	String			name,
	XtResourceList		resources,
	Cardinal		num_resources,
	Cardinal *		offset,
	Cardinal *		size
));

/**
 ** OlCheckReadOnlyResources()
 **/

void
#if	OlNeedFunctionPrototypes
OlCheckReadOnlyResources (
	Widget			new,
	Widget			current,
	Arg *			args,
	Cardinal		num_args
)
#else
OlCheckReadOnlyResources (new, current, args, num_args)
	Widget			new;
	Widget			current;
	Arg *			args;
	Cardinal		num_args;
#endif
{
	WidgetClass		wc = XtClass(new);

	CheckReadOnlyResources (
		new, args, num_args,
		(XtPointer)new, (XtPointer)current,
		CORE_C(wc).resources, CORE_C(wc).num_resources
	);
	return;
} /* OlCheckReadOnlyResources */

/**
 ** OlCheckReadOnlyConstraintResources()
 **/

void
#if	OlNeedFunctionPrototypes
OlCheckReadOnlyConstraintResources (
	Widget			new,
	Widget			current,
	Arg *			args,
	Cardinal		num_args
)
#else
OlCheckReadOnlyConstraintResources (new, current, args, num_args)
	Widget			new;
	Widget			current;
	Arg *			args;
	Cardinal		num_args;
#endif
{
	WidgetClass		wc = XtClass(XtParent(new));

	CheckReadOnlyResources (
		new, args, num_args,
		CORE_P(new).constraints,
		current? CORE_P(current).constraints : (XtPointer)0,
		CONSTRAINT_C(wc).resources, CONSTRAINT_C(wc).num_resources
	);
	return;
} /* OlCheckReadOnlyConstraintResources */

/**
 ** CheckReadOnlyResources()
 **/

static void
#if	OlNeedFunctionPrototypes
CheckReadOnlyResources (
	Widget			w,
	Arg *			args,
	Cardinal		num_args,
	XtPointer		new,
	XtPointer		current,
	XtResourceList		resources,
	Cardinal		num_resources
)
#else
CheckReadOnlyResources (w, args, num_args, new, current, resources, num_resources)
	Widget			w;
	Arg *			args;
	Cardinal		num_args;
	XtPointer		new;
	XtPointer		current;
	XtResourceList		resources;
	Cardinal		num_resources;
#endif
{
	Arg *			arg;

	static XrmName		ReadOnly = 0;


	if (!ReadOnly)
		ReadOnly = XrmStringToName(XtCReadOnly);

	for (arg = args; num_args != 0; num_args--, arg++) {
		XrmResourceList *	r = (XrmResourceList *)resources;
		XrmName			name = XrmStringToName(arg->name);
		Cardinal		n;

		for (n = 0; n < num_resources; n++) {
			if (name == r[n]->xrm_name) {
				if (r[n]->xrm_class == ReadOnly) {
					OlVaDisplayWarningMsg (
						XtDisplayOfObject(w),
						"readOnlyResource",
						"setAttempted",
						OleCOlToolkitWarning,
						"Widget %s: XtN%s resource is read-only, it can't be set",
						XtName(w), arg->name
					);
					if (current)
						COPY (
						    current, new,
						    OFFSET(r[n]->xrm_offset),
						    r[n]->xrm_size
						);
				}
				break;
			}
		}
	}

	return;
} /* CheckReadOnlyResources */

/**
 ** OlCheckUnsettableResources()
 **/

void
#if	OlNeedFunctionPrototypes
OlCheckUnsettableResources (
	Widget			new,
	Widget			current,
	Arg *			args,
	Cardinal		num_args,
	...
)
{
#else
OlCheckUnsettableResources (va_alist)
	va_dcl
{
	Widget			new;
	Widget			current;
	Arg *			args;
	Cardinal		num_args;
#endif

	va_list			ap;

	WidgetClass		wc = XtClass(new);


#if	OlNeedFunctionPrototypes
	va_start (ap, num_args);
#else
	va_start (ap);
	new = va_arg(ap, Widget);
	current = va_arg(ap, Widget);
	args = va_arg(ap, Arg *);
	num_args = va_arg(ap, Cardinal);
#endif

	CheckUnsettableResources (
		args, num_args,
		(XtPointer)new, (XtPointer)current,
		CORE_C(wc).resources, CORE_C(wc).num_resources,
		ap
	);

	va_end (ap);
	return;
} /* OlCheckUnsettableResources */

/**
 ** OlCheckUnsettableConstraintResources()
 **/

void
#if	OlNeedFunctionPrototypes
OlCheckUnsettableConstraintResources (
	Widget			new,
	Widget			current,
	Arg *			args,
	Cardinal		num_args,
	...
)
{
#else
OlCheckUnsettableConstraintResources (va_alist)
	va_dcl
{
	Widget			new;
	Widget			current;
	Arg *			args;
	Cardinal		num_args;
#endif

	va_list			ap;

	WidgetClass		wc = XtClass(XtParent(new));


#if	OlNeedFunctionPrototypes
	va_start (ap, num_args);
#else
	va_start (ap);
	new = va_arg(ap, Widget);
	current = va_arg(ap, Widget);
	args = va_arg(ap, Arg *);
	num_args = va_arg(ap, Cardinal);
#endif

	CheckUnsettableResources (
		args, num_args,
		CORE_P(new).constraints,
		current? CORE_P(current).constraints : (XtPointer)0,
		CONSTRAINT_C(wc).resources, CONSTRAINT_C(wc).num_resources,
		ap
	);

	va_end (ap);
	return;
} /* OlCheckUnsettableConstraintResources */

/**
 ** CheckUnsettableResources()
 **/

static void
#if	OlNeedFunctionPrototypes
CheckUnsettableResources (
	Arg *			args,
	Cardinal		num_args,
	XtPointer		new,
	XtPointer		current,
	XtResourceList		resources,
	Cardinal		num_resources,
	va_list			ap
)
#else
CheckUnsettableResources (args, num_args, new, current, resources, num_resources, ap)
	Arg *			args;
	Cardinal		num_args;
	XtPointer		new;
	XtPointer		current;
	XtResourceList		resources;
	Cardinal		num_resources;
	va_list			ap;
#endif
{
	String			name;

	Cardinal		n;
	Cardinal		offset;
	Cardinal		size;


	while ((name = va_arg(ap, String)))
	    for (n = 0; n < num_args; n++)
		if (STREQU(name, args[n].name)) {
			OlVaDisplayWarningMsg (
				XtDisplayOfObject(new),
				"unsettableResource", "setValues",
				OleCOlToolkitWarning,
				"Widget %s: XtN%s resource can not be set",
				XtName(new), name
			);
			if (current && FindOffsetAndSize (
				name, resources, num_resources,
				&offset, &size
			))
				COPY (current, new, offset, size);
		}

	return;
} /* CheckUnsettableResources */

/**
 ** FindOffsetAndSize()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
FindOffsetAndSize (
	String			name,
	XtResourceList		resources,
	Cardinal		num_resources,
	Cardinal *		offset,
	Cardinal *		size
)
#else
FindOffsetAndSize (name, resources, num_resources, offset, size)
	String			name;
	XtResourceList		resources;
	Cardinal		num_resources;
	Cardinal *		offset;
	Cardinal *		size;
#endif
{
	XrmResourceList *	r = (XrmResourceList *)resources;

	XrmName			xrm_name = XrmStringToName(name);

	Cardinal		n;


	for (n = 0; n < num_resources; r++, n++)
		if (xrm_name == r[n]->xrm_name) {
			*offset = OFFSET(r[n]->xrm_offset);
			*size =	r[n]->xrm_size;
			return (True);
		}
	return (False);
} /* FindOffsetAndSize */
