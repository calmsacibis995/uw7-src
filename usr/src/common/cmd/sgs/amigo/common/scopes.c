#ident	"@(#)amigo:common/scopes.c	1.30"

/* Gather scope info */

#include "amigo.h"
#include "scopes.h"
#include "costing.h"

typedef struct stack_item *Stack;

static  Scope scope_root;
static scope_num;
static Object_set live_at_restore;


static Arena stack_arena, scope_arena;

	/* Static fns listed alphabetically */
static void coalesce_scopes();
static void fix_blocks();
static Boolean is_single_entry_exit();
static Scope scope_alloc();
static Scope scope_nesting();
static Scope search();
static Stack stack_alloc();

#ifndef NODBG
extern func_curr;
static void debug_blocks();
static void scope_of_cg_q_debug();
static void stack_debug();
#endif

	/*
	** OK_SYM is used both by copy_prop ( indirectly through call
	** to cg_q_in_scope()
	** and register_allocation.
	*/

#define OK_SYM(sid) (sid > 0 && (SY_FLAGS(sid) & SY_AMIGO_OBJECT) &&\
	!(SY_FLAGS(sid) & (SY_TAG|SY_LABEL|SY_MOSU|SY_ISFIELD)) &&\
	TY_ISSCALAR(SY_TYPE(sid)))
	
	/* More restictive definition for reg_alloc. */
#define OK_SYM_FOR_REG(sid) ( sid > 0 && OK_SYM(sid) && (SID_TO_OBJ(sid)->flags) & \
	CAN_BE_IN_REGISTER )

struct scope_usage {
	Scope scope;
	int usage_count;
	Scope_usage next, parent;
};

Boolean 
build_scopes(arena, top_down_basic)
Arena arena;
Block top_down_basic;
{
	Object obj;
	fix_blocks(top_down_basic);
		/* Set the file static -- maybe just make it a global PSP */
	scope_arena = arena;
	scope_root = scope_nesting(top_down_basic);
	if(scope_root == NULL) { /* Can't figure out nesting */
		DEBUG(aflags[sc].level & 1, ("Scope building unsuccessful\n"));
		return false;
	}
	DEBUG(aflags[sc].level & 2, ("===> SCOPES BEFORE COALESCE <===\n"));
	DEBUG_COND(aflags[sc].level & 2, scope_debug(scope_root));
	scope_num = 0;
	coalesce_scopes(scope_root);
	/* The following will assign scopes to globals/file statics */
	for (obj=get_object_last(); obj; obj=obj->next)
		if (!obj->scope)
			obj->scope = scope_root;

	DEBUG(aflags[sc].level & 1, ("===> SCOPES AFTER COALESCE <===\n"));
	DEBUG_COND(aflags[sc].level & 1, scope_debug(scope_root));
	DEBUG_COND(aflags[sc].level & 8, scope_of_cg_q_debug(top_down_basic));
	return true;
}

Scope
parent_scope(scope)
Scope scope;
{
	return scope->parent;
}

	/*
	** Insert a new scope_usage using link. Return value points at
	** new item.
	*/
static Scope_usage
insert_scope_usage(scope, link)
Scope scope;
Scope_usage *link;
{
	Scope_usage rtrn = Arena_alloc(scope_arena,1,struct scope_usage);
	rtrn->usage_count = 0;
	rtrn->scope = scope;
	rtrn->next = *link;
	*link = rtrn;
	
	if(scope->parent) { /* Get ancestors on list */
		Scope_usage s_u, remember = rtrn;	
		for(s_u = rtrn->next;
			s_u && s_u->scope->id > scope->parent->id; s_u = s_u->next) 
				remember = s_u;
		if(!s_u || s_u->scope->id != scope->parent->id)
			rtrn->parent =
				insert_scope_usage(scope->parent, &(remember->next));
		else
			rtrn->parent = s_u;
	}
	else
		rtrn->parent = 0;
	return rtrn;
}

	/*
	** Find the index after which restore code should be
	** inserted.  If the last executable in the scope is not
	** a JUMP, this is just the last index in the scope,
	** otherwise it is the index preceding the jump.
	*/
