#ident	"@(#)amigo:common/data_flow.c	1.21"
#include "amigo.h"
#define ALLOCATE 1
#ifndef NODBBG
static  void PROTO(	debug_antic,(void));
static  void PROTO(	debug_avail_in,(void));
static  void PROTO(	debug_bool_avail_in,(void));
static  void PROTO(	debug_dominators,(void));
static  void PROTO(	debug_live_on_entry,(Arena));
static  void PROTO(	debug_live_at_bottom,(Arena));
static  void PROTO(	debug_seldom,(void));
#endif

	/* Solve globally for SELDOM */
void
seldom(arena) 
Arena arena;
{
	Arena temp_arena = arena_init();
	int changed = true;
	DEBUG_UNCOND(int pass = 0;)
	Block_set temp_set = Block_set_alloc(temp_arena);

		/*
		** First solve data_flow for control_reaches:
		** given two blocks b1, b2, b2  belongs to b1(control_reaches)
		** iff control can flow to b2 from b1.  I.e., control_reaches
		** is the transitive closure of successor.
		*/
	REVERSE_DEPTH_FIRST(blk_ptr)

		Block_list successor;
		Block block;

		block = *blk_ptr;
		block->control_reaches = Block_set_alloc(temp_arena);
		bv_init(false,block->control_reaches);
			/* Intially set control_reaches to successors */
		for(successor = block->succ; successor; successor = successor->next)
			bv_set_bit(successor->block->block_count,block->control_reaches);
	END_DEPTH_FIRST

	changed = true;

	while(changed) {

		DEBUG_UNCOND(pass++;)
		DEBUG(aflags[df].level&2,
			("Seldom data_flow before pass %d:\n", pass));
		DEBUG_COND(aflags[df].level&2, debug_seldom());

		changed = false;

		REVERSE_DEPTH_FIRST(blk_ptr)

			Block block = *blk_ptr;
			Block_list successor;

			successor = block->succ;
			if(successor) {
				bv_assign(temp_set, block->control_reaches);
				do {
					bv_or_eq(temp_set, successor->block->control_reaches);
					successor = successor->next;
				} while(successor);
				if(! bv_equal(temp_set, block->control_reaches)) {
					bv_assign(block->control_reaches, temp_set);
					changed = true;
				}
			}
		END_DEPTH_FIRST
	}

	REVERSE_DEPTH_FIRST(blk_ptr)
		Block block = *blk_ptr;
		if(!bv_belongs(block->block_count, block->control_reaches))
			block->seldom = true;
	END_DEPTH_FIRST

	DEBUG(aflags[df].level&1,("Final seldom data_flow (after pass %d):\n",pass));
	DEBUG_COND(aflags[df].level&1, debug_seldom());
	arena_term(temp_arena);
}

	/* Solve globally for ANTICIP */
