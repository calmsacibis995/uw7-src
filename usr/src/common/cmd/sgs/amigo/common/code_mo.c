#ident	"@(#)amigo:common/code_mo.c	1.31"
#include "amigo.h"
#include "costing.h"
#include "lang.h"
static void PROTO(rewrite_loop_headers,(Loop, Loop));
static void PROTO(tree_walk,(ND1 *, Loop));
static ND1* PROTO(edit,(struct Expr_info *, Loop));
static Boolean changed;
#ifndef NODBG
static int dlineno;
#endif

		/*
		** If we find an invariant that may not be anticipated
		** in the loop header, but nevertheless is available
		** in the the unique predecessor of the header, or in
		** the header itself if there is more than one predecessor it
		** gets special treatment.  We could make the treatment more
		** elegant if we computed the global attribute AVAIL for
		** expressions.
		*/
void
loop_invariant_code_motion(first_loop)
Loop first_loop;
{
	Loop loop, loop_cluster;	
	Arena code_motion_arena;

	code_motion_arena = arena_init();

	loop_expr_kills(first_loop);
	locally_anticipate(code_motion_arena);
	anticipate(code_motion_arena); /* solve global data_flow for ANTIC */

	/* For each basic block in a loop, do a top-down traversal
	   of the trees in the block.  Replace any subtree not
	   killed in the loop by its temp, and do not visit the
	   children.  Add the replaced expr to the loop invariants.
	*/
	
	loop_cluster = first_loop;

	for(loop = first_loop; loop; loop = loop->next) {
		Block_list bl;
		loop->header_pred_count = 0;
		for(bl = loop->header->pred; bl; bl = bl->next)
			loop->header_pred_count++;
		FOR_ALL_TREES_IN_LOOP(loop,node,bl,flow,index)
			changed=false;

			DEBUG(aflags[cm].level&8,
				("Checking top node in loop %d:\n",loop->id));
			DEBUG_COND(aflags[cm].level&8,tr_e1print(node,"XX"));
#ifndef NODBG
			dlineno = flow->cgq_ln;
#endif
			if(node->op == CBRANCH)
				node = node->left;


			tree_walk(node,loop);
			if(changed) {
					/*
					** Set temporary flag to
					** force new_expr call AFTER
					** headers are generated.
					** Headers may cause temps to
					** escape.
					*/
				node->flags |= FF_AMIGO;
				DEBUG(aflags[cm].level&4,("Mark for hash new expr:\n"));
				DEBUG_COND(aflags[cm].level&4,tr_e1print(node,"XXX"));
			}
		END_FOR_ALL_TREES_IN_LOOP
		DEBUG(aflags[cm].level&2,("loop %d invariants:",loop->id));
		DEBUG_COND(aflags[cm].level&2,bv_print(loop->invariants));
		if(! loop->parent ) { /* top level loop */
			rewrite_loop_headers(loop_cluster, loop->next);
			loop_cluster = loop->next;
		}
	}
	for(loop = first_loop; loop; loop = loop->next) {
		DEBUG(aflags[cm].level&4, ("Checking loop %d for unhashed nodes:\n",loop->id));
		FOR_ALL_TREES_IN_LOOP(loop,node,bl,flow,index)

			if(node->op == CBRANCH)
				node = node->left;

			if(node->flags & FF_AMIGO) {
				node->flags &= ~FF_AMIGO;
				new_expr(node, bl->block);
				DEBUG(aflags[cm].level&4,("Hashing new expr at index %d: \n", CGQ_INDEX_NUM(index)));
				DEBUG_COND(aflags[cm].level&4,print_expr(node->opt));
			}
		END_FOR_ALL_TREES_IN_LOOP
	}
	arena_term(code_motion_arena);
}

static Boolean
cost_effective(node)
ND1 * node;
{
	DEBUG_UNCOND(nodes_costed_count++;)
	if(( node->opt->flags & EXPENSIVE ) == 0) return false;
	return COST(node) > cm_threshold;
}

#define INVARIANT(node,loop) (EXCEPT_OPT(node) \
	&& !(language == Cplusplus_language && cg_tree_contains_op(node, EH_LABEL)) \
	&& !bv_belongs(node->opt->setid, loop->expr_kills) \
	&& (\
		bv_belongs(node->opt->setid, loop->header->next->antic) \
		||\
		loop->header_pred_count == 1 &&\
			bv_belongs(node->opt->setid, loop->header->pred->block->loc_antic) &&\
			! bv_belongs(node->opt->setid, loop->header->pred->block->exprs_killed)\
		||\
		loop->header_pred_count > 1 &&\
			bv_belongs(node->opt->setid, loop->header->loc_antic) &&\
			! bv_belongs(node->opt->setid, loop->header->exprs_killed)\
	)\
	&& cost_effective(node) )

	/* precondition: node is invariant in loop and there is an
	   enclosing loop
	   The code generator's costing function should only get
	   called when we find a maximal subexpression which is
	   invariant in the enclosing loop */
static void
cost_node(node,loop)
ND1 *node;
Loop loop;
{
	Loop parent = loop->parent;
	if( !INVARIANT(node, parent) ) {
		if(node->left) 
			cost_node(node->left, loop);
		if(node->right)
			cost_node(node->right, loop);
	}
}