Cgq_index
spill_restore_index(scope)
Scope scope;
{
	if(scope->last_op != JUMP)
		return scope->last_index;
	else {
		Cgq_index index;
		cgq_t *elem;
	        for(index = scope->last_index; ;
			index = CGQ_PREV_INDEX(elem)) {

			elem = CGQ_ELEM_OF_INDEX(index);
			if(elem->cgq_op == CGQ_EXPR_ND2 &&
				elem->cgq_arg.cgq_nd2->op == JUMP) {
				assert(CGQ_PREV_INDEX(elem) != CGQ_NULL_INDEX);
				return CGQ_PREV_INDEX(elem);
			}
		}
	}
}
void PROTO(add_refs,(Object_set live, ND1 *node));

static Block
unique_predecessor(block)
Block block;
{
	Block_list bl = block->pred;
	int count;
	count = 0;
	while(bl) {
		count++;
		if(count>1)
			return 0;
		block = bl->block;
		bl = bl->next;
	}

	if(count > 0)
		return block;
	else
		return 0;
}

	/*
	** Check that at index ( in last block of scope ) object is live
	*/
Boolean
is_live_at_restore_index(object, restore_index, scope)
Object object;
Cgq_index restore_index;
Scope scope;
{
	Block block;

	if(!scope->block_end->live_at_bottom)
		return(false);
	else
		bv_assign(live_at_restore, scope->block_end->live_at_bottom);

	block = scope->block_end;

		/* This code mimics similar code in dead_store code */
	while(block) {	

	CGQ_FOR_ALL_IN_BLOCK_REVERSE(block, flow, index)
		ND1 *node;

		if(index == restore_index)
			return bv_belongs(object->setid, live_at_restore);
		
		node = HAS_ND1(flow);
		if(node) {

			if(node->op == ASSIGN && OPTABLE(node->left) &&
				node->left->flags&OBJECT &&
				node->left->opt->object->flags&CAN_BE_IN_REGISTER) {
				bv_clear_bit(node->left->opt->object->setid,
					live_at_restore);	
				node = node->right;
			}

			add_refs(live_at_restore, node);
		}
	END_CGQ_FOR_ALL_IN_BLOCK_REVERSE

	block = unique_predecessor(block);
	}

	Amigo_fatal("is_live_at_restore_index()");
}

	/*
	** For now find the predecessor of the first executable
	*/
Cgq_index
get_first_scope_index(scope, found_block)
Scope scope;
Block *found_block;
{
	Block block;
	Cgq_index return_index;

	block = scope->block_begin;

	for(;;) {
		CGQ_FOR_ALL_BETWEEN(flow, index, block->first, block->last)
			if(HAS_EXECUTABLE(flow) &&
				flow->cgq_scope_marker > scope->scope_begin ) {
				if(block == scope->block_begin &&
					flow->cgq_op == CGQ_EXPR_ND2 &&
					flow->cgq_arg.cgq_nd2->op == LABELOP) {	
						/*
						** We will do save AFTER
						** the label in case entry
						** to the lead block is via
						** a jump or branch.
						*/
					return_index = index;
					goto rtrn;
				}
				else {
					return_index = flow->cgq_prev;
					goto rtrn;
				}
			}
		CGQ_END_FOR_ALL_BETWEEN
		if(block == scope->block_end)
			break;
		block = block->next;
	}	

	return_index = CGQ_NULL_INDEX;
rtrn:
	if(found_block)
		*found_block = block;
	return return_index;
}

int
spill_cost(scope)
Scope scope;
{
	Block block;
	
	(void)get_first_scope_index(scope, &block);
	return(2 * get_block_weight(block));
}

void
bump_scope_usage(obj, scope, weight)
Object obj;
Scope scope;
int weight;
{
	Scope_usage s_u, *link;
		/*
		** Scopes are on the list in decreasing order
		** by scope id.  Therefore, the parent always occurs
		** later in the list than the child.
		*/

	DEBUG(aflags[sc].level & 128,
		("bumping use for object %s(%d) in scope(%d)\n",
		OBJ_TO_NAME(obj), obj->setid, scope->id)); 

	DEBUG_COND(aflags[sc].level & 16, scope_usage_debug("USE BEFORE",obj));
	link = &(obj->scope_usage);

	for(s_u = obj->scope_usage;
		s_u && s_u->scope->id > scope->id; s_u = s_u->next) {
		link = &(s_u->next);	
	}

	if(! s_u || s_u->scope->id != scope->id ) 
		s_u = insert_scope_usage(scope, link);
		
	DEBUG_COND(aflags[sc].level & 16, scope_usage_debug("USE AFTER insert",obj));
	do {
		s_u->usage_count += weight;
		if(s_u->usage_count > MAX_WEIGHT)
			s_u->usage_count = MAX_WEIGHT;
		s_u = s_u->parent;
	} while(s_u);
	DEBUG_COND(aflags[sc].level & 16, scope_usage_debug("USE AFTER",obj));
}