void
anticipate(arena) 
Arena arena;
{
	int changed = true;
	DEBUG_UNCOND(int pass = 0;)
	Expr_set temp_set = Expr_set_alloc(arena);

	REVERSE_DEPTH_FIRST(blk_ptr)

		Block block;

		block = *blk_ptr;
		block->antic = Expr_set_alloc(arena);
		block_expr_kills(block,ALLOCATE);
		if(block->succ)	
			bv_init(true,block->antic);
		else
			bv_assign(block->antic, block->loc_antic);

	END_DEPTH_FIRST

	changed = true;

	while(changed) {

		DEBUG_UNCOND(pass++;)
		DEBUG(aflags[df].level&2,
			("Anticipate data_flow before pass %d:\n", pass));
		DEBUG_COND(aflags[df].level&2, debug_antic());

		changed = false;

		REVERSE_DEPTH_FIRST(blk_ptr)

			Block block = *blk_ptr;
			Block_list successor;
				/* init to PRESERVES maybe should store
				   in block, instead of exprs_killed.
				   Could also speed up the algorithm
				   by storing a change flag in each block
				   and checking for at least one changed
				   successor, rather than simply checking
				   for at least one successor -- PSP */
			successor = block->succ;
			if(successor) {
				bv_init(true,temp_set);
				bv_minus_eq(temp_set,block->exprs_killed);

				do {
					if(block->block_count !=
						successor->block->block_count)
						/* only follow out edges! */	
						bv_and_eq(temp_set,successor->block->antic);
					successor = successor->next;
				} while(successor);

				bv_or_eq(temp_set,block->loc_antic);

				if(! bv_equal(temp_set,block->antic)) {
					Expr_set temp;
					DEBUG(aflags[df].level&2,
						("Changed antic in block %d\n\tnew:",
						block->block_count));
					DEBUG_COND(aflags[df].level&2,
						bv_print(temp_set));
					DEBUG(aflags[df].level&2,("\told:"));
					DEBUG_COND(aflags[df].level&2,
						bv_print(block->antic));

					temp = block->antic;
					block->antic = temp_set;
					temp_set = temp;
					changed = true;
				}
			}

		END_DEPTH_FIRST
	}

	DEBUG(aflags[df].level&1,("Final anticipate data_flow (after pass %d):\n",pass));
	DEBUG_COND(aflags[df].level&1, debug_antic());
}

	/* Solve globally for BOOL_AVIN ( boolean values available on entry ) */
void
boolean_avail_in(arena) 
Arena arena;
{
	int changed = true;
	DEBUG_UNCOND(int pass = 0;)
	Expr_set temp1_set = Expr_set_alloc(arena);
	Expr_set temp2_set = Expr_set_alloc(arena);

	changed = true;

	while(changed) {

		DEBUG_UNCOND(pass++;)
		DEBUG(aflags[bp].level&16,
			("bool_avail_in data_flow before pass %d:\n", pass));
		DEBUG_COND(aflags[bp].level&16, debug_bool_avail_in());

		changed = false;

		DEPTH_FIRST(blk_ptr)

			Block block = *blk_ptr;
			Block_list predecessor;

			predecessor = block->pred;
			if(predecessor) {
				bv_init(true,temp2_set);

				do {
					Block pred_block;
					pred_block = predecessor->block;
					bv_assign(temp1_set,
						pred_block->bool_true);

					bv_minus_eq(temp1_set,
						pred_block->exprs_killed);
					if(pred_block->edge_list) {
						Edge_descriptor ed;
						for(ed = pred_block->edge_list; ed->block_num != block->block_count; ed = ed->next)
							assert(ed->next);
						bv_or_eq(temp1_set,ed->set_to_true);
					}
					bv_and_eq(temp2_set,temp1_set);
					
					predecessor = predecessor->next;
				} while(predecessor);


				if(! bv_equal(temp2_set,block->bool_true)) {
					Expr_set temp;
					DEBUG(aflags[bp].level&32,
						("Changed bool_true in block %d\n\tnew:",
						block->block_count));
					DEBUG_COND(aflags[bp].level&32,
						bv_print(temp2_set));
					DEBUG(aflags[bp].level&32,("\told:"));
					DEBUG_COND(aflags[bp].level&32,
						bv_print(block->bool_true));

						/*
						** Switch pointers, rather
						** than copying bitvectors
						*/
					if (block->bool_true == BVZERO) {
						Bit_vector x = block->bool_true;
						block->bool_true = Expr_set_alloc(arena);
						bv_assign(block->bool_true, x);
					}
					temp = block->bool_true;
					block->bool_true = temp2_set;
					temp2_set = temp;
					changed = true;
				}
			}

		END_DEPTH_FIRST
	}

	changed = true;
	DEBUG_UNCOND(pass = 0;)

	while(changed) {

		DEBUG_UNCOND(pass++;)
		DEBUG(aflags[bp].level&16,
			("bool_avail_in data_flow before pass %d:\n", pass));
		DEBUG_COND(aflags[bp].level&16, debug_bool_avail_in());

		changed = false;

		DEPTH_FIRST(blk_ptr)

			Block block = *blk_ptr;
			Block_list predecessor;

			predecessor = block->pred;
			if(predecessor) {
				bv_init(true,temp2_set);

				do {
					Block pred_block;
					pred_block = predecessor->block;
					bv_assign(temp1_set,
						pred_block->bool_false);

					bv_minus_eq(temp1_set,
						pred_block->exprs_killed);
					if(pred_block->edge_list) {
						Edge_descriptor ed;
						for(ed = pred_block->edge_list; ed->block_num != block->block_count; ed = ed->next)
							assert(ed->next);
						bv_or_eq(temp1_set,ed->set_to_false);
					}
					bv_and_eq(temp2_set,temp1_set);
					
					predecessor = predecessor->next;
				} while(predecessor);


				if(! bv_equal(temp2_set,block->bool_false)) {
					Expr_set temp;
					DEBUG(aflags[bp].level&32,
						("Changed bool_false in block %d\n\tnew:",
						block->block_count));
					DEBUG_COND(aflags[bp].level&32,
						bv_print(temp2_set));
					DEBUG(aflags[bp].level&32,("\told:"));
					DEBUG_COND(aflags[bp].level&32,
						bv_print(block->bool_false));

						/*
						** Switch pointers, rather
						** than copying bitvectors
						*/
					if (block->bool_false == BVZERO) {
						Bit_vector x = block->bool_false;
						block->bool_false = Expr_set_alloc(arena);
						bv_assign(block->bool_false, x);
					}
					temp = block->bool_false;
					block->bool_false = temp2_set;
					temp2_set = temp;
					changed = true;
				}
			}

		END_DEPTH_FIRST
	}

	DEBUG(aflags[bp].level&16,("Final bool_avail_in data_flow (after pass %d):\n",pass));
	DEBUG_COND(aflags[bp].level&16, debug_bool_avail_in());
}

	/* Solve globally for AVIN ( assignments available on entry ) */
