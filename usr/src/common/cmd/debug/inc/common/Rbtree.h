#ifndef Rbtree_h
#define Rbtree_h
#ident	"@(#)debugger:inc/common/Rbtree.h	1.3"

//	Red-Black implementation of a 2-3-4 tree.
//	Nodes are also threaded for sequential access to entire tree
//	
//	A red-black tree is a binary tree in which localized
//	node groups correspond to single nodes in a 2-3-4 tree.
//	A 2-node is represented as a regular (black) node.
//	A 3-node is represented as a black node above a red node,
//	either as the left or right child.
//	A 4-node is represented as a black node above 2 red nodes.
//	Thus, the root is a black node and a red node can't have a 
//	red parent.  The height is at most 2log2n.

enum	tcolor	{ Red, Black };
class	Rbtree;

class Rbnode
{
	Rbnode*		leftchild;
	Rbnode*		rightchild;
	Rbnode*		nextnode;	// links in list used to walk
	Rbnode*		prevnode;	// the tree in order quickly
	tcolor		color;
	friend class	Rbtree;
	friend void	rotate(Rbnode *, Rbnode *, Rbnode *);
	friend Rbnode 	*split(Rbnode *, Rbnode *, Rbnode *, Rbnode *);
public:
	Rbnode() { leftchild = rightchild = nextnode = prevnode = 0; }
	~Rbnode() {}

	virtual Rbnode*	makenode();	// save this in tree memory.
	virtual	int	cmp(Rbnode &node);	// (t)insert compare rtn
	virtual int	lookupCmp(Rbnode &node); // (t)lookup compare rtn ...
						 // ... usually cmp
					
	
	Rbnode*		next() { return nextnode; }
	Rbnode*		prev() { return prevnode; }
};

class Rbtree
{
	Rbnode*		firstnode;
	Rbnode*		lastnode;
	Rbnode		end; // above root and below all leaves
public:

	Rbnode*		tfirst() { return firstnode; }
	Rbnode*		tlast() { return lastnode; }
	Rbnode*		tinsert(Rbnode & node);
	Rbnode*		tlookup(Rbnode & node);
	Rbtree();
	~Rbtree();
};

#endif // end of Rbtree.h
