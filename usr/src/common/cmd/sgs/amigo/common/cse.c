#ident	"@(#)amigo:common/cse.c	1.13"
#include "amigo.h"
#include "costing.h"
#include <unistd.h>
#ifndef NO_CSE

/* Global Common Subexpression Elimination 
**
** This file eliminates global common subexpressions which have a
** single source.  In practice, this handles most of the cases
** (more than 95%, I believe).  First we compute available expressions
** and then we build lists: the head of the list is a source, the
** remaining items are recalculations of the same (available) expression.
*/

typedef struct cse_source {
	struct occur_info *first;
	struct cse_source *next;
} cse_source;

typedef struct occur_info {
	ND1 *occur;
	ND1 *parent;
	struct occur_info *next;
	Cgq_index flow;
	Block block;
	unsigned int flag;
} occur_info;

/* Values for flag in occur_info */

#define DEAD		01
#define ASSIGN_TARGET	02

static void flush_occurs(), add_occur(), replace_expr_by_cse(),
	    local_opt(), available_expressions(),
	    e_gen_kill(), get_gen(), get_source(), drive(),
	    look_for_source(), flush_all(), local_cse(),
	    flush_cse(), kill_cse();
static int worthit();
static cse_source *find_cse();
static occur_info *find_occur();
static struct Block *single_source();

#ifndef NODBG
static void assert_id();
#define ASSERT_ID(src, id)	assert_id(src, id)
#else
#define ASSERT_ID(src, id)
#endif

/* In each block, current_source[i-1] can have one of the following values:
**
** 0:     Expr i has not been calculated nor killed in the current block.
** -1:    Expr i has been killed in the current block.
** other: The source for the expr i.  It may be global or local.
*/
static cse_source **current_sources;

/* cse_table[i-1] is a global list of sources for expr i.  Expr i may
** be a candidate for unrelated cse elimination; that is why we have
** a list of sources.
*/
static cse_source **cse_table;

static Arena cse_arena;
static Expr_set curr_kills;
static Bit_vector saw;

/* The following sets give available, generated, and killed
** information per block.  All the exprs in e_gen are suitable
** for a source.  Thus, if expr i is calculated on one branch
** of a ?: operator, i will not be in e_gen.  
*/
static Expr_set *e_avail, *e_gen, *e_kill;
static TEMP_SCOPE_ID global_temp_scope;

static int ob_count;
void
global_cse(blocks)
struct Block *blocks;
{
	int i;
	int count = 0;
	DEBUG(aflags[cse].level&8,("\nTrees before global cse\n"));
	DEBUG_COND(aflags[cse].level&8,print_trees(3, blocks));

	/* Initialize a bunch of globals */

	ob_count = count = order_blocks(blocks);

	cse_arena = arena_init();

	global_temp_scope = get_global_temp_scope();

	e_gen = Arena_alloc(cse_arena, count, Expr_set);
	e_kill = Arena_alloc(cse_arena, count, Expr_set);
	e_avail = Arena_alloc(cse_arena, count, Expr_set);
	for (i = 0; i < count; i++)
		e_avail[i] = 0;

	saw = bv_alloc(count, cse_arena);
	
	curr_kills = Expr_set_alloc(cse_arena);	

	current_sources = Arena_alloc(cse_arena, EXPR_SET_SIZE, cse_source *);

	cse_table = Arena_alloc(cse_arena, EXPR_SET_SIZE, cse_source *);
	memset(cse_table, 0, EXPR_SET_SIZE * sizeof(cse_source *));

	/* Calculate available expressions */
	available_expressions(blocks, count);

	/* Calculate expression anticipation */
	locally_anticipate(cse_arena);
	anticipate(cse_arena);
	
	/* Process the blocks */
	drive(blocks);

	/* We are done */
	arena_term(cse_arena);
	DEBUG(aflags[cse].level&8,("\nTrees after global cse\n"));
	DEBUG_COND(aflags[cse].level&8,print_trees(3, blocks));
}

static void
drive(blocks)
struct Block *blocks;
{
	/*
	** The reachable blocks may be processed in any order. 
	** But unreachable blocks should not be processed
	** as some data may be garbage for these blocks.
	*/

	DEPTH_FIRST(blk_ptr)
		local_cse(*blk_ptr);
	END_DEPTH_FIRST
	
	flush_all();
}

static void
local_cse(block)
struct Block *block;
{
	DEBUG(aflags[cse].level&4,("\n++ cse: block %d\n",block->block_count));
	memset(current_sources, 0, EXPR_SET_SIZE * sizeof(cse_source *));

