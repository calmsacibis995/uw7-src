#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/node.c	1.6"
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <Memutil/memutil.h>
#include "misc.h"
#include "node.h"

#define INC			32
#define SNODE_head		(pSNODE ? pSNODE : refill_pSNODE())
#define DNODE_head		(pDNODE ? pDNODE : refill_pDNODE())

static SNODE *			refill_pSNODE();
static DNODE *			refill_pDNODE();
/*
 *	temporary nodes for iterator macros
 */
SNODE *				_pSNODE;
DNODE *				_pDNODE;
/*
 *	used to zero out new nodes
 */
static SNODE			emptySNODE;
static DNODE			emptyDNODE;
/*
 *	free space list pointers
 */
static SNODE *			pSNODE = NULL;
static DNODE *			pDNODE = NULL;

void
free_SNODE(p)
	SNODE *			p;
{
	p->next = pSNODE;
	pSNODE = p;
}

DNODE *
alloc_DNODE(data)
	ADDR			data;
{
	DNODE *			p = DNODE_head;

	pDNODE = pDNODE->next;
	*p = emptyDNODE;
	p->data = data;
	return p;
}

void
free_DNODE(p)
	DNODE *			p;
{
	p->next = pDNODE;
	pDNODE = p;
}

TNODE *
alloc_TNODE(data)
	ADDR			data;
{
	return (TNODE *)alloc_DNODE((ADDR)alloc_DNODE(data));
}

void
free_TNODE(p)
	TNODE *			p;
{
	free_DNODE((DNODE *)p->data);
	free_DNODE((DNODE *)p);
}

static SNODE *
refill_pSNODE()
{
	int			count = INC;
	SNODE *			p = ARRAY(SNODE, count);

	while (count--) {
		p->next = pSNODE;
		pSNODE = p++;
	}
	return pSNODE;
}

static DNODE *
refill_pDNODE()
{
	int			count = INC;
	DNODE *			p = ARRAY(DNODE, count);

	while (count--) {
		p->next = pDNODE;
		pDNODE = p++;
	}
	return pDNODE;
}