void
avail_in(arena) 
Arena arena;
{
	int changed = true;
	DEBUG_UNCOND(int pass = 0;)
	Assign_set temp1_set = Assign_set_alloc(arena);
	Assign_set temp2_set = Assign_set_alloc(arena);

	DEPTH_FIRST(blk_ptr)

		Block block = *blk_ptr;
		block->avin = Assign_set_alloc(arena);
		if(block->pred)
			bv_init(true,block->avin);	
		else
			bv_init(false,block->avin);
	END_DEPTH_FIRST

	changed = true;

	while(changed) {

		DEBUG_UNCOND(pass++;)
		DEBUG(aflags[df].level&2,
			("Avail_in data_flow before pass %d:\n", pass));
		DEBUG_COND(aflags[df].level&2, debug_avail_in());

		changed = false;

		DEPTH_FIRST(blk_ptr)

			Block block = *blk_ptr;
			Block_list predecessor;

			predecessor = block->pred;
			if(predecessor) {
				bv_init(true,temp2_set);

				do {
					bv_assign(temp1_set,
						predecessor->block->avin);
					bv_minus_eq(temp1_set,
						predecessor->block->assigns_killed);
					bv_or_eq(temp1_set,
						predecessor->block->reaches_exit);
					bv_and_eq(temp2_set,temp1_set);
					
					predecessor = predecessor->next;
				} while(predecessor);

				if(! bv_equal(temp2_set,block->avin)) {
					Expr_set temp;
					DEBUG(aflags[df].level&2,
						("Changed avin in block %d\n\tnew:",
						block->block_count));
					DEBUG_COND(aflags[df].level&2,
						bv_print(temp1_set));
					DEBUG(aflags[df].level&2,("\told:"));
					DEBUG_COND(aflags[df].level&2,
						bv_print(block->avin));

						/*
						** Switch pointers, rather
						** than copying bitvectors
						*/
					if (block->avin == BVZERO) {
						Bit_vector x = block->avin;
						block->avin = Assign_set_alloc(arena);
						bv_assign(block->avin, x);
					}
					temp = block->avin;
					block->avin = temp2_set;
					temp2_set = temp;
					changed = true;
				}
			}

		END_DEPTH_FIRST
	}

	DEBUG(aflags[df].level&1,("Final avail_in data_flow (after pass %d):\n",pass));
	DEBUG_COND(aflags[df].level&1, debug_avail_in());
}

	/* Solve globally for DOMINATORS */
