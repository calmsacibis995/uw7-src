#ifndef NOIDENT
#ident	"@(#)olmisc:array.c	1.9"
#endif

#if	defined(SVR4)
#include "stdarg.h"
#else
#include "varargs.h"
#endif

#include "string.h"
#include "malloc.h"
#include "stdio.h"
#include "memory.h"

#include "Xol/array.h"

/*
 * Convenient macros:
 */

	/*
	 * AT() gives the address of the Nth item.
	 * Note: Assumes local variable "data_size" is set.
	 */
#define AT(pVar,N) ((char *)(pVar)->array + (data_size * (N)))

	/*
	 * CMP() compares two elements.
	 * Note: Assumes local variables "pVar" and "data_size" are set.
	 */
#define CMP(pA,pB) \
	(pVar->compare?							\
		  ((*pVar->compare)((char *)pA, (char *)pB))		\
		: memcmp((mem_hush)pA, (mem_hush)pB, data_size)		\
	)

	/*
	 * EQU() sees if two elements are equal.
	 * Note: Assumes local variables "pVar" and "data_size" are set.
	 */
#define EQU(pA,pB)	(CMP(pA,pB) == 0)

	/*
	 * This keeps the compiler from complaining about the arguments
	 * we pass to the memory(3C) routines.
	 */
#if	OlNeedFunctionPrototypes
# define mem_hush	const void *
#else
# define mem_hush	char *
#endif

#define STREQU(A,B)	(strcmp((A),(B)) == 0)

/*
 * Private data:
 */

static Cardinal		__OlArrayDataSize	= 0;

/*
 * Private routines:
 */

static Boolean		___OlArrayFind OL_ARGS((
	CharArray *		pVar,
	int *			pIndx,
	int			hint,
	Cardinal		data_size,
	XtPointer		p
));

/**
 ** _OlArrayDefaultCompare()
 **
 ** Default comparison routine for use with "qsort" and similar routines.
 ** Note: Requires static variable "__OlArrayDataSize".
 **/

int
#if	OlNeedFunctionPrototype
_OlArrayDefaultCompare (
	char *			pA,
	char *			pB
)
#else
_OlArrayDefaultCompare (pA, pB)
	char *			pA;
	char *			pB;
#endif
{
	return (memcmp((mem_hush)pA, (mem_hush)pB, __OlArrayDataSize));
} /* _OlArrayDefaultCompare() */

/**
 ** __OlArrayFind()
 **/

int
#if	OlNeedFunctionPrototypes
__OlArrayFind (
	String			file,
	int			line,
	CharArray *		pVar,
	int *			pIndx,
	Cardinal		data_size,
	XtPointer		p
)
#else
__OlArrayFind (file, line, pVar, pIndx, data_size, p)
	String			file;
	int			line;
	CharArray *		pVar;
	int *			pIndx;
	Cardinal		data_size;
	XtPointer		p;
#endif
{
	int			indx	= _OL_NULL_ARRAY_INDEX;

	Boolean			found	= False;

	if (!pVar)
		OlVaDisplayWarningMsg (
			(Display *)0,
			"emptyArray",
			"illegalOperation",
			"OlToolkitWarning",
			"Null array pointer: \"%s\", line %d.",
			file, line
		);
	else {
		found = ___OlArrayFind(pVar, &indx, _OL_NULL_ARRAY_INDEX, data_size, p);
		if (pIndx)
			*pIndx = indx;
	}

	return (found? indx : _OL_NULL_ARRAY_INDEX);
} /* __OlArrayFind */

/**
 ** ___OlArrayInsert()
 **/

int
#if	OlNeedFunctionPrototypes
___OlArrayInsert (
	String			file,
	int			line,
	CharArray *		pVar,
	int			flag,
	int			indx,
	Cardinal		data_size,
	XtPointer		p
)
#else
___OlArrayInsert (file, line, pVar, flag, indx, data_size, p)
	String			file;
	int			line;
	CharArray *		pVar;
	int			flag;
	int			indx;
	Cardinal		data_size;
	XtPointer		p;
