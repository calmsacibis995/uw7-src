#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/dring.c	1.4"
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "misc.h"
#include "node.h"

DNODE *
dring_insert(root, prev, p)
	DNODE **		root;
	DNODE *			prev;
	DNODE *			p;
{
	DNODE *			next;

	if (prev) 
	  {
		next = prev->next;
	  } 
	else 
	  {
		next = dring_head(root);
		dring_head(root) = p;

		if (next) 
		  {
			prev = next->prev;
		  } 
		else 
		  {
			p->next = p;
			p->prev = p;
			return p;
		  }
	  }
	p->next = next;
	p->prev = prev;
	prev->next = p;
	next->prev = p;
	return p;
}

DNODE *
dring_delete(root, p)
	DNODE **		root;
	DNODE *			p;
{
	if (dring_head(root) == p) 
	  {
		if (dring_tail(root) == p) 
		  {
			dring_head(root) = NULL;
			p->next = NULL;
			p->prev = NULL;
			return p;
		  }
		dring_head(root) = p->next;
	  }
	p->prev->next = p->next;
	p->next->prev = p->prev;
	p->next = NULL;
	p->prev = NULL;
	return p;
}

dring_ITERATOR
dring_iterator(root)
	DNODE **		root;
{
	dring_ITERATOR		I;

	I.root = root;
	I.next = dring_head(root);
	I.prev = dring_tail(root);
	return I;
}

#ifdef notdefined

DNODE *
dring_next(pI)
	dring_ITERATOR *	pI;
{
	DNODE *			p = NULL;

	if (pI->next) 
	  {
		p = pI->next;
		pI->next = (pI->next == dring_tail(pI->root)) ? NULL : p->next;
	  }
	return p;
}

DNODE *
dring_prev(pI)
	dring_ITERATOR *	pI;
{
	DNODE *			p = NULL;

	if (pI->prev) 
	  {
		p = pI->prev;
		pI->prev = (pI->prev == dring_head(pI->root)) ? NULL : p->prev;
	  }
	return p;
}
#endif
