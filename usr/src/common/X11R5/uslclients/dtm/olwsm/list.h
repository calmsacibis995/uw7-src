#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/list.h	1.10"
#endif

#ifndef _LIST_H
#define _LIST_H

#include "misc.h"

/*
 *	List
 */
typedef struct {
	ADDR			entry;
	int			size;
	int			count;
	int			max;
} List;

typedef struct {
	ADDR			min;
	ADDR			max;
	ADDR			next;
	ADDR			prev;
	int			size;
} list_ITERATOR;
/*
 *	useful defines
 */
#define LISTOF(type)		{ NULL, sizeof(type), 0, 0 }
#define LIST(type, p)		{ (ADDR)p, sizeof(type), DIMENSION(p), 0 }

#define list_clear(p)		((p)->count = 0)
#define list_index(p, e)	(((ADDR)(e) - (p)->entry) / (p)->size)
#define list_element(p, i)	(((i) * (p)->size) + (p)->entry)
#define list_append(p, v, n)	list_insert((p), (p)->count, (ADDR)(v), (n))
#define list_push(p, v)		list_insert((p), (p)->count, (ADDR)(v), 1)
#define list_pop(p)		list_delete((p), (p)->count - 1, 1)
#define list_merge(p, new)	list_append((p), (new)->entry, (new)->count)

#define list_next(pI) \
( \
	(pI)->next < (pI)->max ? \
		((pI)->next += (pI)->size, (pI)->next - (pI)->size) : \
		NULL \
)
#define list_prev(pI) \
( \
	(pI)->prev > (pI)->min ? \
		(pI)->prev -= (pI)->size : \
		NULL \
)
#define list_dump(tag, p) \
\
	debug((stderr, \
		"%s: entry = 0x%x, size = %d, count = %d, max = %d\n", \
		(tag), (p)->entry, (p)->size, (p)->count, (p)->max))

/*
 *	externs in list.c
 */
extern List *			alloc_List();
extern void			compress_List();
extern void			expand_List();
extern void			free_List();

extern void			list_insert();
extern void			list_delete();
extern void			list_sort();
extern ADDR			list_search();
extern list_ITERATOR		list_iterator();

#endif
