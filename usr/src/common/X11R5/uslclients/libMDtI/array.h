#ifndef NOIDENT
#ident	"@(#)libMDtI:array.h	1.1"
#endif

#ifndef _ARRAY_H
#define _ARRAY_H

/*************************************************************************
 *
 * Description:	OpenLook interface to a generic array (libDtI).
 *	
 *	#include <DtI.h>
 *
 *	_OlArrayStruct(Type,Name)
 *		Declare an array named "Name" containing values of
 *		type "Type".
 *
 *	_OlArrayType(Name)
 *		Refer to the array named "Name", for the purpose
 *		of creating variables of the named array type.
 *		Needed to create a variable of this array type,
 *		as in
 *
 *			_OlArrayStruct(double, Dbl);
 *			_OlArrayType(Dbl)	dblArray;
 *
 *		However, the following two might be easier:
 *
 *	_OlArrayDeclare(Name,Var)
 *		Equal to:
 *			_OlArrayType(Name)	Var;
 *
 *	_OlArrayDefine(Name,Var,Initial,Step,Fnc)
 *		Declare and initialize an array of the named type.
 *		This array can be accessed with the variable "Var",
 *		and can be referenced with the name "Name".
 *		The array is initially extended "Initial" elements,
 *		and thereafter extended "Step" elements.
 *		(No space is actually allocated until the first time
 *		an element is added to the array.)
 *		"Fnc" is the comparison function used to search
 *		for an element and when sorting the array. It's called
 *		like this:
 *
 *			(*Fnc)(pElement1, pElement2)
 *
 *		and should return -1,0,1 for <,==,>.
 *
 *	_OlArrayDef(Name,Var)
 *		As above, with
 *			Initial == _OlArrayDefaultInitial
 *			   Step == _OlArrayDefaultStep
 *			    Fnc == _OlArrayDefaultCompare
 *
 *		These defaults for "Initial" and "Step" cause
 *		an increasing extension as more space is needed, with
 *		the following progression (total extent):
 *
 *			2, 5, 9, 15, 24, 38, ...
 *
 *		(Each increase is 1/2 the current extent plus 2).
 *
 *		If "Initial" is given explicitly in _OlArrayInitialize()
 *		or _OlArrayDefine() but "Step" is defaulted, the
 *		progression starts with the initial value:
 *
 *			n, n+(n/2)+2, n+(n/2)+2 + (n+(n/2)+2)/2 + 2, ...
 *
 *		If "Initial" is defaulted but "Step" is given
 *		explicitly, the progression is uniform:
 *
 *			Step, 2*Step, 3*Step, 4*Step, ...
 *
 *		The default for "Fnc" is really just "memcmp()" in
 *		disguise. Warning: This default comparison works fine
 *		for elements that don't include pointers in their
 *		substructure (or when pointer comparison is desired).
 *		If the pointed-to values should be compared instead,
 *		you'll have to provide your own comparison routine.
 *
 *	void _OlArrayInitialize(pVar,Initial,Step,Fnc)
 *		Like _OlArrayDefine, except it will initialize an
 *		existing array pointed to by "pVar".
 *
 *	void _OlArrayInit(pVar)
 *		As above, with
 *			Initial == _OlArrayDefaultInitial
 *			   Step == _OlArrayDefaultStep
 *			    Fnc == _OlArrayDefaultCompare
 *
 *	void _OlArrayAllocate(Name,pVar,Initial,Step)
 *		Similar to the above, except will allocate the array
 *		"head" and will mark it to be automatically freed with
 *		_OlArrayFree.
 *
 *	void _OlArrayDelete(pVar,indx)
 *		Deletes the "indx"th element from the array pointed to
 *		by "pVar".
 *
 *	void _OlArrayFree(pVar)
 *		Frees the array. Note: You are responsible for freeing
 *		any auxiliary storage pointed to by each element sub-
 *		structure. Also, the memory of the original initial extent
 *		is lost--you may need to reset it with _OlArrayInitialize.
 *
 *	void _OlArrayAppend(pVar,Data)
 *		Appends the data element to the array.
 *
 *	void _OlArrayUniqueAppend(pVar,Data)
 *		Appends the data element to the array if it doesn't
 *		already exist.
 *
 *	int _OlArrayInsert(pVar,indx,Data)
 *		Inserts the data element after the "indx"th element.
 *		Returns "indx".
 *
 *	int _OlArrayOrderedInsert(pVar,Data)
 *		Inserts the data element, maintaining a sorted
 *		order governed by the comparison function registered
 *		with the array. As long as only this routine is
 *		used--not _OlArrayInsert()--the array will stay
 *		sorted. Searches will typically be much faster,
 *		since a binary search can be employed, if the
 *		list is known to be ordered.
 *		Returns the index where the element was inserted.
 *
 *	int _OlArrayOrderedUniqueInsert(pVar,Data)
 *		As above, but only if the element isn't in the list
 *		already.
 *		Returns the index where the element was found or
 *		inserted.
 *
 *	int _OlArrayHintedOrderedInsert(pVar,indx,Data)
 *		Like _OlArrayOrderedInsert, except this one allows
 *		the client to suggest where to start searching for
 *		the correct place to insert the data. Typically
 *		used right after using _OlArrayFindHint() to see
 *		if an element already exists in the array.
 *
 *	int _OlArrayFind(pVar,Data)
 *		Returns the index into the array of the given data
 *		element. Returns _OL_NULL_ARRAY_INDEX if the data
 *		element is not found.
 *
 *	int _OlArrayFindHint(pVar,pHint,Data)
 *		Like _OlArrayFind, except will also give the
 *		index of where the given data should be inserted
 *		to maintain an ordered array. The insert index is
 *		assigned to the "int" pointed to by "pHint". If
 *		the data is already in the list, the value returned
 *		by the function is the same as pointed to by "pHint".
 *
 *	void _OlArraySort(pVar)
 *		Sorts the array (uses "qsort").
 *
 *	void _OlArraySetOrdered(pVar)
 *		Marks the array as ordered. This is useful when
 *		you have inserted the data ``manually'' (with
 *		_OlArrayInsert, not _OlArrayOrderedInsert) but
 *		have still maintained a sort order.
 *
 *	_OlArrayElement(pVar,indx)
 *		References the "indx"th element in the array.
 *		The reference is the element itself, not a pointer
 *		to it. It is also an ``lvalue''. Thus, some examples
 *		of its use are:
 *
 *			_OlArrayElement(&Bob,3) = Data;
 *			_OlArrayElement(&Bob,3).field = 23;
 *			&(_OlArrayElement(&Bob,3))->field = 23;
 *
 *		CAUTION: Do not expect the address of an element to
 *		stay constant for the life of that element, even if
 *		it's index does stay constant!
 *
 *	_OlArraySize(pVar)
 *		Gives the number of elements in the array (but usually
 *		*not* the allocated extent of the array!)
 *
 *	CAUTION: The "Data" paramenter must be of the correct type
 *	for these routines to work properly; you'll get no compiler
 *	warning for wrong types (sorry)!
 *
 *	Some predefined array types and associated routines:
 *
 *	CharArray	Array of char's
 *	IntArray	Array of int's
 *	WidgetArray	Array of Widgets
 *
 *	_OlWidgetArrayFindByName(pVar,name)
 *		Searches the array (of type WidgetArray) for the named
 *		widget.
 *
 *******************************file*header******************************/