#endif
{
	if (!pVar) {
		OlVaDisplayWarningMsg (
			(Display *)0,
			"emptyArray",
			"illegalOperation",
			"OlToolkitWarning",
			"Null array pointer: \"%s\", line %d.",
			file, line
		);
		return (_OL_NULL_ARRAY_INDEX);
	}

	switch (flag) {

	case _OL_ARRAY_ORDERED_INSERT:
	case _OL_ARRAY_ORDERED_UNIQUE_INSERT:
		if (!(pVar->flags & _OL_ARRAY_IS_ORDERED)) {
			OlVaDisplayWarningMsg (
				(Display *)0,
				"orderedInsert",
				"illegalOperation",
				"OlToolkitWarning",
	"Attempted ordered insert into unordered array: \"%s\", line %d.",
				file, line
			);
			indx = pVar->num_elements;
		} else if (flag == _OL_ARRAY_ORDERED_UNIQUE_INSERT) {
			if (___OlArrayFind(pVar, &indx, indx, data_size, p))
				return (indx);
		} else
			(void)___OlArrayFind(pVar, &indx, indx, data_size, p);
		break;

	case _OL_ARRAY_DIRECT_INSERT:
	default:
		pVar->flags &= ~_OL_ARRAY_IS_ORDERED;
		break;
	}

	/*
	 * If the array is empty and we've been given an explicit
	 * starting size, preallocate to that size. If we haven't been
	 * given an explicit starting size, don't do anything yet...the
	 * step size will be used shortly.
	 */
	if (pVar->num_slots && !(pVar->flags & _OL_ARRAY_IS_ALLOCATED))
		pVar->array = XtMalloc(pVar->num_slots * data_size);

	/*
	 * If we don't have any more room, allocate some.
	 */
	if (pVar->num_elements == pVar->num_slots) {
		if (pVar->step == _OlArrayDefaultStep)
			pVar->num_slots += (pVar->num_slots / 2) + 2;
		else
			pVar->num_slots += pVar->step;
		pVar->array = XtRealloc(
			pVar->array, (pVar->num_slots * data_size)
		);
	}

	pVar->flags |= _OL_ARRAY_IS_ALLOCATED;

	/*
	 * An out-of-range index causes an append.
	 */
	if ((indx < 0) || (indx > pVar->num_elements))
		indx = pVar->num_elements;

	/*
	 * Move up the data starting at "indx" to make room for the
	 * new data-item. Note: "->num_elements" hasn't been bumped
	 * yet, so "->num_elements - indx" is the number of data-items
	 * from "indx" up.
	 */
	if (pVar->num_elements > indx)
		OlMemMove (
			char,
			AT(pVar, indx+1),
			AT(pVar, indx),
			data_size * (pVar->num_elements - indx)
		);

	/*
	 * Copy the data, bump the count.
	 */
	memcpy (AT(pVar, indx), (char *)p, data_size);
	pVar->num_elements++;

	return (indx);
} /* ___OlArrayInsert() */

/**
 ** __OlArraySort()
 **/

void
#if	OlNeedFunctionPrototypes
__OlArraySort (
	String			file,
	int			line,
	CharArray *		pVar,
	Cardinal		data_size
)
#else
__OlArraySort (file, line, pVar, data_size)
	String			file;
	int			line;
	CharArray *		pVar;
	Cardinal		data_size;
#endif
{
	_OlArrayQsortArg4	cmp;


	if (!pVar) {
		OlVaDisplayWarningMsg (
			(Display *)0,
			"emptyArray",
			"illegalOperation",
			"OlToolkitWarning",
			"Null array pointer: \"%s\", line %d.",
			file, line
		);
		return;
	}

	__OlArrayDataSize = data_size;	/* just in case */
	if (!(cmp = (_OlArrayQsortArg4)pVar->compare))
		cmp = (_OlArrayQsortArg4)_OlArrayDefaultCompare;

	qsort ((char *)pVar->array, pVar->num_elements, data_size, cmp);
	pVar->flags |= _OL_ARRAY_IS_ORDERED;

	return;
} /* __OlArraySort */

/**
 ** ___OlArrayFind()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
___OlArrayFind (
	CharArray *		pVar,
	int *			pIndx,
	int			hint,
	Cardinal		data_size,
	XtPointer		p
)
#else
___OlArrayFind (pVar, pIndx, hint, data_size, p)
	CharArray *		pVar;
	int *			pIndx;
	int			hint;
	Cardinal		data_size;
	XtPointer		p;
#endif
{
	if (!pVar->num_elements) {
		*pIndx = 0;
		return (False);

	} else if (pVar->flags & _OL_ARRAY_IS_ORDERED) {
		register int		low	= 0;
		register int		high	= pVar->num_elements - 1;
		register int		i;
		register int		cmp;
		register int		mid;

		if (low <= hint && hint <= high)
			mid = hint;
		else
			mid = ((high - low) / 2);
		do {
			i = low + mid;

			cmp = CMP(p, AT(pVar,i));

			if (cmp == 0)
				break;
			else if (cmp < 0)
				high = i - 1;
			else
				low = i + 1;

			mid = ((high - low) / 2);
		} while (high >= low);

		/*
		 * "cmp" holds the result of the last comparison of the
		 * searched-for element and the "i"th element in the
		 * array. If it shows that the searched-for element was
		 * ``greater'' than the "i"th element, then "i+1" is
		 * where the searched-for element should be inserted.
		 */
		if (cmp > 0)
			i++;
		*pIndx = i;
		return (cmp == 0);

	} else {
		register int		i;
		register int		start;
		register Boolean	found	= False;

		if (hint != _OL_NULL_ARRAY_INDEX)
			start = hint;
		else
			start = 0;

		for (i = start; i < pVar->num_elements; i++)  {
			if ((found = EQU(p, AT(pVar,i))))
				break;
		}
		if (start > 0 && !found)
			for (i = 0; i < start; i++)
				if ((found = EQU(p, AT(pVar,i))))
					break;

		*pIndx = i;
		return (found);
	}
} /* ___OlArrayFind() */

/**
 ** __OlWidgetArrayFindByName()
 **/

int
#if	OlNeedFunctionPrototypes
__OlWidgetArrayFindByName (
	String			file,
	int			line,
	WidgetArray *		pVar,
	String			name
)
#else
__OlWidgetArrayFindByName (file, line, pVar, name)
	String			file;
	int			line;
	WidgetArray *		pVar;
	String			name;
#endif
{
	if (pVar && pVar->num_elements) {
		register int		i;

		for (i = 0; i < pVar->num_elements; i++)
			if (STREQU(XtName(pVar->array[i]), name))
				return (i);
	}
	return (_OL_NULL_ARRAY_INDEX);
} /* __OlWidgetArrayFindByName() */
