#ident	"@(#)amigo:common/l_simplify.c	1.6"

#include "amigo.h"

#ifndef NODBG
static void loop_block_debug();
#endif

static Arena l_simplify_arena;

	/*
	** The following macro identifies troublesome blocks:
	** things like blocks consisting of a single setlocctr
	** tree.  The dominators member is not set for unreachable
	** blocks.  We may have problems moving/not moving these.
	*/
#define ORPHAN(block) (! ((block)->dominators) ) 

static void do_cgq_links();
static void remove_blocks_from_loop();
static Cgq_index move_interval();
static void reverse_cbranch();
static Boolean check_f_end_block();
static Cgq_index last_executable();
static Cgq_index insert_return();

	/*
	** Move seldom executed code out of loops.
	*/
Boolean
loop_simplify(last_loop)
Loop last_loop;
{
	Block block;
	Loop loop;
	Boolean must_insert_return, did_something;
	Cgq_index move_index;
	l_simplify_arena = arena_init();
	did_something = false;

	dominate(l_simplify_arena);

	must_insert_return = check_f_end_block(&move_index);

	if(move_index != CGQ_NULL_INDEX) {

	seldom(l_simplify_arena);
		/*
		** Top down ( for no particular reason )
		** through loops.
		** Only simplify loops at deepest nesting level.
		*/
	for (loop = last_loop; loop; loop = loop->prev) if(! loop->child) {
		Boolean has_locals = 0;
		Block remember_block, start, end;
		assert(loop->header != loop->end);
		remember_block = loop->header;

			/* start is the first block of an interval to
			** be moved.
			*/
		start = 0;

		for( block = loop->header;
			block != loop->end;
			block = block->next){
				/* The BL_HAS_LOCAL data may not be current.
				** Need to check preceeding opts for this.
				*/
			if(block->block_flags & BL_HAS_LOCAL) {
				DEBUG(aflags[ls].level&1,
				("Give up on l_simplify loop(%d), block(%d) has local\n",
				loop->id,
				block->block_count));
				has_locals = 1;
				break;
			}
		}

		if(has_locals)
			continue;
		
		for( block = loop->header->next;
			block != loop->end;
			remember_block = block, block = block->next){
			DEBUG_COND(aflags[ls].level&4, loop_block_debug(block, loop));
			if(block->seldom || ORPHAN(block) ) {
				if(start) /* extend the interval */
					end = block;	
				else if (block->seldom){ /* Start a new interval */
					start = remember_block;
					end = block;
				}
			}
			else {  /*
				** We are back in a hot part of the loop, so
				** move the interval of seldom blocks
				** out of the loop.
				*/
				if(start) {
START_OPT
					move_index = move_interval(
						&start, end, loop,
						move_index,
						&must_insert_return,
						&did_something);
				}
STOP_OPT
			}
		}
		if(start) {
START_OPT
			move_index = move_interval(&start, end, loop,
					move_index,
					&must_insert_return,
					&did_something);
		}
STOP_OPT
	}

	}
#ifndef NODBG
	else DEBUG(aflags[ls].level,
		("Gave up on loop_simplify, unprotected or bad f_end_block\n"));
#endif
	arena_term(l_simplify_arena);
	return did_something;
}

static Cgq_index
insert_return(index)
Cgq_index index;
{
	ND1 *node = t1alloc();
	node->type = TY_VOID;	
	node->op = RETURN;
	node->flags = 0;
	new_expr(node,0);
	return insert_nd1(index, node, get_f_end_block());
}

	/*
	** Find the last "executable" in the block
	*/
static Cgq_index
last_executable(block)
Block block;
{
	Cgq_index return_index;
	return_index = CGQ_NULL_INDEX;
	CGQ_FOR_ALL_IN_BLOCK_REVERSE(block,flow,index)
		if(HAS_EXECUTABLE(flow)) {
			return_index = index;
			BREAK_CGQ_FOR_ALL_IN_BLOCK_REVERSE;
		}
	END_CGQ_FOR_ALL_IN_BLOCK_REVERSE
	return return_index;
}

	/*
	** Find a nice safe place to move the moribund code.
	** If the block has no dominators, we can't fall into
	** it.  If there is no safe place set index to the place
	** at which a return node can be inserted and return true,
	** otherwise set index to the place at which moved code
	** can be inserted and return false.
	*/