	/* work through every flownode in block: cse */
	FOR_ALL_ND1_IN_BLOCK(block, flow, node, index)

		Expr assign_targ;

		/* flush CSE's killed by effects in left and
		** right subtrees
		*/
		bv_init(false,curr_kills);
		get_recursive_kills(curr_kills,node->left,ACCUMULATE);
		get_recursive_kills(curr_kills,node->right,ACCUMULATE);
		flush_occurs(curr_kills);

		/* check if lhs of assign can be cse source */
		switch (node->op) {
		ASSIGN_CASES:
			if ( EXCEPT_OPT(node->left) && 
				node->type == node->left->type &&
				!bv_belongs(node->left->opt->setid, curr_kills)
			) 
				assign_targ = node->left->opt;
			else
				assign_targ = 0;
			break;
		default:
			assign_targ = 0;
		}

		local_opt(index, node, curr_kills, 0, block);

		bv_init(false,curr_kills);
		if (CONTAINS_SIDE_EFFECTS(node)) {
			get_kills(curr_kills, node, ACCUMULATE);
			flush_occurs(curr_kills);
		}

		/* allow lhs of assignment as cse source
		** use a saved value incase assign_targ change by
		** a cse
		*/
		if (assign_targ) {
			DEBUG(aflags[cse].level&4,("\nassign target source"));
			DEBUG_COND(aflags[cse].level&4,print_expr(assign_targ));
			add_occur(index, node->left, node, assign_targ, block, ASSIGN_TARGET);
		}

	END_FOR_ALL_ND1_IN_BLOCK
}

/* Flush all cse's in cse_table.  The cse's are processed by first
** doing the largest expressions and then subexpressions of the
** larger expressions.  The order is guaranteed by the expression
** numbers (expressions are numbered in depth-first order).
*/

static void
flush_all()
{
	int i;
	cse_source *src;

	for (i=EXPR_SET_SIZE-1; i>=0; i--) {
		src = cse_table[i];
		while (src) {
			/* Do not try to do a cse if the source has
			** been replaced by a previous cse.
			*/
			if (!(src->first->flag & DEAD))
				flush_cse(SETID_TO_EXPR(i+1), src->first);
			src = src->next;
		}
	}
}
		
static void
local_opt(root,node,kills,context,block)
Cgq_index root;
ND1 *node; 
Expr_set kills; 	/* do not optimize these expressions */
int context;
struct Block *block;
{
	int id = node->opt->setid;
	int first_occur = 0;
	int optable;

	if (EXCEPT_OPT(node)) {
		switch ((int)current_sources[id-1]) {

		case -1:	
			first_occur = 1;
			break;
		case 0:		
			first_occur = !single_source(id, block);
			break;
		}
	}

	optable =
		/* optimizable and not killed */
		EXCEPT_OPT(node) && !bv_belongs(id,kills)

		/* is an RVAL */
		&& !(context&IS_LVAL)
		&& !((context&IS_COND) && first_occur);

	if (node->left)
		local_opt(root,node->left,kills, 
			get_context(node,LEFT,context), block);
	if (node->right)
		local_opt(root,node->right,kills, 
			get_context(node,RIGHT,context), block);

	if (optable) {
		DEBUG(aflags[cse].level&4,
			("\nexpr %d: first_occur = %d\n", id, first_occur));
		add_occur(root,node, NULL, node->opt, block, 0);
	}
}

/* Add an occurrence of cse e->setid */