static void
tree_walk(node,loop)
ND1 *node;
Loop loop;
{
	if(INVARIANT(node,loop)) {
START_OPT
		bv_set_bit(node->opt->setid, loop->invariants);
		DEBUG_COND(aflags[cm].level&1,dprint1(node));
		if(loop->parent) { /* cost subexpressions */
			if(node->left) cost_node(node->left, loop);
			if(node->right) cost_node(node->right, loop);
		}
		replace_expr_by_temp(node->opt, loop->scope, node);
		changed=true;
      		DEBUG(aflags[cm].level&1,
      			("Replacing invariant(%d) above, cost %d, by temp(%d) in loop %d at line=%d\n",
      				node->opt->setid, COST(node), node->sid, loop->id, dlineno));        
STOP_OPT
	}
	else {
		if(node->left) tree_walk(node->left,loop);
		if(node->right) tree_walk(node->right,loop);
	}
}

		/*

		Rewrite all the loop headers in this scope ( cluster ),
		i.e., all loops in inner to outer order from first_loop
		to, but not including, last_loop.

		*/
static void
rewrite_loop_headers(first_loop, last_loop)
Loop first_loop, last_loop; /* An innermost loop in the cluster */
{
	Loop loop;
	loop = first_loop;
	for(loop = first_loop; loop != last_loop; loop = loop->next) {
		Loop parent = loop->parent;
		BV_FOR(loop->invariants,inv)
			if(parent &&
				!bv_belongs(inv, parent->expr_kills)
				&& bv_belongs(inv,parent->header->antic)) {
				bv_set_bit(inv,parent->invariants);
				DEBUG(aflags[cm].level&2,
					("Moving invariant expr %d to \
invariants for loop %d\n", inv, parent->id));
			}
			else {
				struct Expr_info *
					this_expr = expr_info_array[inv];
				ND1 *assign_node = t1alloc();
				assign_node->op = ASSIGN;
				DEBUG(aflags[cm].level&1,("Evaluating invariant %d \
in header for loop %d\n", inv, loop->id));
				DEBUG_COND(aflags[cm].level&2,
					print_expr(expr_info_array[inv]));
				assign_node->right =
					edit(this_expr,loop);
				assign_node->left =
					make_temp(this_expr, loop->scope);
				assign_node->right->type = assign_node->type =
					assign_node->left->type;
				assign_node->flags = FF_SEFF;
				new_expr(assign_node, loop->header);
				if(assign_node->right->opt->addcon) {
					/* An address may have escaped */
					bv_or_eq((get_generic_deref())->kills,
					assign_node->right->opt->addcon->kills);
					bv_or_eq((get_generic_call())->kills,
					assign_node->right->opt->addcon->kills);
				}
				DEBUG(aflags[cm].level&1,("New node: \n"));
				DEBUG_COND(aflags[cm].level&1,
					tr_e1print(assign_node,"T"));
				insert_node(assign_node,loop->header);
			}
		END_BV_FOR	
	} 
}

		/* Build a new ND1 node from the expression.  In the
		   process, check for subexpressions which are
		   loop-invariants.  Replace these by temps and if they
		   are loop-invariant in the enclosing loop enter them on
		   the invariants list of the enclosing loop. 

		   The recursive step is:

		   if parent is not null, and if expr is invariant
		   in parent enter expr in the invariants of parent
		   and return a temp for expr.  Otherwise return a
		   node constructed from the expr and its edited children.
		*/
static ND1 *
edit(expr, loop)
struct Expr_info *expr;
Loop loop;
{
	/* precondition: expr is invariant in loop. */

	Loop parent = loop->parent;
	if(parent && !bv_belongs(expr->setid,parent->expr_kills) &&
		bv_belongs(expr->setid,parent->header->next->antic) &&
		expr->cost > CM_THRESHOLD ) {
		DEBUG(aflags[cm].level&1,
			("Replacing subexpression %d by temp \
in loop %d\n", expr->setid, loop->id));
		DEBUG(aflags[cm].level&2,("\tAdding to invariants of loop %d\n",
			parent->id));
		DEBUG_COND(aflags[cm].level&2,
			print_expr(expr_info_array[expr->setid]));
	 	bv_set_bit(expr->setid,parent->invariants);
		return make_temp(expr,loop->scope);
	}
	else { /* generate assign to temp in this loop's header */
		ND1 *new_nd1 = t1alloc();
		*new_nd1 = expr->node;

		if(CHILD(expr,left)) {
			if(EXCEPT_OPT_CHILD(expr,left))
				if(bv_belongs((CHILD(expr,left))->setid,
					loop->invariants))
					new_nd1->left = make_temp(CHILD(expr,left),loop->scope);	
				else
					new_nd1->left =
						edit(CHILD(expr,left),loop);
			else new_nd1->left =
				expr_to_nd1(CHILD(expr,left));
		}
		if(CHILD(expr,right)) {
			if(EXCEPT_OPT_CHILD(expr,right))
				if(bv_belongs((CHILD(expr,right))->setid,
					loop->invariants))
					new_nd1->right = make_temp(CHILD(expr,right),loop->scope);	
				else
					new_nd1->right =
						edit(CHILD(expr,right),loop);
			else new_nd1->right =
				expr_to_nd1(CHILD(expr,right));
		}
		return new_nd1;
	}	
}