int
usage_in_scope(obj, scope)
Object obj;
Scope scope;
{
	Scope_usage s_u;

		/*
		** Check this for efficiency -- i.e., make sure
		** we are not looking at stupid stuff -- PSP
		*/
	for(s_u = obj->scope_usage; s_u; s_u = s_u->next) {
		if(s_u->scope == scope) {
			if (s_u->usage_count > 0 
			 && s_u->usage_count < BLOCK_WEIGHT_INIT)
				return 1;
			return s_u->usage_count / BLOCK_WEIGHT_INIT;
		}
	}
	return 0;
}

Boolean
is_spilled(object,scope)
Object object;
Scope scope;
{
	Object_list ol;
	for(ol = scope->o_spilled; ol; ol = ol->next) {
		if(object == ol->object)
			return true;
	}
	return false;
}

Scope
scope_of_cg_q(flow)
cgq_t *flow;
{
	int tree_num = flow->cgq_scope_marker;
	if(tree_num) return search(scope_root, tree_num);
	else return scope_root;
}

int
cg_q_in_scope(index, object)
Cgq_index index;
Object object;
{
		/* Need better checks: PSP */
	if(! OK_SYM (object->fe_numb) )
		return false;
	if(object->flags&EXTERN)
		return true;
#ifndef NODBG
	if(!object->scope) {
		DPRINTF("Bad object scope for object %s(%d)\n",
			OBJ_TO_NAME(object),object->fe_numb);
		assert(object->scope);
	}
#endif
	return CGQ_SCOPE_BETWEEN( CGQ_ELEM_OF_INDEX(index)->cgq_scope_marker,
		object->scope->scope_begin, object->scope->scope_end);
}

	/*
	** Following function creates stub scope entries for the cg_q
	** sufficient to allow copy_prop to determine scope.
	** Remove this call if we can create full blown scope
	** info once ( before copy_prop ) and have it current for
	** register allocation. PSP
	*/

extern void cgq_print_tree();

void
order_cg_q(arena, top_down_basic)
Arena arena;
Block top_down_basic;
{
	int tree_count; /* Use to order ND1s lexically */
	Boolean last_block;

	tree_count = 0;
	scope_arena = arena;

	last_block = (top_down_basic->next == 0);

	CGQ_FOR_ALL(flow, index)
		
	if(last_block) {
		if (CGQ_NEXT_INDEX(flow) == CGQ_NULL_INDEX)
			top_down_basic->last = index;
	}	

	else if(index == top_down_basic->last) {

		top_down_basic = top_down_basic->next;

		last_block = (top_down_basic->next == 0);

		if(top_down_basic->first != CGQ_NEXT_INDEX(flow)) {
			top_down_basic->first = CGQ_NEXT_INDEX(flow);
		}
	}
	
	if(flow->cgq_op == CGQ_CALL_SID && OK_SYM(flow->cgq_arg.cgq_sid) 
		|| flow->cgq_op == CGQ_START_SCOPE
			&& ! (flow->cgq_flags & CGQ_DELETE)
		|| flow->cgq_op == CGQ_END_SCOPE
			&& ! (flow->cgq_flags & CGQ_DELETE)  ) {

	DEBUG_COND(aflags[sc].level & 64, cgq_print_tree(flow,index,2));
		if( ( flow->cgq_func == db_symbol
			|| flow->cgq_op == CGQ_START_SCOPE) 
			&& ! (flow->cgq_flags & CGQ_DELETE)) {
			/* start of scope for symbol */
			Object obj = SID_TO_OBJ(flow->cgq_arg.cgq_sid);
			if(obj->scope == 0)
				obj->scope = scope_alloc();
			obj->scope->scope_begin = tree_count;
		}
		else if( ( flow->cgq_func == db_sy_clear 
			|| flow->cgq_op == CGQ_END_SCOPE )
			&& ! (flow->cgq_flags & CGQ_DELETE)) {

			Object obj = SID_TO_OBJ(flow->cgq_arg.cgq_sid);
			assert(obj);
			if(obj->scope == 0) {
				/*
				** This can happen if all ND1s have been
				** eliminated.  So just fake it.
				*/
				obj->scope = scope_alloc();
				obj->scope->scope_begin = tree_count;
			}
			obj->scope->scope_end = tree_count;
		}
	}
	else if (HAS_EXECUTABLE(flow) != 0)
		tree_count = tree_count + 1;

	flow->cgq_scope_marker = tree_count;
	CGQ_END_FOR_ALL
}

	/*
	** contain(scope1, scope2) iff scope1 is a "subset" of scope2
	*/