static void
add_occur(index,node,parent,e,block,flags)
Cgq_index index;
ND1 *node, *parent;
struct Expr_info *e;
struct Block *block;
unsigned int flags;
{
	occur_info *occurs;
	cse_source *source;
	Cgq_index sflow;
	ND1 *snode;
	struct Block *sblock;

	/* Must be optimizable to be here */

	assert(node->opt->setid == e->setid);

	if (!(e->flags & EXPENSIVE))
		return;

	switch ((int)current_sources[e->setid-1]) {

	case 0:
		/* First time we have seen this expression in this
		** block.  See if it is available globally.  If it 
		** is, "prime" the source of the cse by adding an 
		** occurrence corresponding to the source.  If it
		** is not available globally, use this occurence
		** as a source.
		*/
		sblock = single_source(e->setid,block);
		if (sblock) {
			DEBUG(aflags[cse].level&4,("\nadd global occurence "));
			DEBUG_COND(aflags[cse].level&4,print_expr(e));
			get_source(e->setid, &sflow, &snode, sblock);
			source = find_cse(e->setid, snode);
			if (!source) {
				DEBUG(aflags[cse].level&4,("\nprime global source "));

				source = Arena_alloc(cse_arena, 1, cse_source);
				occurs = Arena_alloc(cse_arena, 1, occur_info);
				source->next = cse_table[e->setid-1];
				cse_table[e->setid-1] = source;
				source->first = occurs;
				occurs->occur = snode;
				occurs->flow = sflow;
				occurs->block = sblock;
				occurs->next = NULL;
				occurs->flag = 0;
			}
			/* Add new occurrence */

			occurs = Arena_alloc(cse_arena, 1, occur_info);
			occurs->flag = 0;
			occurs->next = source->first->next;
			source->first->next = occurs;
			current_sources[e->setid-1] = source;
			break;
		}
		/* FALLTHRU */
	case -1:
		/* We have a new source.  If we did not fall through
		** from the above case, the expression was previously
		** killed in this block, and this is a new source.  
		** Otherwise, it is the first time we have seen the
		** expression in this block, and it was unavailable
		** globally from a single source.
		*/
		if ((source = find_cse(e->setid, node)) != NULL) {
			/* This is a source for a previously primed
			** global cse.  It is already in the table.
			*/
			assert(source->first->occur == node);
			current_sources[e->setid-1] = source;
			ASSERT_ID(source->first, e->setid);
			return;
		}
		DEBUG(aflags[cse].level&4,("\nadd local source "));
		DEBUG_COND(aflags[cse].level&4,print_expr(e));
		source = Arena_alloc(cse_arena, 1, cse_source);
		source->first = occurs = Arena_alloc(cse_arena, 1, occur_info);
		source->next = cse_table[e->setid-1];
		cse_table[e->setid-1] = current_sources[e->setid-1] = source;
		occurs->next = NULL;
		break;
	default:
		/* Add to list of occurences.  We already know
		** the source of the cse.
		*/
		DEBUG(aflags[cse].level&4,("\nadd local occurrence "));
		DEBUG_COND(aflags[cse].level&4,print_expr(e));
		occurs = Arena_alloc(cse_arena, 1, occur_info);
		source = current_sources[e->setid-1];
		occurs->next = source->first->next;
		source->first->next = occurs;
		break;
	}
	occurs->occur = node;
	occurs->flow = index;
	occurs->block = block;
	occurs->flag = flags;
	occurs->parent = parent;
	ASSERT_ID(source->first, e->setid);
}

static int 
cse_in_tree(tree, id)
ND1 *tree;
int id;
{
	/* Check to see if CSE is referenced in tree.  The tree
	** is not yet canonicalized.
	*/
	switch(optype(tree->op)) {
	case LTYPE:
		return tree->op == CSE && (!id || tree->c.size == id);
	case BITYPE:
		if (cse_in_tree(tree->right, id))
			return 1;
		/* FALLTHRU */
	case UTYPE:
		return cse_in_tree(tree->left, id);
	}
	/* NOTREACHED */
}

static void
push_down_let(root)
ND1 *root;
{
	/* Push down LET node as far as it can go */

	ND1 *r = root->right;
	ND1 tmp;

	/* Don't push LET past a CALL: cg can't handle it */

	if (callop(r->op))
		return;

	if (r->left && !cse_in_tree(r->left, (int)root->c.size)) {

		/* push LET node to right of current right */

		tmp = *root;
		*root = *r;
		*r = tmp;

		/* root of new tree is still root */

		r->right = root->right;
		root->right = r;
		r->type = r->right->type;
		
		DEBUG(aflags[cse].level&2,("\nPushed LET to right:\n")); 
		DEBUG_COND(aflags[cse].level&2,dprint1(root));
		push_down_let(r);
	}
	else if (r->right && !asgop(r->op) && !cse_in_tree(r->right, (int)root->c.size)) {

		/* push LET node to left of current right */

		tmp = *root;
		*root = *r;
		*r = tmp;

		/* root of new tree is still root */

		r->right = root->left;
		root->left = r;
		r->type = r->right->type;

		DEBUG(aflags[cse].level&2,("\nPushed LET to right:\n")); 
		DEBUG_COND(aflags[cse].level&2,dprint1(root));
		push_down_let(r);
	}
	return;
}

static void
commute_cse(n)
ND1 *n;
{
	ND1 *tmp = n->left;
	n->left = n->right;
	n->right = tmp;
}