/*
 * Need the following to put in front of arg #4 of qsort,
 * to keep the compiler from warning us.
 */
typedef int (*_OlArrayQsortArg4) (const void * , const void *);

typedef int (*_OlArrayCompareFncType) ( char * , char * );

#define _OlArrayStruct(Type,Name) \
	struct Name {							\
		Type *			array;				\
		Cardinal		num_elements;			\
		Cardinal		num_slots;			\
		unsigned short		flags;				\
		unsigned short		step;				\
		_OlArrayCompareFncType	compare;			\
		Type			tmp;				\
	}
#define _OL_ARRAY_IS_ORDERED	0x0001
#define _OL_ARRAY_IS_ALLOCATED	0x0002
#define _OL_ARRAY_IS_STATIC	0x0004

typedef _OlArrayStruct(char,CharArray)		CharArray;
typedef _OlArrayStruct(Widget,WidgetArray)	WidgetArray;
typedef _OlArrayStruct(int,IntArray)		IntArray;

#define _OlArrayType(Name)			struct Name
#define _OlArrayDeclare(Name,Var)		_OlArrayType(Name) Var

#define _OlArrayDef(Name,Var) \
	_OlArrayDefine(Name,Var,_OlArrayDefaultInitial,_OlArrayDefaultStep,0)