Boolean
contain(scope1, scope2)
Scope scope1, scope2;
{

/* Alternatively:
	Scope scope;
	for(scope = scope1; scope; scope = scope->parent) {
		if(scope == scope2) return 1;
	}
	return 0;
*/
	return
	
		scope1->scope_begin >= scope2->scope_begin
		&&
		scope1->scope_end <= scope2->scope_end
	;
}

struct stack_item {
		/*
		** A stack_item is unmarked if the start of the scope
		** has been seen, but not the end.  A marked item
		** has had its end seen when it was not on top, i.e.,
		** its start is earlier than the start of the top symbol
		** and its end ( which has just been seen ) occurs later
		** than the start of the top symbol ( whose end has not
		** been seen.  Thus the marked symbol overlaps any unmarked
		** symbol nearer the top of the stack.
		*/
	SX sid;
	int marked;
	int first_tree;
	Block first_block;
	Stack next;	
	Scope scope;
};

static void
coalesce_scopes(scope)
Scope scope;
{
	/* scope is the root of the scope structure */
	Scope child, last_child;
	Object_list ol;

	scope->id = ++scope_num;

	for(last_child = 0, child = scope->child; child;)
			/*
			** If this scope's child is the same,
			** or the child is not single entry/exit,
			** kill the child, take the child's objects.  
			*/
		if(child->scope_begin == scope->scope_begin &&
			child->scope_end == scope->scope_end
		 	|| !is_single_entry_exit(child)) {

			Scope sc;

			while(child->o_list) {
				ol = scope->o_list;
				scope->o_list = child->o_list;
				child->o_list = child->o_list->next;
				scope->o_list->next = ol;
			}
				/* Give the child's children to
				** the parent
				*/
			if(last_child) {
				last_child->sibling = child->sibling;
				for(sc = last_child; sc->sibling; sc = sc->sibling);	
				sc->sibling = child->child;
				child = last_child->sibling;
			}
			else { /* We are killing first child */
				if(child->sibling) {
					scope->child = child->sibling;
					for(sc = scope->child; sc->sibling;
						sc = sc->sibling);
					sc->sibling = child->child;
				}
				else { /* we are killing only child */
					scope->child = child->child;
				}
				child = scope->child; /* next child */
			}
			
		}
		else {
			last_child = child;
			child = child->sibling;
		}

      	for(ol = scope->o_list; ol; ol = ol->next) {
      		/* Enter scope in object data base. */
      		ol->object->scope = scope;
		DEBUG(aflags[sc].level & 8,
			("\tEntering object(%d) in scope(%d)\n",
			ol->object->setid,scope->id));
      	}

	for(child = scope->child; child; child = child->sibling) {
		assert(is_single_entry_exit(child));
		coalesce_scopes(child);
		child->parent = scope;
	}

}

	/*
	** Make sure block->last is correct for every block.
	** This pass could be merged into scope_nesting without
	** too much pain.
	*/ 
static void
fix_blocks(top_down_basic)
Block top_down_basic;
{
	Block block_bound, last_block;
	Cgq_index last_index;
	block_bound = top_down_basic;
	last_block = 0;
	last_index = CGQ_NULL_INDEX;

	DEBUG_COND(aflags[sc].level & 32,
		debug_blocks("BEFORE fix =========== ", top_down_basic));

	CGQ_FOR_ALL(flow,index)
		if(block_bound && index == block_bound->first) {
			DEBUG(aflags[sc].level & 32, 
				("Fixing block(%d [%d-%d])\n",
				block_bound->block_count,
				CGQ_INDEX_NUM(block_bound->first),
				CGQ_INDEX_NUM(block_bound->last)));
			if(last_block && last_index != last_block->last) {
				last_block->last = last_index;
			}
			last_block = block_bound;
			block_bound = block_bound->next;
				/* Get next non empty block */
			while(block_bound && EMPTY_BLOCK(block_bound))
				block_bound = block_bound->next;
		}
		last_index = index;
	CGQ_END_FOR_ALL

	DEBUG_COND(aflags[sc].level & 32,
		debug_blocks("AFTER fix =========== ", top_down_basic));

}

enum Edge_constraint { INSIDE, OUTSIDE, ONE_OUTSIDE };

	/* Check the edges in the list satisfy the constraint */