static void
optimize_cse(top)
ND1 *top;
{
	/* Since CSE nodes are like register nodes, we would like
	** to canonicalize the trees to simplify efficient
	** template writing.
	*/
	switch(optype(top->op)) {
	case BITYPE:
		/* Catch potential addressing modes */
		if (top->op == PLUS) {

			/* Want &AUTO + CSE */
			if (	top->left->op == CSE && 
				TY_ISPTR(top->type)  &&
				top->right->op == ICON &&
				top->right->sid != ND_NOSYMBOL
			) {
				commute_cse(top);
			}

			/* Want CSE + ICON (not address ICON) */
			else if (top->right->op == CSE &&
				TY_ISPTR(top->type)  &&
				top->left->op == ICON &&
				top->left->sid == ND_NOSYMBOL
			) {
				commute_cse(top);
			}

			/* Want CSE + (reg << ICON) */
			else if (top->right->op == CSE && 
				top->left->op == LS
			) {
				commute_cse(top);
			}
		}
		optimize_cse(top->right);
		/* FALLTHRU */
	case UTYPE:
		optimize_cse(top->left);
		/* FALLTHRU */
	case LTYPE:
		return;
	}
}

static TEMP_SCOPE_ID
current_scope(occurs)
occur_info *occurs;
{
	struct Block *start, *end;
	start = end = occurs->block;
	occurs = occurs->next;	/* src cannot be dead at this point */
	while (occurs) {
		if (!(occurs->flag & DEAD)) {
			if (occurs->block->lex_block_num > end->lex_block_num)
				end = occurs->block;
			if (occurs->block->lex_block_num < start->lex_block_num)
				start = occurs->block;
		}
		occurs = occurs->next;
	}
	DEBUG(aflags[cse].level&4,("lexical cse scope: %d --> %d\n", 
			start->lex_block_num, 
			end->lex_block_num)); 

	/* The following code prevents an amigo allocated temp
	** from being mapped to the same stack location
	** as another live local variable.  We think this only
	** happens when there are jumps back into the segment
	** of the CGQ over which the cse is being done.
	*/
	CGQ_FOR_ALL_BETWEEN(flow, index, start->first, end->last)
		if(flow->cgq_op == CGQ_EXPR_ND2 &&
			flow->cgq_arg.cgq_nd2->op == LABELOP)
			return al_create_scope(global_temp_scope.first,
				global_temp_scope.last);
	CGQ_END_FOR_ALL_BETWEEN

	FOR_ALL_ND1_IN_BLOCK(start, flow, node, index)
		return al_create_scope(index, end->last);
	END_FOR_ALL_ND1_IN_BLOCK
	cerror(gettxt(":575","No executable code found in block %d"), start->lex_block_num);
	/* NOTREACHED */
}

static void
flush_occurs(kills)
Expr_set kills;
{
	BV_FOR_REVERSE(kills, i)

		current_sources[i-1] = (cse_source *)(-1);

	END_BV_FOR
}