void
dominate(arena) 
Arena arena;
{
	int changed = true;
	DEBUG_UNCOND(int pass = 0;)
	Block_set temp_set = Block_set_alloc(arena);

	DEPTH_FIRST(blk_ptr)

		Block block = *blk_ptr;
		block->dominators = Block_set_alloc(arena);
		if(block->pred)
			bv_init(true,block->dominators);	
		else {
			bv_init(false,block->dominators);
			bv_set_bit(block->block_count,block->dominators);
		}
	END_DEPTH_FIRST

	changed = true;

	while(changed) {

		DEBUG_UNCOND(pass++;)
		DEBUG(aflags[df].level&2,
			("DOMINATORS data_flow before pass %d:\n", pass));
		DEBUG_COND(aflags[df].level&2, debug_dominators());

		changed = false;

		DEPTH_FIRST(blk_ptr)

			Block block = *blk_ptr;
			Block_list predecessor;

			predecessor = block->pred;
			
			if(predecessor) {
				Block_set temp;
				bv_init(true,temp_set);

				do {
					if(predecessor->block->dominators)
					bv_and_eq(temp_set,predecessor->block->dominators);
					
					predecessor = predecessor->next;
				} while(predecessor);

					/* every block dominates itself */
				bv_set_bit(block->block_count,temp_set);

				if(! bv_equal(temp_set,block->dominators)) {
					DEBUG(aflags[ls].level&4 || aflags[df].level&2,
						("Changed dominators in block %d\n\tnew:",
						block->block_count));
					DEBUG_COND(aflags[ls].level&4 || aflags[df].level&2,
						bv_print(temp_set));
					DEBUG(aflags[df].level&2,("\told:"));
					DEBUG_COND(aflags[df].level&2,
						bv_print(block->dominators));

						/*
						** Switch pointers, rather
						** than copying bitvectors
						*/
					temp = block->dominators;
					block->dominators = temp_set;
					temp_set = temp;
					changed = true;
				}
			}

		END_DEPTH_FIRST
	}

	DEBUG(aflags[df].level&1,
		("Final dominators data_flow (after pass %d):\n",pass));
	DEBUG_COND(aflags[df].level&1,
		debug_dominators());
}
	/*
	** Solve live on entry data flow equations for the
   	** Register Allocator.  This should be converted to always use
   	** Object_sets, in which case the first parameter should be eliminated
 	** and all bv_alloc() calls changed to Object_set_alloc()
	** -- PSP
	*/

void
live_at_bottom(bv_leng, arena, flag) 
int bv_leng;
Arena arena; /* Arena in which the live_on_entry field will persist. */
int flag;
{
	Object obj;
	Bit_vector live_after_function = bv_alloc(bv_leng, arena);
	bv_init(false, live_after_function);
	for (obj=get_object_last(); obj; obj=obj->next) {
		if (obj->fe_numb <= 0)
			continue;
		if (!(obj->flags & flag))
			continue;
		if (SY_CLASS(obj->fe_numb) == SC_EXTERN ||
		    SY_CLASS(obj->fe_numb) == SC_STATIC)
			bv_set_bit(obj->setid, live_after_function);
	}
        REVERSE_DEPTH_FIRST(block_ptr)
		int init = 0;
		Block block = *block_ptr;
		Block_list s;
		block->live_at_bottom = bv_alloc(bv_leng, arena);
		if (!block->succ) {
			bv_assign(block->live_at_bottom, live_after_function);
			continue;
		}
		for (s= block->succ; s; s= s->next) {
			Block succ = s->block;
			if (!init) {
				init = 1;
				bv_assign(block->live_at_bottom, 
				    succ->live_on_entry);
			}
			else
				bv_or_eq(block->live_at_bottom,
				    succ->live_on_entry);
		}
        END_DEPTH_FIRST
	DEBUG_COND(aflags[df].level&1, debug_live_at_bottom(arena));
}