static Boolean
check_f_end_block(index)
Cgq_index *index;
{
	Block block;
	block = get_f_end_block();
	if(! ORPHAN(block) ) {
		*index = last_executable(block);
		return true; /* end block reachable */
	}
	else {
		*index = CGQ_PREV_INDEX(CGQ_ELEM_OF_INDEX(block->first));
		return false;
	}
}

	/*
	** This function fixes up the block list of blocks owned by the loop
	*/
static void
remove_blocks_from_loop(entry_block, end, loop)
Block entry_block, end;
Loop loop;
{
	Block block;
	extern void remove_block(); /* In loops.c */

	block = entry_block;
	do {
		block = block->next;
		remove_block(block, loop);
	} while (block != end);
}

	/*
	** Some of this stuff should be moved into cgstuff.c
	** This code removes the interval from it's existing place
	** in the cg_q and inserts it after place.
	** Need a better name for this function -- PSP.
	*/
static void
do_cgq_links(entry_block, end, place)
Block entry_block, end;
Cgq_index place;
{
	Cgq_index temp;
		/* Remember the index of the item following the entry block */
	temp = CGQ_ELEM_OF_INDEX(entry_block->last)->cgq_next;
		/* Change that index to be the index of the item
		** following the last item in the interval.
		*/
	CGQ_ELEM_OF_INDEX(entry_block->last)->cgq_next =
		CGQ_ELEM_OF_INDEX(end->last)->cgq_next;
	CGQ_ELEM_OF_INDEX(CGQ_ELEM_OF_INDEX(end->last)->cgq_next)->cgq_prev =
		entry_block->last;
	
		/*
		** After the last item in the interval comes what
		** follows place.
		*/
	CGQ_ELEM_OF_INDEX(end->last)->cgq_next =
		CGQ_ELEM_OF_INDEX(place)->cgq_next;
	CGQ_ELEM_OF_INDEX(CGQ_ELEM_OF_INDEX(place)->cgq_next)->cgq_prev =
		end->last;

		/* temp is to be patched in at place */
	CGQ_ELEM_OF_INDEX(place)->cgq_next = temp;
	CGQ_ELEM_OF_INDEX(temp)->cgq_prev = place;
	{
	Block block;
	block = get_f_end_block_pred();
	block->next = entry_block->next;
	entry_block->next = end->next;
	end->next = get_f_end_block();
	enter_f_end_block_pred(end);
	}
}

	/*
	** We think we are looking at an interval of blocks
	** following entry_block.  entry_block should end in
	** a cbranch ( guard ) which dominates every block
	** in the interval.  If the dominator condition
	** fails there is something we don't understand, so give
	** up.
	** End should be followed by a label which is the target
	** of the branch.  ( Maybe don't need to check the second
	** condition. )
	** move_interval return a new " safe place " to move future
	** intervals.
	*/