static void
flush_cse(e, source)
struct Expr_info *e;
occur_info *source;
{
	/* This function performs the tree manipulation necessary for
	** cse's.  It is only called if the source of the cse is not
	** DEAD, though some occurences of the cse may be DEAD.  The
	** function worthit() is called to determine whether it is
	** cost-effective to perform the cse.  worthit() also removes 
	** the DEAD occurences, so we do not check the flag after
	** calling worthit().
	*/
	int intra_cse = 0;
	int expensive = 0;
	occur_info *curr;
	TEMP_SCOPE_ID scope;

	DEBUG(aflags[cse].level&4,("\nTest cse ")); 
	DEBUG_COND(aflags[cse].level&4,print_expr(e));
	if (!worthit(source, &intra_cse, &expensive))
		return;

	scope = current_scope(source);

	START_OPT

	if (intra_cse == 2) {

		/* CSE only appears in single statement */

		ND1 *old_top;
		ND1 *cs = t1alloc();
		int copyok = expensive ? 0 : FF_COPYOK;
		static int id = 0;
		ND1 *top = CGQ_ELEM_OF_INDEX(source->flow)->cgq_arg.cgq_nd1;

		kill_cse(source->occur->left);
		kill_cse(source->occur->right);

		if (top->op == SWBEG)
			top = top->left;

		*cs = *source->occur;

		/* replace top of tree by a LET, lhs is cse, rhs is old top */
		old_top = t1alloc();
		*old_top = *top;

		top->op = LET;
		top->left = cs;
		top->right = old_top;
		top->flags = old_top->flags | FF_SEFF | copyok;
		top->type = old_top->type;
		top->c.size = ++id;
		source->occur->op = CSE;
		source->occur->c.size = top->c.size;
		for(curr = source->next; curr; curr=curr->next) {
			kill_cse(curr->occur->left);
			kill_cse(curr->occur->right);
			replace_expr_by_cse(curr->occur, top->c.size);
		}
		DEBUG(aflags[cse].level,("\nPerformed local cse:\n")); 
		DEBUG_COND(aflags[cse].level,dprint1(top->left));
		optimize_cse(top);
		push_down_let(top);
		new_expr(top, source->block);
		return;
	}
	else if (intra_cse && !(source->flag & ASSIGN_TARGET)) {
		ND1 *cse;
		ND1 *top =CGQ_ELEM_OF_INDEX(source->flow)->cgq_arg.cgq_nd1;
		ND1 *old_top;

		/* Abort if there is a CSE node in the first occurence.  We
		** cannot move this node above the LET node.  In the future,
		** find a more elegant way of doing this instead of just
		** passing on the cse.
		*/
		if (cse_in_tree(source->occur, 0))
			return;

		if (top->op == SWBEG)
			top = top->left;

		/* Make cse the assignment of cse to temp. */

		cse = t1alloc();
		*cse = *source->occur;

		/* Following since we just copied children of src. */

		source->occur->left = source->occur->right = 0;
		store_expr(e, scope, cse);

		/* Replace top of tree by a COMOP, lhs is cse assign,
		** rhs is old top.
		*/
		old_top = t1alloc();
		*old_top = *top;

		top->op = COMOP;
		top->left = cse;
		top->right = old_top;
		top->flags = old_top->flags;
		top->flags |= FF_SEFF;
		top->type = old_top->type;

		/* Replace CSE by temp */

		replace_expr_by_temp(e, scope, source->occur);
	}
	else {  /* intra_cse == 0 */
		ND1 *top, *source_node = source->occur;
		if (source->flag & ASSIGN_TARGET)
			source_node = source->parent;
		if (source_node->op == INCR)
			source_node->op = ASG PLUS;
		else if (source_node->op == DECR)
			source_node->op = ASG MINUS;
		store_expr(e, scope, source_node);

		top =CGQ_ELEM_OF_INDEX(source->flow)->cgq_arg.cgq_nd1;
		if (top->op == SWBEG)
			top = top->left;
		new_expr(top, source->block);
	}
	DEBUG(aflags[cse].level,("\nPerforming cse for ")); 
	DEBUG_COND(aflags[cse].level,print_expr(e));
	DEBUG(aflags[cse].level,
		("\tNode #%d (line=%d) stored into temp(%d)\n",
		node_no(source->occur),
		CGQ_ELEM_OF_INDEX(source->flow)->cgq_ln,e->temp));

	for(curr = source->next; curr; curr=curr->next) {
		ND1 *top;
		DEBUG(aflags[cse].level,
			("\tNode #%d (line=%d) replaced by temp\n",
			node_no(curr->occur),
			CGQ_ELEM_OF_INDEX(curr->flow)->cgq_ln));
		kill_cse(curr->occur->left);
		kill_cse(curr->occur->right);
		replace_expr_by_temp(e, scope, curr->occur);
		top =CGQ_ELEM_OF_INDEX(curr->flow)->cgq_arg.cgq_nd1;
		if (top->op == SWBEG)
			top = top->left;
		new_expr(top, curr->block);
	}

	STOP_OPT
}

static occur_info *
find_occur(id, node)
int id;
ND1 *node;
{
	cse_source *source = cse_table[id-1];
	occur_info *o;
	
	while (source) {
		for (o = source->first; o; o = o->next)
			if (o->occur == node && !(o->flag & DEAD))
				return o;
		source = source->next;
	}
	return NULL;
}

static cse_source *
find_cse(id, node)
int id;
ND1 *node;
{
	cse_source *source = cse_table[id-1];
	occur_info *o;
	
	while (source) {
		for (o = source->first; o; o = o->next)
			if (o->occur == node && !(o->flag & DEAD))
				return source;
		source = source->next;
	}
	return NULL;
}

static void
kill_cse(n)
ND1 *n;
{
	/* Remove all subtrees as candidates for sources/occurences of cse. */

	occur_info *o;
	if (!n) return;
	if (EXCEPT_OPT(n) && (o = find_occur(n->opt->setid, n)) != NULL)
		o->flag |= DEAD;
	if (n->left) kill_cse(n->left);
	if (n->right) kill_cse(n->right);
}

