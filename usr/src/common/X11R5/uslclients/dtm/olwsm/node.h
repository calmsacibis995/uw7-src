#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/node.h	1.5"
#endif

#ifndef _NODE_H
#define _NODE_H

typedef struct SNODE {
	struct SNODE *		next;
	ADDR			data;
} SNODE;

typedef struct DNODE {
	struct DNODE *		next;
	struct DNODE *		prev;
	ADDR			data;
} DNODE, TNODE;

typedef struct {
	SNODE **		root;
	SNODE *			next;
} slink_ITERATOR;

typedef struct {
	SNODE **		root;
	SNODE *			next;
} sring_ITERATOR;

typedef struct {
	DNODE **		root;
	DNODE *			next;
} dlink_ITERATOR;

typedef struct {
	DNODE **		root;
	DNODE *			next;
	DNODE *			prev;
} dring_ITERATOR;

typedef struct {
	TNODE **		root;
	TNODE *			next;
} ntree_ITERATOR;
/*
 *	useful defines
 */
#define slink_head(r)		(*(r))
#define sring_head(r)		(*(r) ? (*(r))->next : NULL)
#define dlink_head(r)		(*(r))
#define dring_head(r)		(*(r))

#define sring_tail(r)		(*(r))
#define dring_tail(r)		(*(r) ? (*(r))->prev : NULL)

#define slink_push(r, p)	slink_insert((r), NULL, (p))
#define sring_push(r, p)	sring_insert((r), NULL, (p))
#define dlink_push(r, p)	dlink_insert((r), NULL, (p))
#define dring_push(r, p)	dring_insert((r), NULL, (p))

#define slink_pop(r)		(*(r) ? slink_delete((r), slink_head(r)) : NULL)
#define sring_pop(r)		(*(r) ? sring_delete((r), sring_head(r)) : NULL)
#define dlink_pop(r)		(*(r) ? dlink_delete((r), dlink_head(r)) : NULL)
#define dring_pop(r)		(*(r) ? dring_delete((r), dring_head(r)) : NULL)

#define sring_pushq(r, p)	sring_insert((r), sring_tail(r), (p))
#define dring_pushq(r, p)	dring_insert((r), dring_tail(r), (p))
/*
 *	next/prev iterators
 */
#define slink_next(pI) \
( \
	_pSNODE = (pI)->next, \
	(pI)->next = _pSNODE ? _pSNODE->next : NULL, \
	_pSNODE \
)

#define sring_next(pI) \
( \
	_pSNODE = (pI)->next, \
	(pI)->next = _pSNODE ? (((pI)->next == sring_tail((pI)->root)) ?  \
		NULL : _pSNODE->next) : NULL, \
	_pSNODE \
)

#define dlink_next(pI) \
( \
	_pDNODE = (pI)->next, \
	(pI)->next = _pDNODE ? _pDNODE->next : NULL, \
	_pDNODE \
)

#define dring_next(pI) \
( \
	_pDNODE = (pI)->next, \
	(pI)->next = _pDNODE ? (((pI)->next == dring_tail((pI)->root)) ?  \
		NULL : _pDNODE->next) : NULL, \
	_pDNODE \
)

#define dring_prev(pI) \
( \
	_pDNODE = (pI)->prev, \
	(pI)->prev = _pDNODE ? (((pI)->prev == dring_head((pI)->root)) ?  \
		NULL : _pDNODE->prev) : NULL, \
	_pDNODE \
)
/*
 *	access macros
 */
#define SNODE_next(p)		((p)->next)
#define SNODE_data(p)		((p)->data)
#define DNODE_next(p)		((p)->next)
#define DNODE_prev(p)		((p)->prev)
#define DNODE_data(p)		((p)->data)
#define TNODE_next(p)		((TNODE *)(p)->next)
#define TNODE_prev(p)		((TNODE *)(p)->prev)
#define TNODE_chain(p)		((TNODE *)(p)->data)
#define TNODE_child(p)		(TNODE_next(TNODE_chain(p)))
#define TNODE_parent(p)		(TNODE_prev(TNODE_chain(p)))
#define TNODE_data(p)		(TNODE_chain(p)->data)
/*
 *	externs for iterators
 */
extern SNODE *			_pSNODE;
extern DNODE *			_pDNODE;
/*
 *	externs in node.c
 */
extern DNODE *			alloc_DNODE();
extern TNODE *			alloc_TNODE();
extern void			free_DNODE();
extern void			free_SNODE();
extern void			free_TNODE();
/*
 *	externs in slink.c
 */
extern SNODE *			slink_insert();
extern SNODE *			slink_delete();
extern slink_ITERATOR		slink_iterator();
/*
 *	externs in sring.c
 */
extern SNODE *			sring_insert();
extern SNODE *			sring_delete();
extern sring_ITERATOR		sring_iterator();
/*
 *	externs in dlink.c
 */
extern DNODE *			dlink_insert();
extern DNODE *			dlink_delete();
extern dlink_ITERATOR		dlink_iterator();
/*
 *	externs in dring.c
 */
extern DNODE *			dring_insert();
extern DNODE *			dring_delete();
extern dring_ITERATOR		dring_iterator();
/*
 *	externs in ntree.c
 */
extern TNODE *			ntree_insert();
extern TNODE *			ntree_delete();
extern TNODE *			ntree_next();
extern ntree_ITERATOR		ntree_iterator();

#endif
