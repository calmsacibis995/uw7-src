#ident	"@(#)amigo:common/dead_store.c	1.17"

#include "amigo.h"
#include "lang.h"

#ifdef NODBG
static
#endif
Arena dead_store_arena;

void PROTO(add_refs,(Object_set live, ND1 *node));

void
dead_store()
{
	Object_set live; 
	Object_set live_globals;
	Object obj;

	dead_store_arena = arena_init();


	live = Object_set_alloc(dead_store_arena);
	live_globals = Object_set_alloc(dead_store_arena);
	bv_init(false, live_globals);

	for (obj = get_object_last(); obj; obj = obj->next) {
		if (obj->flags & VALID_CANDIDATE) {
			switch (SY_CLASS(obj->fe_numb)) {
			case SC_EXTERN:
			case SC_STATIC:
				bv_set_bit(obj->setid, live_globals);
			}
		}
	}

	DEPTH_FIRST(blk_ptr)

	Block block;
	Block_list bl_list;

	block = *blk_ptr;


	/* Set live to live_on_exit */
	if (!block->succ)
		bv_assign(live, live_globals);
	else {
		bv_init(false, live);
		for(bl_list = block->succ; bl_list; bl_list = bl_list->next) {
			bv_or_eq(live, bl_list->block->live_on_entry);
		}
	}

/* An assignment can be deleted if it does not cause an exception,
** or if we are not adhering to strict ieee floating point and we 
** may cause an arithmetic exception.
*/

#define OPTABLE_FOR_DEAD(node)	\
	(OPTABLE(node) ||	\
	    	(!ieee_fp()	\
	     && EXCEPT_OPT(node)\
	     && (node->opt->flags&RAISES_EXCEPTION) == ARITH_EXCEPTION))
	

#define DEAD(node) (\
	OPTABLE_FOR_DEAD(node)	\
	&& (node->opt->flags&OBJECT)\
	&& (node->opt->object->flags&VALID_CANDIDATE)\
	&& !bv_belongs(node->opt->object->setid,live)\
)

	FOR_ALL_ND1_IN_BLOCK_REVERSE(block,flow,node,index)

	DEBUG(aflags[ds].level&4,
		("\tflow(%d) live_on_exit before node:", CGQ_INDEX_NUM(index)));
	DEBUG_COND(aflags[ds].level&4, pr_object_set(live));

	if(node->op == ASSIGN) {

		if(DEAD(node->left) && !(node->right->opt->flags & CONTAINS_VOLATILE)) {

			if(!CONTAINS_SIDE_EFFECTS(node->right)) {
					/*
					** ASSIGNs which are under CGQ_FIX_SWBEG
					** can't be deleted, because the value
					** is used for the switch.
					*/
				if(flow->cgq_op == CGQ_FIX_SWBEG) {
					DEBUG(aflags[ds].level,
					("\tChanging assign SWBEG to simple expr in flow(%d) in block(%d)\n",
					CGQ_INDEX_NUM(index),
					block->block_count));
					flow->cgq_arg.cgq_nd2->left =
						flow->cgq_arg.cgq_nd2->left->right;
					add_refs(live, node);
				}
				else {
					ND1* eh_label_node;
					DEBUG(aflags[ds].level,
					("\tDeleting flow(%d) in block(%d)\n",
					CGQ_INDEX_NUM(index),
					block->block_count));
					if (language == Cplusplus_language
					&& (eh_label_node = cg_tree_contains_op(node, EH_LABEL))) {
						/* Cannot eliminate all the EH_LABELs within the assignment;
						   have to keep (any) one of them around, so replace the
						   assignment with one of the labels. */
						*node = *eh_label_node;
						new_expr(node,0);
						add_refs(live,node);
					} else
						/* the normal case */ 
						amigo_delete(block,index);
				}
			} 
			else { /* Side effect on right of dead store */
				DEBUG(aflags[ds].level,
					("\tChanging assign to simple expr in flow(%d) in block(%d)\n",
					CGQ_INDEX_NUM(index),
					block->block_count));
				*node = *(node->right);
				new_expr(node,0);
				add_refs(live,node);
			}
		}
		else { /* Either left is not dead or rhs is volatile */

				/* In any case lhs is being killed
				** so it should be removed from live.
				*/
			if(OPTABLE_FOR_DEAD(node->left) && node->left->opt->flags&OBJECT
				&& (node->left->opt->object->flags&VALID_CANDIDATE)) {
				bv_clear_bit(node->left->opt->object->setid, live);
				add_refs(live,node->right);
			}
			else
				add_refs(live, node);
			
		}
	}
	else /* not an assign */
		add_refs(live,node);

	DEBUG(aflags[ds].level&4,
		("\tflow(%d) live_on_exit:", CGQ_INDEX_NUM(index)));
	DEBUG_COND(aflags[ds].level&4, pr_object_set(live));

	END_FOR_ALL_ND1_IN_BLOCK

	DEBUG(aflags[ds].level&2,
		("\tblock(%d) live_on_exit:", block->block_count));
	DEBUG_COND(aflags[ds].level&2, pr_object_set(live));

	END_DEPTH_FIRST

	arena_term(dead_store_arena);
}

	/*
	** Recursively descend node, adding all object references
	** to live.
	*/
void
add_refs(live, node)
Object_set live;
ND1 *node;
{
	if(! node) return;
	add_refs(live,node->right);
	add_refs(live,node->left);
	if(OPTABLE_FOR_DEAD(node) && node->opt->flags & OBJECT) {
		int setid = node->opt->object->setid;
			/*
			** AMIGO adds temps in spills which may
			** cause overflow of bit vectors.
			*/
		if(setid <= OBJECT_SET_SIZE)
			bv_set_bit(node->opt->object->setid, live);
	}
}