#ifndef CSE_IS_OK
#define CSE_IS_OK(node)	1
#endif

/* The theory behind the macro WORTHIT is that the cost of doing a cse
** is partially offset by the chance that the cse will not get a register
** and will have to be stored and then reloaded from memory.  If we make
** the assumption that the cost of a load is equal to the cost of a store,
** the equation simplifies to WORTHIT, since we save 
**
**		COST(expr) - COST(load)
**
** per occurence.  On the other hand, the first evaluation will result
** in an extra store, so the cost we save from the above expression
** must be greater that the cost of a store.  Assuming
**
** 		COST(load) == COST(store)
**
** then
**
**		(count-1) * COST(expr) > count * COST(load)
**
** must be true the do the cse.  On a register rich machine, we can
** assume COST(load) == 0.  Then above expression will always be true.
**
** worthit() also removes DEAD occurences from the occurnce list, thus
** later functions need not check the flag.
*/

/* FORGET THE THEORY.  THIS SEEMS TO WORK BETTER FOR THE TIME
** BEING.  PEROFORMANCE ANALYSIS WILL CLEAN THIS UP.
*/

#define WORTHIT(count, cost, type, anticipated)	( \
		anticipated ? \
			((cost) > cse_threshold + LOAD_COST(type)) : \
			((cost) > 2 * (cse_threshold + LOAD_COST(type))) \
	)

/*
		(((count-1) * (cost)) > ((count) * (LOAD_COST(type))))
*/

static int
worthit(occurs, intra_cse, expensive)
occur_info *occurs;
int *intra_cse;
int *expensive;
{
	occur_info *last, *first_occur = occurs;
	int inter_cse=0;
	int occur_cnt;

	/* The anticipated flag tells us whether we expect to recompute the
	** expression always.  It set if one of the following hold:
	**
	**	1. It is recomputed in the same block as the source.
	**	2. It is anticipated by all the successor blocks of the source.
	**
	** The first condition will catch some incorrect cases if the
	** expression is only recomputed conditionally in the same block
	** and condition two does not hold.  Also, if we have performed
	** any cse, the anticipates information may be slightly out of
	** date.  
	*/
	int anticipated = 0;

	*expensive = 0;
	last = first_occur;
	occur_cnt = 1;
	for ( occurs=first_occur->next; occurs; occurs=occurs->next) {
		if (occurs->flag & DEAD) {
			last->next = occurs->next;
			continue;
		}
		if (first_occur->block == occurs->block)
			anticipated = 1;
		++occur_cnt;
		last = occurs;
	}

	if (occur_cnt <= 1)
		return 0;

	if (!anticipated) {
		struct Block_list *succ;
		int id = first_occur->occur->opt->setid;
		for (succ=first_occur->block->succ; succ; succ=succ->next) {
			if (!bv_belongs(id, succ->block->antic)) {
				anticipated = 0;
				break;
			}
			anticipated = 1;
		}
	}

	if (WORTHIT(	occur_cnt, 
			COST(first_occur->next->occur),
			first_occur->next->occur->type,
			anticipated
		   )
	   )
		*expensive = 1;

	for ( occurs = first_occur->next;  occurs; occurs=occurs->next) {
		if (first_occur->flow == occurs->flow)
			*intra_cse = 1;
		else
			inter_cse = 1;
	}

	if (!inter_cse && *intra_cse)
		*intra_cse = 2;

	if (*expensive)
		return 1;

	/* CONSTANTCONDITION */
	if (*intra_cse == 2 && CSE_IS_OK(first_occur->occur)) 
		return 1;

	return 0;
}

static void
replace_expr_by_cse(targ, id)
ND1 *targ;
int id;
{
        if(targ->left)
		tfree((NODE *)(targ->left));
        if(targ->right)
                tfree((NODE *)(targ->right));
        targ->op = CSE;
        targ->c.size = id;
	targ->left = targ->right = NULL;
        return;
}

/**************************************************
**
**	Calculate available expressions
**
**	generate and kill information for basic
**	block b is left in e_gen[b] and e_kill[b]
**
**	available expressions upon entering basic
**	block b are left in e_avail[b].
**
**	The algorithm is taken from the Aho, Sethi
**	and Ullman "Dragon Book"
*/

