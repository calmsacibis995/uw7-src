#ident	"@(#)debugger:libdbgen/common/Rbtree.C	1.4"
#include	"Rbtree.h"


/*
*	Rbtree routines
*	A threaded red-black implementation of a 2-3-4 tree.
*
*	A red-black tree is a binary tree in which localized
*	node groups correspond to single nodes in a 2-3-4 tree.
*	A 2-node is represented as a regular (black) node.
*	A 3-node is represented as a black node above a red node,
*	either as the left or right child.
*	A 4-node is represented as a black node above 2 red nodes.
*	Thus, the root is a black node and a red node can't have a 
*	red parent.  The height is at most 2log2n.
*
*	The difference between a red-black 4-node and three 2-nodes
*	is only a matter of whether the children are red nodes.
*	Thus, the start of a 4-node split is simply changing
*	the two children from red to black.
*	When a 4-node is split,
*	the middle item is moved into the parent node;
*	in a red-black tree,
*	this is equivalent to changing that node to red;
*	and as long its red-black parent is black,
*	there is nothing left to do.
*	If the red-black parent node is red,
*	rotations such as in AVL trees can be used
*	to restore red-black correspondence to 2-3-4 nodes.
*	This corresponds to two of the three situations
*	in which the 4-node has a 3-node parent in a 2-3-4 tree.
*
*	There are only four distinct red above red patterns
*	to be repaired due to insertion.
*	Two cases require a single rotation to separate the
*	two red nodes into siblings of a black grandparent node.
*	The other two cases reduce to the first two by a similar
*	single rotation;
*	thus, they need a double rotation to complete the repair.
*	After any of these four cases,
*	the next 4-node down the tree can be no closer than
*	a grandchild of the newly created 4-node.
*/

Rbtree::~Rbtree()
{
	Rbnode	*p = firstnode, *q;
	while(p)
	{
		q = p;
		p = p->next();
		delete q;
	}

}

Rbtree::Rbtree()
{
	firstnode = lastnode = 0;
	end.color = Black;
	end.leftchild = &end;
	end.rightchild = 0;
	end.prevnode = end.nextnode = 0;
}

// makenode and cmp are virtual functions
// useful functions are defined in the derived classes

Rbnode *
Rbnode::makenode()
{
	return new Rbnode;
}

int
Rbnode::cmp( Rbnode & )
{
	return 0;
}

int
Rbnode::lookupCmp(Rbnode &node)
{
	return cmp(node);
}

Rbnode *
Rbtree::tlookup(Rbnode &node)
{
	int	i;
	Rbnode *p = end.leftchild;

	while (p != &end)
	{
		i = p->lookupCmp(node);
		if (i > 0)
			p = p->leftchild;
		else if (i < 0)
			p = p->rightchild;
		else
			return p;
	}
	return 0; // not found
}

// static 
void
rotate(Rbnode *g, Rbnode *p, Rbnode *c)	// rotate c above p 
{
	//
	// 4 cases: p is g's leftchild or rightchild child
	//      and c is p's leftchild or rightchild child
	//
	if (p == g->leftchild)
		g->leftchild = c;
	else
		g->rightchild = c;
	if (c == p->leftchild)
	{
		p->leftchild = c->rightchild;
		c->rightchild = p;
	}
	else
	{
		p->rightchild = c->leftchild;
		c->leftchild = p;
	}
}

// static 
Rbnode *
split(Rbnode *gg, Rbnode *g, Rbnode *p, Rbnode *c)
{
	if (c->color != Red)	// nonleaf node 
	{
		c->leftchild->color = Black;
		c->rightchild->color = Black;
		if (g == 0)
			return c;	// c is the root
		c->color = Red;
	}
	if (p->color == Red)	// need to fix tree 
	{
		if ((c == p->leftchild) == (p == g->leftchild))
			c = p;
		else	// double rotation needed 
			rotate(g, p, c);
		rotate(gg, g, c);
		g->color = Red;
		c->color = Black;
	}
	return c;
}

Rbnode *
Rbtree::tinsert(Rbnode &node)
{
	Rbnode *gg = 0, *g = 0;	// greatgrandparent and grandparent 
	Rbnode *p = &end, *c = p->leftchild;	// parent and child 
	int dir = -1;		// start on the leftchild 

	if (c == &end)
	{
		c = node.makenode();
		c->leftchild = &end;
		c->rightchild = &end;
		p->leftchild = c;
		firstnode = lastnode = c;
		c->prevnode = c->nextnode = 0;
		c->color = Black;
		return c;
	}
	while (c != &end)
	{
		if (c->leftchild->color == Red && c->rightchild->color == Red)
		{
			c = split(gg, g, p, c);
			g = p = c;
		}
		gg = g;
		g = p;
		p = c;
		dir = node.cmp(*c);
		if (dir < 0)
		{
			c = c->leftchild;
		}
		else if (dir > 0)
		{
			c = c->rightchild;
		}
		else
			return 0;	// already here
	}
	// Add new node c below p.

	c = node.makenode();
	c->leftchild = &end;
	c->rightchild = &end;
	if (dir < 0)
	{
		p->leftchild = c;
		if (p->prevnode)
		{
			c->prevnode = p->prevnode;
			p->prevnode->nextnode = c;
		}
		else
		{
			firstnode = c;
			c->prevnode = 0;
		}
		p->prevnode = c;
		c->nextnode = p;
	}
	else
	{
		p->rightchild = c;
		if (p->nextnode)
		{
			c->nextnode = p->nextnode;
			p->nextnode->prevnode = c;
		}
		else
		{
			lastnode = c;
			c->nextnode = 0;
		}
		p->nextnode = c;
		c->prevnode = p;
	}

	c->color = Red;
	(void)split(gg, g, p, c);
	return c;
}