#define _OlArrayDefine(Name,Var,Initial,Step,Fnc) \
	_OlArrayDeclare(Name,Var) = {					\
		0, 0,							\
		Initial, _OL_ARRAY_IS_ORDERED|_OL_ARRAY_IS_STATIC, Step,\
		(_OlArrayCompareFncType)Fnc				\
	}

#define _OlArrayInit(pVar) \
	_OlArrayInitialize(pVar,_OlArrayDefaultInitial,_OlArrayDefaultStep,0)

#define _OlArrayInitialize(pVar,Initial,Step,Fnc) \
	( (pVar)->array = 0,						\
	  (pVar)->num_elements = 0,					\
	  (pVar)->num_slots = (Initial),				\
	  (pVar)->flags = _OL_ARRAY_IS_ORDERED|_OL_ARRAY_IS_STATIC,	\
	  (pVar)->step = (Step),					\
	  (pVar)->compare = (_OlArrayCompareFncType)(Fnc)		\
	)

#define _OlArrayAllocate(Name,pVar,Initial,Step) \
	( pVar = XtNew(struct Name),					\
	  pVar->array = 0,						\
	  pVar->num_elements = 0,					\
	  pVar->num_slots = (Initial),					\
	  pVar->flags = _OL_ARRAY_IS_ORDERED,				\
	  pVar->step = (Step),						\
	  pVar->compare = (_OlArrayCompareFncType)0			\
	)

#if defined(SVR4_0) || defined(SVR4) || defined(__hpux)
#define OlMemMove(Type,p1,p2,n) \
    (void)memmove((XtPointer)(p1), (XtPointer)(p2), (n) * sizeof(Type))
#else
#define OlMemMove(Type,p1,p2,n)   \
    if (p1 && p2)                   \
{                               \
        register Type *from = p2 ;    \
        register Type *to   = p1 ;    \
        register Cardinal count = n ; \
                                      \
        if (from < to)                      \
	  {                           \
            from += count;            \
            to += count;              \
          while (count--)           \
                *--to = *--from;      \
		}                           \
      else {                        \
          while (count--)           \
                *to++ = *from++;      \
		}                             \
		}                                     \
    else
#endif

#define _OlArrayDelete(pVar,I) \
	if ((pVar)->array && 0 <= (I) && (I) < _OlArraySize(pVar)) {	\
		register char * pI  = (char *)&((pVar)->array[I]);	\
									\
		_OlArraySize(pVar)--;					\
		OlMemMove (						\
			char,						\
			pI,						\
			(char *)&((pVar)->array[(I)+1]),		\
			_OlArrayElementSize(pVar) * _OlArraySize(pVar)	\
				 - (pI - (char *)(pVar)->array)		\
		);							\
	} else

#define _OlArrayFree(pVar) \
	if ((pVar)) {							\
		if ((pVar)->array)					\
			XtFree ((XtPointer)(pVar)->array);		\
		if ((pVar)->flags & _OL_ARRAY_IS_STATIC) {		\
			(pVar)->array = 0;				\
			(pVar)->num_elements = 0;			\
			(pVar)->num_slots = _OlArrayDefaultInitial;	\
			(pVar)->flags |= _OL_ARRAY_IS_ORDERED;		\
		} else							\
			XtFree ((XtPointer)(pVar));			\
	} else

#define	_OlArrayAppend(pVar,Data) \
	(void)_OlArrayInsert((pVar), _OlArraySize(pVar), Data)

#define	_OlArrayUniqueAppend(pVar,Data) \
	if (_OlArrayFind(pVar, Data) == _OL_NULL_ARRAY_INDEX)		\
		(void)_OlArrayInsert(pVar, _OlArraySize(pVar), Data);	\
	else