void
live_on_entry(bv_length, arena, flag) 
int bv_length;
Arena arena; /* Arena in which the live_on_entry field will persist. */
int flag;
{
        /* see Aho & Ullman dragon book */
	int changed = true;
	DEBUG_UNCOND(int pass = 0;)
	Bit_vector temp_set = bv_alloc(bv_length, arena);
	Object obj;
	Bit_vector live_after_function = bv_alloc(bv_length, arena);
	bv_init(false, live_after_function);
	for (obj=get_object_last(); obj; obj=obj->next) {
		if (obj->fe_numb <= 0)
			continue;
		if (!(obj->flags & flag))
			continue;
		if (SY_CLASS(obj->fe_numb) == SC_EXTERN ||
		    SY_CLASS(obj->fe_numb) == SC_STATIC)
			bv_set_bit(obj->setid, live_after_function);
	}

	changed = true;

		/*
		** Allocate live_on_entry.  This code should
		** be moved out to init code if the live_on_entry
		** ends up being solved more than once in the arena.
		*/

	DEPTH_FIRST(blk_ptr)

		Block block = *blk_ptr;
		block->live_on_entry = bv_alloc(bv_length, arena);
		if (!block->succ) {
			bv_assign(block->live_on_entry, live_after_function);
			bv_minus_eq(block->live_on_entry, block->def);
			bv_or_eq(block->live_on_entry, block->use);
		}
		else
			bv_assign(block->live_on_entry, block->use);
			
		

	END_DEPTH_FIRST

	while(changed) {

		DEBUG_UNCOND(pass++;)
		DEBUG(aflags[df].level&2,
			("Live_on_entry data_flow before pass %d:\n", pass));
			DEBUG_COND(aflags[df].level&2, debug_live_on_entry(arena));

		changed = false;

		REVERSE_DEPTH_FIRST(blk_ptr)

			Block block = *blk_ptr;
			Block_list successor;
			successor = block->succ;
			if(successor) {
				bv_init(false,temp_set);

				do {
				   bv_or_eq(temp_set,successor->block->live_on_entry);
				   successor = successor->next;
				} while(successor);
				bv_minus_eq(temp_set,block->def);
				bv_or_eq(temp_set,block->use);

				if(! bv_equal(temp_set,block->live_on_entry)) {
					bv_assign(block->live_on_entry,temp_set);
					changed = true;
				}
			}

		END_DEPTH_FIRST
	}

	DEBUG(aflags[df].level&1,
		("Final live_on_entry data_flow (after pass %d):\n",pass));
	DEBUG_COND(aflags[df].level&1, debug_live_on_entry(arena));
}

void
fixup_live_on_entry() {
/* sets the sizes of live_on_entry sets to OBJECT_SET_SIZE */
	DEPTH_FIRST(blk_ptr)
		Block block;
		block = *blk_ptr;
		bv_set_size(OBJECT_SET_SIZE,
			(Bit_vector)(block->live_on_entry));
	END_DEPTH_FIRST
}

#ifndef NODBG
static void
debug_seldom()
{
	DEPTH_FIRST(block_ptr)
		DPRINTF("block %d\n",((*block_ptr)->block_count));
		DPRINTF("\tseldom: %s ",
			((*block_ptr)->seldom ? "true":"false"));
		DPRINTF("\tcontrol_reaches:");
		bv_print((*block_ptr)->control_reaches);
	END_DEPTH_FIRST
}