static void
available_expressions(blocks, count)
struct Block *blocks;
int count;
{
	struct Block *b;
	Expr_set *out, oldout;
	int change;
	Arena a;
	int i;

	a = arena_init();
	out = Arena_alloc(a, count, Expr_set);

	oldout = Expr_set_alloc(a);
	e_gen_kill(blocks);

	e_avail[0] = Expr_set_alloc(a);
	out[0] = Expr_set_alloc(a);
	
	bv_init(false, e_avail[0]);
	bv_assign(out[0], e_gen[0]);
	for (b=blocks->next; b; b=b->next) {
		int i = b->lex_block_num - 1;
		e_avail[i] = Expr_set_alloc(a);
		out[i] = Expr_set_alloc(a);
		bv_init(true, out[i]);
		bv_minus_eq(out[i], e_kill[i]);
	}
	do {

		change = 0;

		DEPTH_FIRST(blk_ptr)
			struct Block *b = *blk_ptr;
			int i = b->lex_block_num - 1;
			struct Block_list *pred = b->pred;

			/* Intersect all out[p] such that
			** p is a predecessor of b.
			*/
			if (!pred)
				bv_init(false, e_avail[i]);	
			else
				bv_init(true, e_avail[i]);

			while (pred) {
				int p = pred->block->lex_block_num - 1;
				bv_and_eq(e_avail[i], out[p]);
				pred = pred->next;
			}
			bv_assign(oldout, out[i]);
			bv_assign(out[i], e_avail[i]);
			bv_minus_eq(out[i], e_kill[i]);
			bv_or_eq(out[i], e_gen[i]);
			if (!change && !bv_equal(out[i], oldout)) {
				change = 1;
				DEBUG(aflags[cse].level&48,
					("\nBlock %d differs\n", i+1)); 
				DEBUG(aflags[cse].level&48, ("old out:")); 
				DEBUG_COND(aflags[cse].level&48,
					bv_print(oldout));
				DEBUG(aflags[cse].level&48, ("avail:"));
				DEBUG_COND(aflags[cse].level&48,
					bv_print(e_avail[i]));
				DEBUG(aflags[cse].level&48, ("gen:"));
				DEBUG_COND(aflags[cse].level&48,
					bv_print(e_gen[i]));
				DEBUG(aflags[cse].level&48, ("kill:"));
				DEBUG_COND(aflags[cse].level&48,
					bv_print(e_kill[i]));
				DEBUG(aflags[cse].level&48, ("new out:"));
				DEBUG_COND(aflags[cse].level&48,
					bv_print(out[i]));
			}
				
		END_DEPTH_FIRST
#ifndef NODBG
		if (aflags[cse].level&48) { 
			fprintf(stderr, "Percolating available exprs\n\n");
			for(b=blocks; b; b=b->next) {
				fprintf(stderr, "Block %d: ", b->block_count);
				bv_print(e_avail[b->lex_block_num-1]);
			}
		}
#endif
	} while (change);
#ifndef NODBG
	if (aflags[cse].level&32) {
		fprintf(stderr, "*** Available Expressions\n\n");
		for(b=blocks; b; b=b->next) {
			fprintf(stderr, "Block %d: avail: ", b->block_count);
			bv_print(e_avail[b->lex_block_num-1]);
			fprintf(stderr, "Block %d: gen: ", b->block_count);
			bv_print(e_gen[b->lex_block_num-1]);
			fprintf(stderr, "Block %d: kill: ", b->block_count);
			bv_print(e_kill[b->lex_block_num-1]);
		}
	}
#endif
	e_avail[0] = bvclone(cse_arena, e_avail[0]);
	for (i = 1; i < ob_count; i++) {
		if (!e_avail[i])
			continue;
		if (e_avail[i-1] && bv_equal(e_avail[i], e_avail[i-1]))
			e_avail[i] = e_avail[i-1];
		else
			e_avail[i] = bvclone(cse_arena, e_avail[i]);
	}
	arena_term(a);
}

#define N 25 
static Expr_set cache[N];
static int last;
static void
flush_cache()
{
	int i;

	for (i = 0; i < N; i++)
		cache[i] = 0;
	last = 0;
}

static Expr_set
check_cache(bv)
Expr_set bv;
{
	int slot;
	int i;

	for (i = 0; i < N; i++) {
		if (!cache[i])
			break;
		if (bv_equal(cache[i], bv))
			return cache[i];
	}
	slot = last;
	if (++last >= N)
		last = 0;
	cache[slot] = bvclone(cse_arena, bv);
	return cache[slot];
}

static void
e_gen_kill(blocks)
struct Block *blocks;
{
	Expr_set kill;
	Expr_set gen;
	Expr_set scr1, scr2;
	int i;