static Boolean
check_edges(bl, block_set, constraint)
Block_list bl;
Block_set block_set;
enum Edge_constraint constraint;
{
	int inside_count, outside_count;	

	inside_count = outside_count = 0;

	while(bl) {
		if(bv_belongs(bl->block->block_count, block_set))
			inside_count++;
		else
			outside_count++;
		bl = bl->next;
	}

	switch(constraint) {
	case INSIDE:
		return ( outside_count == 0 );
	case OUTSIDE:
		return ( inside_count == 0 );
	case ONE_OUTSIDE:
		return ( outside_count == 1);
	default:
		Amigo_fatal("Illegal argument to check_edges()");
	}
}

	/*
	** If entries/exits are other that the
	** lexically first/last blocks, this routine may erroneously
	** say a scope is not single entry/single exit.
	*/
static Boolean
is_single_entry_exit(scope)
Scope scope;
{
	Block block, first_block, last_block;
	Block_set block_set;
	Arena temp_arena;
	int return_val = 1;

	first_block = scope->block_begin;
	last_block = scope->block_end;
	temp_arena = arena_init();
	block_set = Block_set_alloc(temp_arena);
	bv_init(false, block_set);

	for(block = first_block; block != last_block->next; block = block->next) {
		bv_set_bit(block->block_count, block_set);
	}

	for(block = first_block; block != last_block->next; block = block->next) {
			/*
			** If block is the lead block, all predecessors
			** must be outside the scope, otherwise all
			** predecessors must be inside the scope.
			*/
		if(! check_edges(block->pred, block_set,
			block == first_block ? OUTSIDE : INSIDE) ) {
			return_val = 0;
			break;
		}
			/*
			** If block is not the trailing block, all successors
			** must be inside the scope.
			** The trailing block must have exactly one  successor
			** outside the scope.  If the trailing block ends
			** in CBRANCH, it must be in scope, otherwise the
			** restore code will end up preceding the CBRANCH.
			*/
		if(! check_edges(block->succ, block_set,
			block == last_block ? ONE_OUTSIDE : INSIDE )) {
			return_val = 0;
			break;
		}

		if(block == last_block) {
				cgq_t *elem;
				elem = CGQ_ELEM_OF_INDEX(block->last);
				if(elem->cgq_op == CGQ_EXPR_ND1 &&
					elem->cgq_arg.cgq_nd1->op == CBRANCH ) {
					if(! CGQ_SCOPE_BETWEEN(elem->cgq_scope_marker, scope->scope_begin, scope->scope_end))
					return_val = 0;
					break;
				}
		}
	}
	arena_term(temp_arena);
	return return_val;
}

static Scope
scope_alloc()
{
	Scope scope;
#ifndef NODBG
	static scope_id; 
#endif
	scope = Arena_alloc(scope_arena,1,struct scope_info);
#ifndef NODBG
	scope->id = ++scope_id;
#endif
	scope->last_op = 0;
	scope->o_list = 0;	
	scope->o_spilled = 0;
	scope->last_index = CGQ_NULL_INDEX;
	scope->parent = scope->child = scope->sibling = 0;
	return scope;
}

