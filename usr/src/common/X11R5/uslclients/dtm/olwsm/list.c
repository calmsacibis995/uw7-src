#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/list.c	1.8"
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <Memutil/memutil.h>

#include "misc.h"
#include "list.h"

#define SIZE(p, n)		((p)->size * (n))
#define ELEM(p, i)		((p)->entry + SIZE((p), (i)))
#define ALLOC(n)		(ADDR)MALLOC((n))

List *
alloc_List(int size)
{
	List *			new = NULL;

	if (size > 0) 
	  {
		new = ELEMENT(List);
		new->entry = NULL;
		new->size = size;
		new->count = 0;
		new->max = 0;
	  }
	return new;
}

void
free_List(List * p)
{
	if (p) 
	  {
		if (p->entry) 
		  {
			FREE(p->entry);
		  }
		FREE(p);
	  }
}

void
compress_List(List * p)
{
	int			n;
	ADDR			entry = NULL;

	if (p) 
	  {
		if (p->count < p->max) 
		  {
			if ((n = SIZE(p, p->count)) > 0) 
			  {
				entry = ALLOC(n);
				BCOPY(p->entry, entry, n);
			  }
			FREE(p->entry);
			p->entry = entry;
			p->max = p->count;
		  }
	  }
}

void
expand_List(List * p, int count)
{
	int			n;
	ADDR			entry;

	if (p && count > 0) 
	  {
		if (p->count + count > p->max) 
		  {
			p->max = p->count + count;
			entry = ALLOC(SIZE(p, p->max));

			if ((n = SIZE(p, p->count)) > 0) 
			  {
				BCOPY(p->entry, entry, n);
			  }
			if (p->entry) 
			  {
				FREE(p->entry);
			  }
			p->entry = entry;
		  }
	  }
}

void
list_insert(List * p, int i, ADDR entry, int count)
{
	int			n;
	ADDR			q;

	if ((p == NULL)
	||  (i < 0 || i > p->count)
	||  (entry == NULL)
	||  (count <= 0)) 
	  {
		return;
	  }
	if (p->count + count > p->max) 
	  {
		p->max += MAX(p->max, count);
		q = ALLOC(SIZE(p, p->max));

		if ((n = SIZE(p, p->count)) > 0) 
		  {
			BCOPY(p->entry, q, n);
		  }
		if (p->entry) 
		  {
			FREE(p->entry);
		  }
		p->entry = q;
	  }
	if ((n = SIZE(p, p->count - i)) > 0) 
	  {
		BCOPY(ELEM(p, i), ELEM(p, i + count), n);
	  }
	p->count += count;
	BCOPY(entry, ELEM(p, i), SIZE(p, count));
}

void
list_delete(List * p, int i, int count)
{
	int			n;

	if ((p == NULL)
	||  (i < 0 || i + count > p->count)
	||  (count <= 0)) 
	  {
		return;
	  }
	if ((n = SIZE(p, p->count - (i + count))) > 0) 
	  {
		BCOPY(ELEM(p, i + count), ELEM(p, i), n);
	  }
	p->count -= count;
}

void
list_sort(List *  p, int (*f)())
{
	if (p && f && p->count) 
	  {
		qsort((char *)p->entry, p->count, p->size, f);
	  }
}

ADDR
list_search(List * p, ADDR entry,int (*f)() )
{
	if (p && entry && f && p->count) 
	  {
		return (ADDR)bsearch(
			(char*)entry, (char*)p->entry, p->count, p->size, f
		);
	  }
	return NULL;
}

list_ITERATOR
list_iterator(List * p)
{
	list_ITERATOR		I;

	I.min = ELEM(p, 0);
	I.max = ELEM(p, p->count);
	I.next = I.min;
	I.prev = I.max;
	I.size = p->size;
	return I;
}