#define	_OlArrayInsert(pVar,indx,Data) \
	__OlArrayInsert(pVar,_OL_ARRAY_DIRECT_INSERT,indx,Data)

#define	_OlArrayOrderedInsert(pVar,Data) \
	__OlArrayInsert(pVar,_OL_ARRAY_ORDERED_INSERT,_OL_NULL_ARRAY_INDEX,Data)

#define	_OlArrayHintedOrderedInsert(pVar,hint,Data) \
	__OlArrayInsert(pVar,_OL_ARRAY_ORDERED_INSERT,hint,Data)

#define	_OlArrayOrderedUniqueInsert(pVar,Data) \
	__OlArrayInsert(pVar,_OL_ARRAY_ORDERED_UNIQUE_INSERT,_OL_NULL_ARRAY_INDEX,Data)

#define __OlArrayInsert(pVar,flag,indx,Data) \
	___OlArrayInsert(						\
		__FILE__, __LINE__,					\
		(CharArray *)(pVar),					\
		flag,							\
		indx,							\
		_OlArrayElementSize(pVar),				\
		((pVar)->tmp = Data, &((pVar)->tmp))							\
	)

#define	_OlArrayFind(pVar,Data) \
	__OlArrayFind(							\
		__FILE__, __LINE__,					\
		(CharArray *)(pVar),					\
		(int *)0,						\
		_OlArrayElementSize(pVar),				\
		((pVar)->tmp = Data, &((pVar)->tmp))	 		\
	)

#define	_OlArrayFindHint(pVar,pHint,Data) \
	__OlArrayFind(							\
		__FILE__, __LINE__,					\
		(CharArray *)(pVar),					\
		pHint,							\
		_OlArrayElementSize(pVar),				\
		((pVar)->tmp = Data, &((pVar)->tmp))	 		\
	)

#define	_OlWidgetArrayFindByName(pVar,name) \
	__OlWidgetArrayFindByName(__FILE__,__LINE__,(pVar),name)

#define	_OlArraySort(pVar) \
	if (!((pVar)->flags & _OL_ARRAY_IS_ORDERED))			\
		__OlArraySort(						\
			__FILE__,__LINE__,				\
			(CharArray *)(pVar),				\
			_OlArrayElementSize(pVar)			\
		)

#define _OlArraySetOrdered(pVar) \
	(pVar)->flags |= _OL_ARRAY_IS_ORDERED

#define _OlArrayElement(pVar,indx)	((pVar)->array[indx])
#define _OlArraySize(pVar)		(pVar)->num_elements
#define _OlArrayElementSize(pVar)	(Cardinal)sizeof(*(pVar)->array)
#define _OlArrayDefaultInitial		0	/* i.e. 2 */
#define _OlArrayDefaultStep		0	/* n += (n / 2) + 2 */

#define _OL_NULL_ARRAY			0
#define _OL_NULL_ARRAY_INDEX		-1
#define _OL_ARRAY_IS_EMPTY(A)		((A)->num_elements == 0)

#define _OL_ARRAY_DIRECT_INSERT		1
#define _OL_ARRAY_ORDERED_INSERT	2
#define _OL_ARRAY_ORDERED_UNIQUE_INSERT	3

#define _OL_ARRAY_INITIAL \
	{ 0, 0,								\
	  _OlArrayDefaultInitial,					\
	  _OL_ARRAY_IS_ORDERED|_OL_ARRAY_IS_STATIC,			\
	  _OlArrayDefaultStep, 0					\
	}

extern int
_OlArrayDefaultCompare (
	char *			pA,
	char *			pB
);
extern int
___OlArrayInsert (
	char *			file,
	int			line,
	CharArray *		pVar,
	int			flag,
	int			indx,
	Cardinal		data_size,
	XtPointer		p
);
extern int
__OlArrayFind (
	char *			file,
	int			line,
	CharArray *		pVar,
	int *			pIndx,
	Cardinal		data_size,
	XtPointer		p
);
extern void
__OlArraySort (
	char *			file,
	int			line,
	CharArray *		pVar,
	Cardinal		data_size
);
extern int
__OlWidgetArrayFindByName (
	char *			file,
	int			line,
	WidgetArray *		pVar,
	String			name
);

#endif