static Cgq_index
move_interval(entry_block, end, loop, move_index, must_insert_return, did_something)
Block *entry_block, end;
Loop loop;
Cgq_index move_index;
Boolean *must_insert_return, *did_something;
{
	int label;
	Block block;
	ND1 *cbranch;
	Block_list sc = (*entry_block)->succ;
	Boolean interval_has_code;

	DEBUG(aflags[ls].level&4, ("Try move loop(%d) interval (%d,%d]\n",
		loop->id,
		(*entry_block)->block_count,end->block_count));

		/*
		** Check entry block is CBRANCH domininating the
		** candidate interval.
		*/
	cbranch = HAS_ND1(CGQ_ELEM_OF_INDEX((*entry_block)->last));

	if(! cbranch || cbranch->op != CBRANCH) {
		DEBUG(aflags[ls].level, ("Gave up on move_interval \
( can\'t find guard ) in loop(%d) interval (%d,%d]\n",
		loop->id,
		(*entry_block)->block_count,end->block_count));
		*entry_block = 0;
		return(move_index);
	}

		/*
		** Check successors of (*entry_block) look OK
		*/
	for(sc = (*entry_block)->succ; sc; sc = sc->next) {
		if(sc->block->block_count != (*entry_block)->next->block_count
			&& sc->block->block_count != end->next->block_count) {

			DEBUG(aflags[ls].level, ("Gave up on move_interval \
in loop(%d) ( entry block does not jump around interval (%d,%d]\n",
			loop->id,
			(*entry_block)->block_count,end->block_count));

			*entry_block = 0;
			return(move_index);
		}
	}

		/*
		** Make sure entry block dominates all the other
		** blocks in the interval.  Otherwise there may
		** be something going on we don't understand.
		** Also check block_flags to make sure there is
		** some non trivial code in the interval.  We
		** don't want to move an interval containing only
		** a jump, for example.
		*/
	block = (*entry_block);
	interval_has_code = false;
	do {
		block = block->next;

		if(block->block_flags & BL_HAS_EXPR)
			interval_has_code = true;
		if(block->dominators && 
			! bv_belongs((*entry_block)->block_count, block->dominators)) {
			DEBUG(aflags[ls].level, ("Bailout on (%d,%d]\n",
				(*entry_block)->block_count,end->block_count));
			DEBUG(aflags[ls].level,
				("Entry_block does not dominate block %d\n",
				block->block_count));
			*entry_block = 0;
			return(move_index);
		
		}
		DEBUG_COND(aflags[ls].level&8, loop_block_debug(block, loop));
	} while (block != end);
	if(! interval_has_code ) {
		DEBUG(aflags[ls].level, ("Gave up on move_interval \
( interval is trivial ) in loop(%d) interval (%d,%d]\n",
			loop->id,
			(*entry_block)->block_count,end->block_count));
		*entry_block = 0;
		return(move_index);
	}

		/*
		** We're committed to move some blocks.  If necessary,
		** add a return at end of function to make the
		** code we insert reachable only by the jumps.
		*/
	*did_something = true;
	if(*must_insert_return) {
		DEBUG(aflags[ls].level&1, ("Insert return at index(%d)\n",
			CGQ_INDEX_NUM(move_index)));
		move_index = insert_return(move_index);
		*must_insert_return = false;
	}

	label = new_label();
	move_index = insert_label(move_index, label);
		/*
		** This label will be the top of the first block of
		** the moved interval, so we must update the block data
		** for it.  Maybe we should have a primitive amigo_insert
		** similar to amigo_delete.
		*/
	(*entry_block)->next->first = move_index;
	reverse_cbranch(cbranch, label, (*entry_block));
	DEBUG(aflags[ls].level&1, ("Moved interval (%d,%d] out of loop(%d)\n",
		(*entry_block)->block_count, end->block_count,
		loop->id
	));

	remove_blocks_from_loop((*entry_block), end, loop);
	do_cgq_links((*entry_block), end, move_index);
	*entry_block = 0;
	return(end->last);
}

static void
reverse_cbranch(cbranch, label, block)
ND1 *cbranch; int label; Block block;
{
	ND1 *node;

	cbranch->c.label = label;
	switch(cbranch->left->op) {
	case NE:
		cbranch->left->op = EQ;
		break;
	case EQ:
		cbranch->left->op = NE;
		break;
	case LT:
		cbranch->left->op = NLT;
		break;
	case GT:
		cbranch->left->op = NGT;
		break;
	case GE:
		cbranch->left->op = NGE;
		break;
	case LE:
		cbranch->left->op = NLE;
		break;
	case LG:
		cbranch->left->op = NLG;
		break;
	case LGE:
		cbranch->left->op = NLGE;
		break;
	case NLT:
		cbranch->left->op = LT;
		break;
	case NGT:
		cbranch->left->op = GT;
		break;
	case NGE:
		cbranch->left->op = GE;
		break;
	case NLE:
		cbranch->left->op = LE;
		break;
	case NLG:
		cbranch->left->op = LG;
		break;
	case NLGE:
		cbranch->left->op = LGE;
		break;
	case NOT:
		node = cbranch->left;
		cbranch->left = node->left;
		nfree(node);
		break;
	default: 
		node = t1alloc();
		*node = *(cbranch->left);
		node->op = NOT;
		node->left = cbranch->left;
		node->right = 0;
		cbranch->left = node;
		break;
	}
	new_expr(cbranch, block);
}

#ifndef NODBG
static void
loop_block_debug(block, loop)
Block block;
Loop loop;
{
	if(block == loop->header)
		fprintf(stderr,"loop(%d) header block %d\n", loop->id, block->block_count);
	if(block == loop->end)
		fprintf(stderr,"loop end %d\n");
	fprintf(stderr,"block %d  seldom: %d ",
		block->block_count, block->seldom);
	pr_block_list(block->pred,"\tpreds");
	pr_block_list(block->succ," succs");
	fprintf(stderr,"\n");
	if(block->dominators) {
		fprintf(stderr,"\tdominators: ");
		bv_print(block->dominators);
	}
}
#endif