#ifndef NODBG
#ifdef __STDC__
#define ASSERT(cond,stack)  if( !(cond) ) {/* Something is wrong, so GIVE UP */ \
	DEBUG(aflags[sc].level&1,\
("\tWarning: optimizer gave up on spills in function(%d)\n\
\tASSERT(%s).\n",\
	func_curr,#cond)); \
	DEBUG_COND(aflags[sc].level&2, stack_debug(stack,#cond));\
	return NULL; \
}
#else
#define ASSERT(cond,stack)  if( !(cond) ) {/* Something is wrong, so GIVE UP */ \
	DEBUG(aflags[sc].level&1,\
("\tWarning: optimizer gave up on spills in function(%d)\n", func_curr));\
	return NULL;\
}
#endif
#else
#define ASSERT(cond,stack) if( !(cond) ) return NULL
#endif

static Scope
scope_nesting(blocklist)
Block blocklist;
{
	Stack st, stack;
	Scope return_scope;
	Block block;

	int tree_count; /* Use to order ND1s lexically */
	Block remember_block; /* Use to fix up the block numbers for global scope */
	int last_op;	 /* Remember if last tree was JUMP or CBRANCH */

	stack_arena = arena_init();
	live_at_restore = Object_set_alloc(scope_arena);

	tree_count = 0;
	last_op = 0;

		 /* push an item for the global scope */
	remember_block = blocklist;
	stack = stack_alloc(0, tree_count, remember_block);
	stack->scope = scope_alloc();

	
	for(block = blocklist; block; block = block->next) {

	DEBUG_COND(aflags[sc].level&8,
		DPRINTF("BLOCK %d\n",block->block_count));
		
	CGQ_FOR_ALL_BETWEEN(flow, index, block->first, block->last )
	
	SX sid;
	enum Stack_action { push, pop, default_action } action;

	DEBUG_COND(aflags[sc].level&8,
		DPRINTF("Checking flow: %d\n", CGQ_INDEX_NUM(index)));

	if(flow->cgq_op == CGQ_CALL_SID && 
		OK_SYM_FOR_REG(flow->cgq_arg.cgq_sid)) {

		sid = flow->cgq_arg.cgq_sid;
		if(flow->cgq_func == db_symbol
			&& ! (flow->cgq_flags & CGQ_DELETE) )
			action = push;
		else if(flow->cgq_func == db_sy_clear
			&& ! (flow->cgq_flags & CGQ_DELETE) )
			action = pop;
		else
			action = default_action;
	}
	else if(flow->cgq_op == CGQ_START_SCOPE 
			&& ! (flow->cgq_flags & CGQ_DELETE) &&
		OK_SYM_FOR_REG(flow->cgq_arg.cgq_sid)) {
		action = push;
		sid = flow->cgq_arg.cgq_sid;
	}

		/*
		** For CGQ_START/STOP_SPILL the positive integer
		** in cgq_arg.cgq_int is used only for matching
		** corresponding START/STOPS and there is no
		** associated object.  To recognize this later,
		** enter a negative sid for this stack item.
		** Hack, hack, cough, cough.
		*/
	else if(flow->cgq_op == CGQ_START_SPILL
			&& ! (flow->cgq_flags & CGQ_DELETE) ) {
		action = push;
		sid = -flow->cgq_arg.cgq_int;
	}

	else if(flow->cgq_op == CGQ_END_SPILL
			&& ! (flow->cgq_flags & CGQ_DELETE) ) {
		action = pop;
		sid = -flow->cgq_arg.cgq_int;
		tree_count++;
		remember_block = block;
	}

	else if(flow->cgq_op == CGQ_END_SCOPE 
			&& ! (flow->cgq_flags & CGQ_DELETE) &&  
		OK_SYM_FOR_REG(flow->cgq_arg.cgq_sid)) {
		action = pop;
		sid = flow->cgq_arg.cgq_sid;
	}

	else
		action = default_action;

	switch(action) {
	case push:
			/* start of scope for symbol */
		st = stack_alloc(sid, tree_count, block); 
		st->next = stack;
		st->first_tree = tree_count;
		stack = st;
		DEBUG_COND(aflags[sc].level & 4, 
			stack_debug(stack,"PUSH STACK"));	
		break;
	case pop:
		for(st = stack; st && st->sid != sid; st=st->next)
			;

		ASSERT(st,stack);
		
		if(st != stack) {
			st->marked = 1; /* "non nested" scope */
		}
		else { 
			Scope new_scope;

			/* We've found the end of scope for this
			** symbol. Pop this item and all marked
			** items and coalesce into a single scope.
			** The unmarked item at the top is the
			** "parent".  Add this scope to the
			** parent's list of children.
			*/
			Stack st_next;
			Object_list ol;
			Object obj;
			DEBUG(aflags[sc].level & 8,
				("\tPopping stack for sid(%d)\n", stack->sid));
			if(stack->sid > 0) {
				obj = SID_TO_OBJ(stack->sid);
				ol = Object_list_alloc(scope_arena);
				ol->next = 0;
				ol->object = obj;
			}
			else {	/*
				** Here, negative sid means this item
				** came from CGQ_START/STOP_SPILL, so
				** there is no associated object.
				*/
				ol = 0;
			}
			st_next = stack->next;
			ASSERT(st_next,stack);
			while(st_next->marked) {
				if(st_next->sid > 0) {
					/* Push object on object list,
					** unless no object associated with
					** this scope. (CGQ_START/END_SPILL)
					*/
					Object_list temp;
					temp = Object_list_alloc(scope_arena);
					temp->next = ol;
					temp->object = SID_TO_OBJ(st_next->sid);
					ol = temp;
				}
				if(st_next->first_tree < stack->first_tree) 
					stack->first_tree = st_next->first_tree;

					/*
					** For some reason we used to ASSERT
					** the falsity of the next if clause.
					*/
				if( st_next->first_block != stack->first_block) {

					DEBUG( st_next->first_block != stack->first_block
					&& aflags[sc].level&2,
					("\tst_next->first_block: %d\n stack->first_block: %d\n",
					st_next->first_block->block_count,
					stack->first_block->block_count));

					stack->first_block = st_next->first_block;

				}

					/*
					** If this item has children,
					**  we must give them to the containing
					** item.
					*/

				if(st_next->scope && st_next->scope->child) {
					if( ! stack->scope )
						stack->scope = scope_alloc();
					if(! stack->scope->child) 
						stack->scope->child =
							st_next->scope->child;	
					else {
						Scope last_child;
						last_child = stack->scope->child;
						while(last_child->sibling)
							last_child =
								last_child->sibling;
						last_child->sibling =
							st_next->scope->child;
					}
				}
				st_next = st_next->next;
				ASSERT(st_next,stack);
			}

			/*
			** stack points to the coalesced new scope.
			** st_next points to the parent.
			*/
			if(! st_next->scope) {
				if(st_next->sid) st_next->scope =
					(st_next->sid <= 0 ||
						!SID_TO_OBJ(st_next->sid)->scope ?

					scope_alloc():
					SID_TO_OBJ(st_next->sid)->scope);
			}
				/*
				** Get a scope for this item.
				*/
			new_scope = stack->scope ?
				stack->scope : scope_alloc();
			new_scope->scope_begin = stack->first_tree;
			new_scope->block_begin = stack->first_block;
			new_scope->scope_end = tree_count;
			new_scope->block_end = block;
			new_scope->last_op = last_op;
				/*
				** Remember the index: this is a
				** reasonable place to end scopes
				** for temps associated with spills.
				*/
			new_scope->last_index = index;
assert(index != CGQ_NULL_INDEX);
			new_scope->o_list = ol;

				/* Place completed child on parent's
				** children list
				*/	
			new_scope->sibling =
					st_next->scope->child;	
			st_next->scope->child = new_scope;
			DEBUG_COND(aflags[sc].level & 8,
				debug_one_scope("NEW CHILD", new_scope));
			DEBUG_COND(aflags[sc].level & 8,
				debug_one_scope("PARENT", st_next->scope));
			stack = st_next;
			DEBUG_COND(aflags[sc].level & 4, 
				stack_debug(stack,"POP STACK"));	
		}
		break;

	case default_action:

		if (HAS_EXECUTABLE(flow) != 0 ) {
			tree_count++;
			remember_block = block;
			if(flow->cgq_op == CGQ_EXPR_ND2  &&
				flow->cgq_arg.cgq_nd2->op == JUMP)
				last_op = JUMP;
			else if(flow->cgq_op == CGQ_EXPR_ND1 &&
				flow->cgq_arg.cgq_nd1->op == CBRANCH)
				last_op = CBRANCH;
			else
				last_op = 0;
		}
		break;
	}

	flow->cgq_scope_marker = tree_count;

	CGQ_END_FOR_ALL

	}
		/* Global scope should be on top */
	ASSERT(stack && stack->sid == 0 && stack->scope,stack);

	return_scope = stack->scope;
	return_scope->scope_begin = stack->first_tree;
	return_scope->scope_end = tree_count;
	return_scope->block_begin = stack->first_block;
	return_scope->block_end = remember_block;
	arena_term(stack_arena);
	return(return_scope);
	
}
	/*
	** Precondition: tree is in root.  We want the most closely
	** nested scope containing tree.
	*/
static Scope
search(root,tree_num)
Scope root;
int tree_num;
{
	Scope scope;
if(!(tree_num > root->scope_begin && tree_num <= root->scope_end))
	assert(tree_num > root->scope_begin && tree_num <= root->scope_end);
	for(scope = root->child; scope; scope = scope->sibling) {
			/* Try for a smaller scope */
		if(tree_num <= scope->scope_end && tree_num > scope->scope_begin)
			return(search(scope,tree_num));
	}
	return root;
}

static Stack
stack_alloc(sid, tree_count, block)
SX sid;
int tree_count;
Block block;
{
	Stack st;
	st = Arena_alloc(stack_arena, 1, struct stack_item);
	st->marked = 0;
	st->sid = sid;
	st->first_tree = tree_count;	
	st->first_block = block;
	st->scope = 0;	
	st->next = 0;
	return st;
}


/* Debugging functions */
#ifndef NODBG

static void
debug_blocks(str, top_down_basic)
char *str;
Block top_down_basic;
{
	Block block;
	DPRINTF("%s\n",str);
	for(block = top_down_basic; block; block = block->next) {

		if(! EMPTY_BLOCK(block))
			DPRINTF("BLOCK %d\n",block->block_count);
		
		CGQ_FOR_ALL_BETWEEN(flow, index, block->first, block->last )
			DPRINTF("FLOW %d\n", CGQ_INDEX_NUM(index));
		CGQ_END_FOR_ALL	
	}
}

int
debug_one_scope(str,scope)
char *str;
Scope scope;
{
	Scope s;
	if(! scope ) {
		DPRINTF("SCOPE EMPTY\n");
		return;
	}
	DPRINTF("%s\n\tscope: %d*=*=*=\n\tchildren: ", str, scope->id);
	for(s = scope->child; s ; s = s->sibling)
		DPRINTF(" %d", s->id);
	DPRINTF("\n\tobjects:");
pr_object_list(scope->o_list);
	DPRINTF("\tparent");
	if(scope->parent) DPRINTF(" %d", scope->parent->id );
	DPRINTF("\n\tsiblings");
	for(s = scope->sibling; s ; s = s->sibling)
		DPRINTF(" %d", s->id);
	DPRINTF("\n");
	DPRINTF("\ttrees: (%d,%d] blocks:[%d,%d] last_index:%d\n",
		scope->scope_begin,scope->scope_end,
		scope->block_begin->block_count,scope->block_end->block_count,
		(scope->last_index == CGQ_NULL_INDEX ? 0 : CGQ_INDEX_NUM(scope->last_index)));
}

void
scope_debug(scope)
Scope scope;
{
	Scope s;
	DPRINTF("scope: %d single_entry: \
%s trees: (%d,%d] blocks:[%d,%d] last_index:%d\n\tchildren",
		scope->id,
		is_single_entry_exit(scope) ? "true":"false",
		scope->scope_begin,scope->scope_end,
		scope->block_begin->block_count,scope->block_end->block_count,
		(scope->last_index == CGQ_NULL_INDEX ? 0 : CGQ_INDEX_NUM(scope->last_index)));
	for(s = scope->child; s ; s = s->sibling)
		DPRINTF(" %d", s->id);
	DPRINTF("\n\tparent");
	if(scope->parent) DPRINTF(" %d", scope->parent->id );
	DPRINTF("\n\tsiblings");
	for(s = scope->sibling; s ; s = s->sibling)
		DPRINTF(" %d", s->id);
	DPRINTF("\n\tobjects ");
	pr_object_list(scope->o_list);	
	for(s = scope->child; s ; s = s->sibling)
		scope_debug(s);
}

static void
scope_of_cg_q_debug(block)
Block block;
{
	while(block) {
		CGQ_FOR_ALL_BETWEEN(flow,index,block->first,block->last)
			Scope scope = scope_of_cg_q(flow);
			DPRINTF("Tree %d belongs to scope(%d)\n",
				flow->cgq_scope_marker,
				(int)scope ? scope->id : -1);
		CGQ_END_FOR_ALL
		block=block->next;
	}
}

void
scope_usage_debug(str,obj)
char *str;
Object obj;
{
	Scope_usage s_u;
	DPRINTF("\n\t========object:%d name=%s scope=%d %s========\n",
		obj->setid,
		OBJ_TO_NAME(obj),
		obj->scope->id,
		str
	);

	for(s_u = obj->scope_usage; s_u; s_u = s_u->next) {
		assert(s_u->scope);
		DPRINTF("\tscope: %d usage: %d parent: %d\n",
			s_u->scope->id,
			s_u->usage_count / BLOCK_WEIGHT_INIT,
			s_u->parent ? s_u->parent->scope->id : -1
		);
	}
}

static void
stack_debug(stack,string)
Stack stack;
char *string;
{
	DPRINTF("\t****%s****\n",string);
	while(stack) {
		DPRINTF("\t sid %d first_tree %d first_block %d scope %x(%d)\n",
			stack->sid, stack->first_tree,
			stack->first_block->block_count,
			(int)(stack->scope), stack->scope ? stack->scope->id:-1);
		stack = stack->next;
	}
	DPRINTF("\n\n");
}

int
debug_scope_id(scope)
Scope scope;
{
	return scope->id;
}
#endif
