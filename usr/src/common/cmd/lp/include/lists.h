/*		copyright	"%c%" 	*/


#ifndef	LISTS_H
#define	LISTS_H
/*==================================================================*/
/*
*/
#ident	"@(#)lists.h	1.2"
#ident	"$Header$"

#include	<sys/types.h>
#include	"boolean.h"


#ifndef	NULL
#define	NULL	0
#endif

#ifndef	UCHAR
typedef	unsigned char	uchar;
#endif

typedef	enum
{
	EmptyList	= 0,
	CharList	= 1,
	IntList		= 2,
	ShortList	= 3,
	LongList	= 4,
	FloatList	= 5,
	DoubleList	= 6,
	StringList	= 7,
	PointerList	= 8,
	StructureList	= 9

}  listType;

struct	_charList
{
	uint	*map;
	uchar	*members;
};

struct	_intList
{
	uint	*map;
	uint	*members;
};

struct	_shortList
{
	uint	*map;
	ushort	*members;
};

struct	_longList
{
	uint	*map;
	ulong	*members;
};

struct	_floatList
{
	uint	*map;
		 float	*members;
};

struct	_doubleList
{
	uint	*map;
	double	*members;
};

struct	_stringList
{
	char	**members;
};

struct	_pointerList
{
	int	*sizeOfMembers;
	void	**members;
};

struct	_structureList
{
	uint	*map;
	uint	sizeOfMembers;
	void	**members;
};

typedef	struct
{
	listType	type;
	uint		flags;  /**/
	uint		size;	/*  Logical size.	*/
	uint		length; /*  N members.		*/

	union {
		struct	_charList	charList;
		struct	_intList	intList;
		struct	_shortList	shortList;
		struct	_longList	longList;
		struct	_floatList	floatList;
		struct	_doubleList	doubleList;
		struct	_stringList	stringList;
		struct	_pointerList	pointerList;
		struct	_structureList	structureList;
	} listUnion;

}  list;


typedef	union
{
	uchar	charMember;
	uint	intMember;
	ushort	shortMember;
	ulong	longMember;
	float	floatMember;
	double	doubleMember;
	char	*stringMember;
	void	*pointerMember;
	void	*structureMember;

} listMemberTypeUnion;

/*----------------------------------------------------------*/
/*
*/
#ifdef	__STDC__

int	SizeofListMember (list *, int);
list	*NewList (listType, int);
list	*ApplyToList (list *, void *(*) (), listType, int);
void	*ListMember (list *, int);
void	*PopListMember (list *);
void	*RemoveListMember (list *, int);
void	FreeList (list **);
void	FreeListMembers (list *);
boolean	AppendToList (list *, void *, int);
boolean	GrowList (list *, int);

#else

int	SizeofListMember ();
list	*NewList ();
list	*ApplyToList ();
void	*ListMember ();
void	*PopListMember ();
void	*RemoveListMember ();
void	FreeList ();
void	FreeListMembers ();
boolean	AppendToList ();
boolean	GrowList ();

#endif
/*==================================================================*/
#endif
