#ident	"@(#)ksh93:src/lib/libodelta/suftree.h	1.1"
#pragma prototyped
#ifndef _SUFTREE_H
#define _SUFTREE_H

#include <ast.h>

/* the type of list elements we play with */
typedef char	Element;

/* for suffix trees, a tree node looks like this */
typedef struct _ts_
{
	Element		*_label;	/* substring labeling the edge */
	long int	_length;	/* the length of the string */
	struct _ts_	*_child;	/* list of children */
	struct _ts_	*_sibling;	/* link for the child list */
	union
	{	/* these two fields are mutual exclusive */
		struct _ts_	*_link;		/* sub-link */
		Element		*_suffix;	/* suffix */
	}	_uls_;
}	Suftree;

/* short hand for various fields in a tree node */
#define	LABEL(n)	((n)->_label)
#define LENGTH(n)	((n)->_length)
#define CHILD(n)	((n)->_child)
#define SIBLING(n)	((n)->_sibling)
#define LINK(n)		((n)->_uls_._link)
#define SUFFIX(n)	((n)->_uls_._suffix)

/* the following definitions are not to be seen by users */
#ifdef _IN_SUF_TREE
#ifdef DEBUG
#define ASSERT(p)	if(!(p)) abort();
#else
#define ASSERT(p)
#endif /*DEBUG*/

#ifndef NULL
#define NULL	(0L)
#endif /*NULL*/

#define ALLOCSIZE	256	/* amount of nodes to allocate each time */
#define NEXT(n)		((n)->_sibling)

#endif /*_IN_SUF_TREE*/

extern Suftree*		bldsuftree(Element*, long);
extern void		delsuftree(Suftree*);
extern long		mtchsuftree(Suftree*, Element*, long, Element**);

#endif