static void
debug_antic()
{
	DEPTH_FIRST(block_ptr)
		DEBUG(aflags[df].level,("block %d\n",((*block_ptr)->block_count)));
		DEBUG(aflags[df].level,("\tantic: "));
		DEBUG_COND(aflags[df].level,bv_print((*block_ptr)->antic));
		DEBUG(aflags[df].level,("\tlocally_antic: "));
		DEBUG_COND(aflags[df].level,bv_print((*block_ptr)->loc_antic));
		DEBUG(aflags[df].level,("\texprs_killed: "));
		DEBUG_COND(aflags[df].level,bv_print((*block_ptr)->exprs_killed));
	END_DEPTH_FIRST
}

static void
debug_dominators()
{
	DEPTH_FIRST(block_ptr)
		DPRINTF("block %d\n",((*block_ptr)->block_count));
		DPRINTF("\tdominators: ");
		bv_print((*block_ptr)->dominators);
		pr_block_list((*block_ptr)->pred,"\tpreds");
		pr_block_list((*block_ptr)->succ," succs");
		fprintf(stderr,"\n");
		fflush(stderr);
	END_DEPTH_FIRST
}


static void
debug_avail_in()
{
	DEPTH_FIRST(block_ptr)
		DEBUG(aflags[df].level,("block %d\n",((*block_ptr)->block_count)));
		DEBUG(aflags[df].level,("\tavin: "));
		DEBUG_COND(aflags[df].level,bv_print((*block_ptr)->avin));
		DEBUG(aflags[df].level,("\treaches_exit: "));
		DEBUG_COND(aflags[df].level,bv_print((*block_ptr)->reaches_exit));
		DEBUG(aflags[df].level,("\tassigns_killed: "));
		DEBUG_COND(aflags[df].level,bv_print((*block_ptr)->assigns_killed));
		pr_block_list((*block_ptr)->pred,"\tpreds");
		pr_block_list((*block_ptr)->succ," succs");
		fprintf(stderr,"\n");
		fflush(stderr);
	END_DEPTH_FIRST
}

static void
debug_bool_avail_in()
{
	DEPTH_FIRST(block_ptr)
		DEBUG(aflags[bp].level,("block %d\n",((*block_ptr)->block_count)));
		DEBUG(aflags[bp].level,("\tbool_true: "));
		DEBUG_COND(aflags[bp].level,bv_print((*block_ptr)->bool_true));
		DEBUG(aflags[bp].level,("\tbool_false: "));
		DEBUG_COND(aflags[bp].level,bv_print((*block_ptr)->bool_false));
		DEBUG(aflags[bp].level,("\texprs_killed: "));
		DEBUG_COND(aflags[bp].level,bv_print((*block_ptr)->exprs_killed));
		pr_block_list((*block_ptr)->pred,"\tpreds");
		pr_block_list((*block_ptr)->succ," succs");
		fprintf(stderr,"\n");
		fflush(stderr);
	END_DEPTH_FIRST
}

static void
debug_live_on_entry(arena)
Arena arena;
{
	DEPTH_FIRST(block_ptr)
		DEBUG(aflags[df].level,("block %d\n",((*block_ptr)->block_count)));
		DEBUG(aflags[df].level,("\tlive_on_entry: "));
		DEBUG_COND(aflags[df].level,
			pr_object_set((*block_ptr)->live_on_entry));
		DEBUG(aflags[df].level,("\tuse: "));
		DEBUG_COND(aflags[df].level,pr_object_set((*block_ptr)->use));
		DEBUG(aflags[df].level,("\tdef: "));
		DEBUG_COND(aflags[df].level,pr_object_set((*block_ptr)->def));
	END_DEPTH_FIRST
}

static void
debug_live_at_bottom(arena)
Arena arena;
{
	DEPTH_FIRST(block_ptr)
		DEBUG(aflags[df].level,("block %d\n",((*block_ptr)->block_count)));
		DEBUG(aflags[df].level,("\tlive_at_bottom: "));
		DEBUG_COND(aflags[df].level,
			pr_object_set((*block_ptr)->live_at_bottom));
	END_DEPTH_FIRST
}

#endif