	flush_cache();

	kill = Expr_set_alloc(cse_arena);
	gen = Expr_set_alloc(cse_arena);

	scr1 = Expr_set_alloc(cse_arena);
	scr2 = Expr_set_alloc(cse_arena);

	while (blocks) {

		int b = blocks->lex_block_num - 1;

		bv_init(false, scr1);
		bv_init(false, scr2);

		FOR_ALL_ND1_IN_BLOCK(blocks, flow, node, index)

			bv_init(false, kill);
			bv_init(false, gen);

			get_gen(gen, node, 0);
			get_recursive_kills(kill, node, ACCUMULATE);

			bv_or_eq(scr1, gen);
			bv_or_eq(scr2, kill);
			bv_minus_eq(scr1, kill);

		END_FOR_ALL_ND1_IN_BLOCK

		e_gen[b] = check_cache(scr1);
		e_kill[b] = check_cache(scr2);

		DEBUG(aflags[cse].level&48, ("\n\nblock %d:\n", b+1));
		DEBUG(aflags[cse].level&48, ("\n\ngen: "));
		DEBUG_COND(aflags[cse].level&48, bv_print(e_gen[b]));
		DEBUG(aflags[cse].level&48, ("\n\nkill: "));
		DEBUG_COND(aflags[cse].level&48, bv_print(e_kill[b]));

		blocks = blocks->next;
	}
}

static void
get_gen(gen, node, context)
Expr_set gen;
ND1 *node;
int context;
{
	if (EXCEPT_OPT(node) && !(context&IS_COND))
		bv_set_bit(node->opt->setid, gen);
	if (node->left) 
		get_gen(gen, node->left, get_context(node,LEFT,context));
	if (node->right)
		get_gen(gen, node->right, get_context(node,RIGHT,context));
}

/*
**	Test to see if Global CSE has a single source
*/
static int sources;

static struct Block *
single_source(id, block)
int id;
struct Block *block;
{
	struct Block *source_block;
	if (!bv_belongs(id, e_avail[block->lex_block_num-1]))
		return 0;
	sources = 0;
	bv_init(false, saw);
	look_for_source(id, block, &source_block);
	assert(sources >= 1);
	DEBUG(aflags[cse].level&4,("\n%d sources for id %d in block %d",
					sources, id, block->block_count)); 
	return sources == 1 ? source_block : 0;
}

static void
look_for_source(id, block, source_block)
int id;
struct Block *block;
struct Block **source_block;
{
	struct Block_list *pred = block->pred;

	while (pred) {
		int p = pred->block->lex_block_num;
		if (!bv_belongs(p, saw)) {
			
			bv_set_bit(p, saw);
			/* Conditions for source:
			**
			**	1. Generated by this block and killed
			**	   by this block.
			**
			**	2. Generated by this block, not available
			**	   going into this block.
			*/
			if (!bv_belongs(id, e_gen[p-1]))
				look_for_source(id, pred->block, source_block);
			else if ( bv_belongs(id, e_kill[p-1]) ||
				 !bv_belongs(id, e_avail[p-1])
				) {
				sources++;
				*source_block = pred->block;
			}
			else
				look_for_source(id, pred->block, source_block);
		}
		pred = pred->next;
	}
}

static ND1 *
check_tree(id, n, context)
int id;
ND1 *n;
int context;
{
	ND1 *ret = 0;
	if (EXCEPT_OPT(n) && n->opt->setid == id && !(context & IS_COND))
		return n;
	if (n->left) 
		ret = check_tree(id, n->left, get_context(n,LEFT,context));
	if (ret)
		return ret;
	if (n->right)
		ret = check_tree(id, n->right, get_context(n,RIGHT,context));
	return ret;
}

static void
get_source(id, flow, node, block)
int id;
Cgq_index *flow;
ND1 **node;
struct Block *block;
{
	/* Find id in source_block, fill in flow and node */

	FOR_ALL_ND1_IN_BLOCK_REVERSE(block, f, n, i)

		if ((*node = check_tree(id, n, 0)) != NULL) {
			*flow = i;
			return;
		}

	END_FOR_ALL_ND1_IN_BLOCK
	
	cerror(gettxt(":576","cse source not found in block"));
}

#ifndef NODBG
static void
assert_id(source, id)
occur_info *source;
int id;
{
	while (source) {
		assert(id == source->occur->opt->setid);
		source = source->next;
	}
}
#endif

#endif	/* NO_CSE */
