#ident	"@(#)amigo:common/reg_alloc.c	1.8.1.58"
#include "amigo.h"
#include "scopes.h"
#include "costing.h"

#ifndef REG_WT_INIT
#define REG_WT_INIT(s) SY_WEIGHT(s) = \
	((SY_CLASS(s) == SC_AUTO ? 0 : (SY_CLASS(s) == SC_PARAM ? -1 : -2)) \
	+ (SY_FLAGS(s) & SY_ISREG))
#endif
#ifndef REG_WT_INIT
#define REG_WT_INIT 1
#endif
#ifndef REG_WT_THRESHOLD
#define REG_WT_THRESHOLD 2
#endif

static Arena spill_arena;
static do_spills_flag = 0;

	/*
	** Give nodes a weight depending inversely on the number
	** of neighbors.  A node with few neighbors will
	** have high INDEPENDENCE.  We will try to color such nodes
	** first, because coloring them will tend least to constrain
	** the further coloring of other nodes.
	*/
#ifndef NODBG

		/*
		** Fake object list so we can see offset allocations
		** for spill temps ( which are never put on amigo's
		** object list. )
		*/
static Object spill_temps;

#else
#define SCALE 64
#endif /* ifndef NODBG */

int reg_slope = 1;
int reg_height = 1;

#define INDEPENDENCE(neighbors) \
	(reg_slope * (max_neighbors -  neighbors) + reg_height)

#define IS_GLOBAL(sid)	\
	(SY_CLASS(sid) == SC_EXTERN || SY_CLASS(sid) == SC_STATIC)

#ifndef CONTAINS_DIV
#define CONTAINS_DIV 0
#define CONTAINS_FP_DIV 0
#endif

#define CONTAINS_ASSIGN (1<<28)
#define IS_ASSIGN (1<<30)
#define CONTAINS_REFS (1<<29)

/* make sure our constants are distinct from external constants */

#if (IS_ASSIGN|CONTAINS_ASSIGN|CONTAINS_REFS) & (CONTAINS_CALL|CONTAINS_DIV|CONTAINS_FP_DIV)
/* Cause a compile time error */
here is an error
#endif


/* file function declarations */
static void make_interference();
static int find_defs_refs();
static void add_interference();
static void update_objects();
static struct Object_info *reduce_graph();
static void color_interference();
static int allocated();
static void alloc_symbol();
static void dealloc_symbol();
static void init_objects();
static void init_color_by_spill();
static void gen_save_restores();
static void allocated_global();
#ifndef NODBG
static void print_obj();
#endif

static struct Object_info **obj_info_array;

static max_neighbors;

/* MAIN ENTRY POINT */

void
reg_alloc(reg_arena, top_down_basic) 
Block top_down_basic;
Arena reg_arena;
{
	extern int do_inline;

	struct Object_info *first_object;

		/* Amigo may have altered the decision to use fixed frame
		** so cg's notion of available registers needs to be
		** set again.
		*/
	set_max_user_reg();

	DEBUG_COND(aflags[tbra].level&4, print_trees(3,top_down_basic));


	do_spills_flag =
#ifndef NODBG
		aflags[sp].xflag &&
#endif
			/* Don't attempt spills if can't sort out pass thrus */
		build_scopes(reg_arena,top_down_basic);

	DEBUG_COND(aflags[tbra].level&1,
		print_trees((int)aflags[tbra].level,top_down_basic));

	get_flow_weights(top_down_basic, reg_arena);
	init_objects(reg_arena);
	max_neighbors = 1; /* Count the node as its own neighbor */
	live_at_bottom(OBJECT_SET_SIZE,reg_arena, CAN_BE_IN_REGISTER);
	make_interference(reg_arena);
	first_object = reduce_graph(reg_arena);
	color_interference(first_object);

}

static void
init_objects(arena) Arena arena; {
	struct Object_info *objct;
#ifndef NODBG
	spill_temps = 0;
#endif
	spill_arena = arena;

		/* one extra, since will not use 0'th slot */
	obj_info_array=Arena_alloc(arena, OBJECT_SET_SIZE+1,
		struct Object_info *);

	init_color_by_spill();
	for (objct = get_object_last();  objct ; objct = objct->next) {
		if (!(objct->flags&CAN_BE_IN_REGISTER) ) {
			objct->set_interferes = NULL;
			continue;
		}
		obj_info_array[objct->setid] = objct;
		objct->weight = REG_WT_INIT(objct->fe_numb) * BLOCK_WEIGHT_INIT;
		objct->reg = -1;
		objct->set_interferes=Object_set_alloc(arena);
		bv_init(false,objct->set_interferes);
		
	}
}

static void
make_interference(reg_arena) Arena reg_arena;
{
	Object_set  refs = Object_set_alloc(reg_arena);
	Object_set  must_defs = Object_set_alloc(reg_arena);
	Object_set  may_defs = Object_set_alloc(reg_arena);
	Object_set  live = Object_set_alloc(reg_arena);
	Object_set  live_after_refs = Object_set_alloc(reg_arena);
	Object_set  live_in_block = Object_set_alloc(reg_arena);
	int first_block = 1;
	
        DEPTH_FIRST(block_ptr)
		Block block = *block_ptr;
		unsigned state_flags = (CONTAINS_REFS|IS_ASSIGN);
		unsigned flags = 0;
                int weight = get_block_weight(block);
		bv_assign(live, block->live_at_bottom);
		bv_assign(live_in_block, block->live_at_bottom);
#ifndef NODBG
		if (aflags[ra].level&16) {
			DPRINTF("Doing block %d\n", block->block_count);
		}
#endif

                FOR_ALL_ND1_IN_BLOCK_REVERSE(block,flow,node,index)
			if (state_flags & (CONTAINS_ASSIGN|IS_ASSIGN)) {
				bv_init(false, must_defs);
				bv_init(false, may_defs);
			}
			if (state_flags & CONTAINS_REFS)
				bv_init(false, refs);

			if(node->op != NAME || flow->cgq_op == CGQ_FIX_SWBEG)
#define TOP 1
				state_flags =
					find_defs_refs(TOP, flow, node, must_defs,
						may_defs, refs, weight, 0);
#ifndef NODBG
			if (aflags[ra].level&16) {
				dprint1(node);
				DPRINTF("flags=0x%x live must may ref\n", state_flags);
				bv_print(live);
				bv_print(must_defs);
				bv_print(may_defs);
				bv_print(refs);
				DPRINTF("\n\n");
				
			}
#endif

			flags |= state_flags;
			bv_or_eq(live_in_block, refs);

			/* at this point may want to add the block to
			   def_ref list for objects in defs and refs and
			   to live list for objects in live_after_refs
			   may want to also figure may def inaddition to
			   must defs that are returned for pass-thru logic
			*/

			/* add interference between d and objects in live */
			if (state_flags & IS_ASSIGN)
				add_interference(node->opt->object, live,
				    flow->cgq_ln
#ifndef NODBG
					, CGQ_INDEX_NUM(index)
#endif
			);

			/* following case is invalid C, we conservatively
			   assume that defs may interfere with refs */
			else if (state_flags&CONTAINS_ASSIGN)  {
				/* RMA need bv_or */
				bv_assign(live_after_refs,live);
				bv_or_eq(live_after_refs, refs);
				BV_FOR(may_defs,d)
					add_interference(SETID_TO_OBJ(d), 
				    	    live_after_refs ,flow->cgq_ln
#ifndef NODBG
				            , CGQ_INDEX_NUM(index)
#endif
					);
				END_BV_FOR
			}

			if (state_flags & (CONTAINS_ASSIGN|IS_ASSIGN))
				bv_minus_eq(live,must_defs);
			if (state_flags & CONTAINS_REFS)
				bv_or_eq(live,refs);
                END_FOR_ALL_ND1_IN_BLOCK

		if (first_block) {
			/* parameters/globals are defined on entry */
			struct Object_info *objct;
			first_block = 0;
			for (objct=get_object_last(); objct; objct = objct->next) {
				if (objct->flags&CAN_BE_IN_REGISTER &&
				   SY_CLASS(objct->fe_numb) != SC_AUTO)
					add_interference(objct, live, -1
#ifndef NODBG
							, -1
#endif
					);
			}
		}

		update_objects(flags, live_in_block);
        END_DEPTH_FIRST
	{
		struct Object_info *objct;
		for (objct = get_object_last(); objct; objct = objct->next) {
			if (objct->flags & CAN_BE_IN_REGISTER) {
				int count =
					bv_set_card(objct->set_interferes) + 1;
				if(count > max_neighbors)
					max_neighbors = count;
			}
		}
	}
#ifndef NODBG
    	if (aflags[ra].level&2)  {
		struct Object_info *objct;
		DPRINTF("max_neighbors: %d\n", max_neighbors);
		for (objct = get_object_last(); objct; objct = objct->next) {
			SX sid = objct->fe_numb;
			if (objct->flags & CAN_BE_IN_REGISTER) {
				DPRINTF("object=%d %s(%d) indep=%d interferes", 
				    objct->setid,SY_NAME(sid), sid,
				    INDEPENDENCE(bv_set_card(objct->set_interferes)));
				bv_print(objct->set_interferes);
				scope_usage_debug("",objct);
			}
		}
	}
#endif
}

static int
find_defs_refs(top_level, flow, node, must_defs, may_defs, refs, weight, context)
Boolean top_level;
cgq_t *flow;
Object_set must_defs, may_defs, refs;
ND1 * node;
int weight;
int context;
{
	int flags=0;
	int assign_case = 0;
	int next_top_level = 0; /* Hack to allow assigns separated
				** by top level comma ops to
				** be treated as top level assigns.
				*/
	int normal_assign = 1;
	switch (node->op) {
	ASSIGN_CASES:
                if( node->left->opt->flags & OBJECT &&
		    node->left->opt->object->flags & CAN_BE_IN_REGISTER) {
			assign_case = 1;

			if (!(context&IS_COND)) /* unconditional assign */
				bv_set_bit(node->left->opt->object->setid,
				    must_defs);
			bv_set_bit(node->left->opt->object->setid, may_defs);
		}

		if(assign_case && (node->op != ASSIGN ||
			TY_SIZE(SY_TYPE(node->left->opt->object->fe_numb))
                                > TY_SIZE(node->type)))
			normal_assign = 0;

		if (node->op == ASG DIV || node->op == ASG MOD) {
			if (TY_ISFPTYPE(node->type))
				flags = CONTAINS_FP_DIV;
			else
				flags = CONTAINS_DIV;
		}
		break;
	case NAME:
		/* check if NAME is a candidate */
		if ( node->opt->object->flags & CAN_BE_IN_REGISTER ) {

				/* add into weighting */

			node->opt->object->weight +=  
				context&UNDER_OP_EQ ? weight*2 : weight;
			if (node->opt->object->weight > MAX_WEIGHT)
				node->opt->object->weight = MAX_WEIGHT;

			if(do_spills_flag) {
				Scope scope;
				scope = scope_of_cg_q(flow);

				bump_scope_usage(node->opt->object,scope,
					context&UNDER_OP_EQ ? weight*2 : weight);
			}


			/* Make 3 lines else clause when scopes work -- PSP */

			if (!(context & IS_LVAL)) {
				bv_set_bit(node->opt->object->setid,refs);
				flags = CONTAINS_REFS;
			}
		}
		break;
	
	case DIV:
	case MOD:
		if (TY_ISFPTYPE(node->type))
			flags = CONTAINS_FP_DIV;
		else
			flags = CONTAINS_DIV;
		break;
	case CALL:
	case UNARY CALL:
	case STCALL:
	case UNARY STCALL:
		flags = CONTAINS_CALL;
		break;
	case COMOP:
		if(top_level)
			next_top_level = 1;
	}

	if (optype(node->op) == LTYPE)
		/* EMPTY */;
	else if (assign_case && (!normal_assign  || !top_level)) {
		/* treat lhs or +=, |=, etc. as an RVAL rather than lval */
		/* Also treat embedded assigns similarly, as they
		** may be evaluated for effect.  E.g.,
		** want i and j to interfere for foo(i=0, j=1);
		*/
		flags |= find_defs_refs(next_top_level, flow, node->left, must_defs,
				may_defs, refs, weight,
				(node->op != ASSIGN ?
					context | UNDER_OP_EQ :
					context)); 
}
	else
		flags |= find_defs_refs(next_top_level,
				flow, node->left, must_defs,
				may_defs, refs, weight,
				get_context(node,LEFT,context));

	if (optype(node->op) == BITYPE)
		flags |= find_defs_refs(next_top_level,
			flow, node->right,must_defs, may_defs, refs, 
			weight, get_context(node,RIGHT,context));

	if (flags&(IS_ASSIGN|CONTAINS_ASSIGN)) {
		flags |= CONTAINS_ASSIGN;
		flags &= ~IS_ASSIGN;
	}
	else if (assign_case)
		flags |= IS_ASSIGN;
		
	return flags;
}


/* RMA get object_info code here, add interference between object and all
   live
*/
static void
add_interference(object1, live, line
#ifndef NODBG
, cgq_numb
#endif
)
struct Object_info *object1;
Object_set live;
#ifndef NODBG
int cgq_numb;
#endif
{
	if (!(object1->flags&CAN_BE_IN_REGISTER)) {
		DEBUG(aflags[ra].level,("Called add_interference on non \
registerable object %s(fe_numb %d)\n",
				SY_NAME(object1->fe_numb), object1->fe_numb
		));
		return;
	}
	BV_FOR(live, objct)
		struct Object_info *object2 = SETID_TO_OBJ(objct);
		if (object2->regclass != object1->regclass)
			continue;
		if (objct == object1->setid)
			continue;
		assert(object2->flags & CAN_BE_IN_REGISTER);
#ifndef NODBG
		if (aflags[ra].level&8) {
			if (!bv_belongs(objct, object1->set_interferes))
				DPRINTF("add inter(cgq=%d line=%d) %s(%d) %s(%d)\n",
				cgq_numb, line, 
				SY_NAME(object1->fe_numb), object1->fe_numb,
				SY_NAME(object2->fe_numb), object2->fe_numb);

		}
#endif
		bv_set_bit(objct, object1->set_interferes); 
		bv_set_bit(object1->setid, object2->set_interferes); 
	END_BV_FOR
}

static void
update_objects(flags, live_in_block)
unsigned flags;
Object_set live_in_block;
{
	flags &= ~(CONTAINS_ASSIGN|IS_ASSIGN|CONTAINS_REFS);
	BV_FOR(live_in_block, cand)
		struct Object_info *objct = SETID_TO_OBJ(cand);
	END_BV_FOR
}


struct Candidates {
	struct Object_info * object;
	int weight;
};

static void
remove_interferes(obj)
Object obj;
{
	BV_FOR(obj->set_interferes,bit)
		Object interfer =  SETID_TO_OBJ(bit);
		bv_clear_bit(obj->setid, interfer->set_interferes);
		/* Note that we do NOT change this object's interferes! */
	END_BV_FOR
}

#define UNCONSTRAINED(o) \
	(is_free_register(o->regclass, bv_set_card(o->set_interferes)))

static struct Object_info *
reduce_graph(reg_arena) 
Arena reg_arena;
{


	struct Object_info *obj, *coloring_stack = 0;
	Object prior_obj = 0;

	for (obj=get_object_last(); obj; obj=obj->next) {
		if (!(obj->flags&CAN_BE_IN_REGISTER)) {
			remove_object(prior_obj, obj);
			continue;
		}
		if  (obj->weight > 0 && obj->weight < BLOCK_WEIGHT_INIT)
			obj->weight = 1;
		else
			obj->weight /= BLOCK_WEIGHT_INIT;
		prior_obj = obj;
	}

		/*
		** Unconstrained nodes are nodes that can be
		** colored regardless of the colors assigned to
		** the neighbors.  For example, if a node has
		** order 3 and 4 colors are available, we can
		** always color the node.  If we remove unconstrained
		** nodes from the graph, new unconstrained nodes
		** may arise, and these can also be removed.
		** We continue this process until we find no
		** unconstrained nodes in the reduced graph.
		** Once the reduced graph is colored, we may
		** color the full graph by coloring the deleted
		** nodes in reverse order to their removal.  This
		** algorithm is due to Chaitin.
		*/

	while((obj = get_object_last()) != NULL) {

		DEBUG_COND(aflags[ra].level&128,obj_debug("before remove",obj));
			
		/* Cycle over list looking for unconstrained nodes. 
		** Each time we remove a node, we create new 
		** uncontrained nodes so, we start over from the 
		** beginning  of the list.
		*/
		prior_obj = 0;
		while(obj) {
			assert(obj->flags&CAN_BE_IN_REGISTER);
			if (!(UNCONSTRAINED(obj))) {
				prior_obj = obj;
				obj = obj->next;
				continue;
			}
			remove_interferes(obj);
			remove_object(prior_obj,obj);
			obj->next = coloring_stack;
			coloring_stack = obj;

			DEBUG_COND(aflags[ra].level&128,
				obj_debug("coloring_stack", coloring_stack));
			obj = get_object_last();
			prior_obj = 0;
		}

		if((obj = get_object_last()) != NULL) {
			/* All objects are constrained */
			Object victim = 0, victim_prior_obj;
			DEBUG_UNCOND(static int pass = 0;)
			int victim_weight = 0;

			DEBUG_UNCOND(pass++;)
			DEBUG(aflags[ra].level&128,("%d\n",pass));
			DEBUG_COND(aflags[ra].level&128,
				obj_debug("after pass ",obj));

			/* Remove the node that has a combination
			** of a lot of neighbors light usage.  If we
			** found an object that is used below the
			** threshold, throw it first:  it will reduce
			** some tension for minimal cost, and it will
			** have a better chance of getting a register
			** later on because there should be no penalty
			** associated with allocating the variable to
			** a register.
			*/
			for (prior_obj=0; obj; prior_obj=obj, obj=obj->next) {
				int wgt;

				if (obj->weight < REG_WT_THRESHOLD) {
					victim = obj;
					victim_prior_obj = prior_obj;
					break;
				}
				wgt = obj->weight * 
					INDEPENDENCE( bv_set_card(obj->set_interferes));

				if(!victim || wgt < victim_weight) {
					victim = obj;
					victim_weight = wgt;
					victim_prior_obj = prior_obj;
				}

			}
found_victim:
			assert(victim);

			DEBUG(aflags[ra].level&128, (
				"\tRemoving victim in reduce_graph:\n"));
			DEBUG_COND(aflags[ra].level&128,print_obj(victim));
			remove_interferes(victim);
			remove_object(victim_prior_obj, victim);

			/* Here is slight tweak to the Chaitin algorithm.
			** Since we know we can color the unrestrained
			** nodes regardless of how we color anything else,
			** we may as well put the constrained node on the
			** stack and try to color it.  Note that we will
			** try to color the "best" constrained node first,
			** so this is cost effective.
			*/
			victim->next = coloring_stack;
			coloring_stack = victim;
		}
	}
	
	set_object_last(coloring_stack);

	DEBUG_COND(aflags[ra].level&512,
		obj_debug("FINAL",get_object_last()));


	return coloring_stack;
	
}


static int
get_reg(objct)
Object objct;
{
	char regs[TOTREGS];
	SX sid = objct->fe_numb;
	int j;

	for (j=0;j<TOTREGS;++j)
		regs[j]= 0;

		/* find all other reg cands interfering with */
		/* the current candidate		     */
	BV_FOR(objct->set_interferes,bit)
		struct Object_info * interfer =  SETID_TO_OBJ(bit);
		if (interfer->reg > -1){
			TWORD cgt = cg_tconv(SY_TYPE(
				interfer->fe_numb),0);
			al_regset(regs,interfer->reg,szty(cgt));
		}
	END_BV_FOR

	return cisreg(cg_tconv(SY_TYPE(sid),0), regs);
}

	/*
	** Check no object, other than obj, in object_set has been
	** given this register.
	*/
static Boolean
already_allocated(obj_id, obj, object_set)
Bit obj_id;
Object obj;
Object_set object_set;
{
	BV_FOR(object_set, bit)
		struct Object_info *interfer =  SETID_TO_OBJ(bit);
		if(interfer->reg == obj->reg && obj_id != bit)
			return true;
	END_BV_FOR
	return false;
}

static void restore_interferes();

static void push_color_by_spill();
static void get_spills();

static void
color_interference(first_object)
struct Object_info *first_object;
{

	int j;
	Object objct, next_object, prior_object;
	char global_regs[TOTREGS];
	for (j=0;j<TOTREGS;++j)
		global_regs[j]= 0;




	for (objct = first_object, prior_object = 0; objct; objct = next_object) {
		SX sid = objct ->fe_numb;
		TWORD cgtype = cg_tconv(SY_TYPE(sid),0);
		int regno = -1;
		next_object = objct->next;

		regno = get_reg(objct);

		if(regno >= 0) {
			int regcnt = szty(cgtype);
			if (objct->weight >= REG_WT_THRESHOLD || 
				allocated(global_regs,regno,regcnt)
				&& objct->weight > 0) {

				al_regset(global_regs,regno,regcnt);
				SY_REGNO(sid) = regno;
				al_regupdate(regno,szty(cgtype));
				objct->reg = regno;
				if (IS_GLOBAL(sid)) 
					allocated_global(objct, sid, regno);
				DEBUG(aflags[ra].level&64, (
					"Allocating reg(%d) to object %d %s(sid %d)\n",
					regno, objct->setid, OBJ_TO_NAME(objct), objct->fe_numb));

			}
			prior_object = objct;
		}
		else {
			DEBUG(aflags[ra].level&64, ( "NOT allocating reg to object %d %s(sid %d)\n",
					objct->setid, OBJ_TO_NAME(objct), objct->fe_numb));
			remove_object(prior_object,objct);
			push_color_by_spill(objct);
		}
		restore_interferes(objct, next_object);
	}
	if(do_spills_flag) {
		get_spills();
#ifndef NODBG
		debug_spills();
#endif
		gen_save_restores();
	}
	return;
}

	/*
	** Put this object " back in the graph " by restoring interferences
	** with items that were removed from the graph in the reduction
	** process.
	*/
static void
restore_interferes(objct, stack)
Object objct, stack;
{
	while(stack) {
		if(bv_belongs(objct->setid, stack->set_interferes))
			bv_set_bit(stack->setid, objct->set_interferes);
		stack = stack->next;
	}
}

static Object color_by_spill;

static void
init_color_by_spill()
{
	color_by_spill = 0;
}

	/* 
	** Insert by weight.  Maybe could just push and sort at 
	** the end. -- PSP
	*/
static void
push_color_by_spill(objct)
Object objct;
{
	Object obj, prior_object;
	DEBUG(aflags[ra].level&64, ( "Pushing object %s(setid %d, sid %d)\n",
		OBJ_TO_NAME(objct), objct->setid, objct->fe_numb));
	prior_object = 0;
	for(obj = color_by_spill; obj ; prior_object = obj, obj = obj->next) {
		if(obj->weight < objct->weight)
			break;
	}
	if(prior_object) {
		objct->next = prior_object->next;
		prior_object->next = objct;
	}
	else {
		objct->next = color_by_spill;
		color_by_spill = objct;	
	}
}

typedef struct victim_list *Victim;

struct victim_list {
	Object obj;
	Scope scope;
	Victim next;
};

static Victim victim_list;

static void
init_victims()
{
	victim_list = 0;
}

static void
mark_spill(victim, scope)
Object victim;
Scope scope;
{
	Victim v, remember_victim = 0;
	int add_to_list;

	for(v = victim_list; v; remember_victim = v, v = v->next) {
		if(v->obj == victim)
			break;
	}

	add_to_list = 1;

		/* if v then this victim is in the list for some scope */
	while(v && v->obj == victim) {
			/*
			** If the new spill region is a subset
			** of a region already on the list,
			** we don't want to spill around it.
			*/
		if(contain(scope, v->scope)) {
			add_to_list = 0;
			break;
		}
			/*
			** If the new region contains any region
			** on the list, we remove the smaller region.
			*/
		else if(contain(v->scope,scope)) {
			if(remember_victim)
				remember_victim->next = v->next;
			else
				victim_list = victim_list->next;	
		}
		else
			remember_victim = v;
		v = v->next;
	}

	if(add_to_list) {

		v = Arena_alloc(spill_arena, 1, struct victim_list);
		v->obj = victim;	
		v->scope = scope;

		if(remember_victim) {
			v->next = remember_victim->next;
			remember_victim->next = v; 
		}
		else {
			v->next = victim_list;
			victim_list = v;
		}
	}
}

#ifndef NODBG
static 
debug_spills()
{
	Victim v;
	DEBUG(aflags[ra].level&32, ("Spills:\n=========\n"));
	for(v = victim_list; v; v = v->next) {
		DEBUG(aflags[ra].level&32, (
			"\tobject %d %s(sid %d) weight(%d) in scope %d\n",
				v->obj->setid,
				OBJ_TO_NAME(v->obj), 
				v->obj->fe_numb,
				v->obj->weight,
				debug_scope_id(v->scope))
		);
	}
}
#endif

	/*
	** We want to spill victim around scope.
	** maybe we can find a containing scope for which
	** the cost is cheaper.  In particular the containing
	** scope may be outside a loop which contains the original
	** scope, so we will execute the save_restore code less
	** frequently.
	*/
static Scope
get_best_spill_region(object_to_spill, spill_region, min_use)
Object object_to_spill;
Scope spill_region;
int *min_use;
{
	int use;
	Scope sc;

		/*
		**Look outward for scope with smallest use
		** of object_to_spill.
		*/

		/*
		** First check object_to_spill's scope strictly contains the
		** spill region
		*/
	if( object_to_spill->scope == spill_region
		|| ! contain(spill_region, object_to_spill->scope) ) {
		return 0;
	}

	*min_use = usage_in_scope(object_to_spill, spill_region) + spill_cost(spill_region);

	for(sc = parent_scope(spill_region); sc
&&
		sc != object_to_spill->scope
;
		sc = parent_scope(sc)) {

		use = usage_in_scope(object_to_spill, sc) + spill_cost(sc);
		if (use < *min_use) {
			*min_use = use;
			spill_region = sc;
		}
	}
	return spill_region;
}

static void
get_spills()
{
	Object object_to_color; 
	init_victims();
	for(object_to_color = color_by_spill; object_to_color; object_to_color = object_to_color->next) {
		Object victim;		/* Object with register we want*/
		Object spill_obj;	/* Object we will spill */
		Scope spill_region;
		int use, benefit, max_benefit;

		assert(object_to_color->scope);
		DEBUG(aflags[ra].level&64, (
		"Try for victim for object %d %s(sid %d) weight(%d) in scope %d\n",
				object_to_color->setid,
				OBJ_TO_NAME(object_to_color), 
				object_to_color->fe_numb,
				object_to_color->weight,
				debug_scope_id(object_to_color->scope))
		);
			
		/*
		** It's not worth spilling unless the
		** benefit exceeds the cost of a store
		** and a load of stack <-> register.
		** We can break because the list is sorted
		** by weight.
		*/
		if(object_to_color->weight < REG_WT_THRESHOLD) {
			DEBUG(aflags[ra].level&32, ( "NOT worth it!\n"));
			break;
		}
		/* We cannot color a global via spill until we make sure that
		** the register is properly initialized from the global
		** and the global is later restored.
		*/
		if(IS_GLOBAL(object_to_color->fe_numb)) {
			DEBUG(aflags[ra].level&32, ( "Cannot color global via spill\n"));
			break;
		}
		/*
		** Look for interfering variables which
		** have been colored: these are possible
		** spill victims.
		*/
		victim = 0;
		max_benefit = 0;

		BV_FOR(object_to_color->set_interferes,bit) 

		Object colored_nbr =  SETID_TO_OBJ(bit);
		Object obj;
		Scope candidate_spill_region;

		if (colored_nbr->reg < 0 || 
		    colored_nbr->scope == object_to_color->scope) 

			BV_CONTINUE;

		/*
		** See if interfering variable is seldom
		** used in the scope of the variable we
		** are trying to color.
		*/

		DEBUG(aflags[ra].level&32, (
		"Check colored_nbr %d, attempting to color %d\n",
		colored_nbr->setid,object_to_color->setid));

		candidate_spill_region =
			get_best_spill_region(colored_nbr, object_to_color->scope, &use );
		use /= BLOCK_WEIGHT_INIT;
		benefit = object_to_color->weight - use;

		if( candidate_spill_region &&
			(benefit > max_benefit) &&
			!already_allocated(bit, colored_nbr, object_to_color->set_interferes)) {
			victim = colored_nbr;
			spill_obj = victim;
			spill_region = candidate_spill_region;
			max_benefit = benefit;

			DEBUG(aflags[ra].level&32, (
			"Found colored victim %d, spill_region %d\n",
			victim->setid,
			debug_scope_id(spill_region)));
		}

		/* 
		** Maybe we can spill object_to_color
		** around colored_nbr->scope.  
		*/
		candidate_spill_region =
			get_best_spill_region(object_to_color, colored_nbr->scope, &use );
		use /= BLOCK_WEIGHT_INIT;
		benefit = object_to_color->weight - use;

		if( candidate_spill_region &&
			(benefit > max_benefit) &&
			!already_allocated(bit, colored_nbr, object_to_color->set_interferes)) {
			victim = colored_nbr;
			spill_obj = object_to_color;
			spill_region = candidate_spill_region;
			max_benefit = benefit;

			DEBUG(aflags[ra].level&32, (
			"Found uncolored victim %d, spill_region %d\n",
			victim->setid,
			debug_scope_id(spill_region)));
		}
		END_BV_FOR

		if (victim) {

			int regno;
			int vid = victim->setid;

			/*
			** At this point we think we can
			** get a register for object_to_color if we
			** spill victim around spill_region.
			** It does not hurt at this point to
			** delete the edge from colored_nbr to
			** object_to_color.  If we are successful at
			** getting the register, we will eventually
			** generate the spill code to force
			** this interference to be deleted.
			*/
			bv_clear_bit(vid, object_to_color->set_interferes);

			regno = get_reg(object_to_color);

			if(regno < 0) {
				DEBUG_COND(aflags[ra].level&256, spill_debug(
					"Failed to spill", victim,
					object_to_color, spill_region));
				bv_set_bit(vid, object_to_color->set_interferes);

			}
			else {
				mark_spill(spill_obj, spill_region);
				object_to_color->reg = regno;
				SY_REGNO(object_to_color->fe_numb) = regno;

				/* Cannot assume that the size of the spilled object is
				   the same as the object_to_color. */
				al_regupdate(regno,
				          szty(cg_tconv(SY_TYPE(object_to_color->fe_numb),0)));

				DEBUG(aflags[ra].level&32, (
					"Got reg %d for object_to_color \
%d %s(sid %d) weight(%d) in scope %d victim %d(%s) spill region %d\n",
					regno,
					object_to_color->setid,
					OBJ_TO_NAME(object_to_color), 
					object_to_color->fe_numb,
					object_to_color->weight,
					debug_scope_id(object_to_color->scope),
					vid,
					OBJ_TO_NAME(victim),
					debug_scope_id(spill_region)	
				));

				DEBUG_COND(aflags[ra].level&1, spill_debug(
					"Spilling", spill_obj, 
					victim, spill_region));
			}
		}
	}
}

#ifndef NODBG

	/*
	** Push a fake object_info for spill_temp ( which does
	** not appear on the regular object list.  Just for debugging.
	*/
static void
add_to_spill_temps(sid)
SX sid;
{
	struct Object_info * obj =
	Arena_alloc(spill_arena, 1, struct Object_info);
	obj->value = (ND1 *)0;
	obj->flags = 0;
	
	obj->setid = 0;
	obj->fe_numb = sid;
	obj->scope = 0;
	obj->scope_usage = 0;
	obj->next = spill_temps;
	spill_temps = obj;
}
#endif

static void
replace_name_by_temp(name, temp, node)
SX name, temp;
ND1 *node;
{
	if(node == 0)
		return;
	if(node->op == NAME) {
		if(node->sid == name)
			node->sid = temp;
		return;
	}
	else {
		replace_name_by_temp(name,temp,node->right);
		replace_name_by_temp(name,temp,node->left);
	}
}

static void
gen_save_restores()
{
	Victim victim;

	for(victim = victim_list; victim; victim = victim->next) {
		TEMP_SCOPE_ID temp_scope;
		SX temp_sx; 
		TWORD temp_type;
		ND1 *t1, *t2;
		cgq_t *cgq_item;

		temp_scope.first = SPILL_SAVE_INDEX(victim->scope);
		temp_scope.last = spill_restore_index(victim->scope);
		DEBUG(aflags[ra].level&2, (
			"Generate save restore for victim %d %s(%d) \
scope(%d) around scope(%d)\n",
			victim->obj->setid,
			OBJ_TO_NAME(victim->obj),
			victim->obj->fe_numb,
			debug_scope_id(victim->obj->scope),
			debug_scope_id(victim->scope)
			
		));
		DEBUG(aflags[ra].level&32, (
			"\tFirst flow (scope(%d)):%d\tLast flow:%d\n",
			debug_scope_id(victim->scope),
			CGQ_INDEX_NUM(temp_scope.first),
			CGQ_INDEX_NUM(temp_scope.last)
			)
		);
		assert(temp_scope.first != CGQ_NULL_INDEX);
		assert(temp_scope.last != CGQ_NULL_INDEX);
		temp_type = SY_TYPE(victim->obj->fe_numb);	
		temp_sx = sy_temp(temp_type);
		DEBUG_UNCOND(SY_NAME(temp_sx) = ".temp");
			al_add_to_scope(temp_sx,temp_scope);
		DEBUG_UNCOND(add_to_spill_temps(temp_sx));

		if(usage_in_scope(victim->obj, victim->scope)) {	/*
			** In the interval of the CGQ delineated by temp_scope,
			** replace all occurrences of name by temp.
			** Could make more efficient by only doing at most
			** use replacements.
			*/
		CGQ_FOR_ALL_BETWEEN(flow, index, temp_scope.first, temp_scope.last)
			ND1 *node;
			if(node = HAS_ND1(flow)) {
				replace_name_by_temp(victim->obj->fe_numb, temp_sx, node);
			}
		CGQ_END_FOR_ALL
	}

		t1 = t1alloc();
		t1->op = NAME;
		t1->sid = temp_sx;
		t1->type = temp_type;
		t1->c.off = 0;
		t2 = t1alloc();
		t2->op = ASSIGN;
		t2->flags = FF_SEFF;
		t2->left = t1;
		t2->type = temp_type;
		t1 = t1alloc();
		t1->op = NAME;
		t1->sid = victim->obj->fe_numb;
		t1->c.off = 0;
		t1->type = temp_type;
		t2->right = t1;	
		cgq_item = cg_q_insert(temp_scope.first);
		cgq_item->cgq_scope_marker = victim->scope->scope_begin + 1;
		cgq_item->cgq_op = CGQ_EXPR_ND1;
		cgq_item->cgq_arg.cgq_nd1 = t2;
		new_expr(t2,0);

		if(is_live_at_restore_index(victim->obj, temp_scope.last, victim->scope)) {
			t1 = tr_copy(t2);
			t2 = t1->left;
			t1->left = t1->right;
			t1->right = t2;
			cgq_item = cg_q_insert(temp_scope.last);
			cgq_item->cgq_op = CGQ_EXPR_ND1;
			cgq_item->cgq_arg.cgq_nd1 = t1;
		cgq_item->cgq_scope_marker = victim->scope->scope_end;
			new_expr(t1,0);
		}
#ifndef NODBG
		else DEBUG(aflags[ra].level&1, (
			"No generation of restore for victim %d %s(%d) \
\n",
			victim->obj->setid,
			OBJ_TO_NAME(victim->obj),
			victim->obj->fe_numb));
#endif
	}
}



static int
allocated(reglist,regno,nregs)
char reglist[];
int regno;
int nregs;
/* Test is regno is allocated */
{
    int i;

    for (i = 0; i < nregs; ++i)
	if (reglist[regno+i])
		return 1;
    return 0;
}

static void
allocated_global(obj, sid, regno)
Object obj;
SX sid;
int regno;
{
	int load = 1, restore = 1;
	int relocation;
	Block block;
	DEBUG(aflags[ra].level, ("Global in register: reg(%d) to object %d %s(sid %d)\n",
		regno, obj->setid, OBJ_TO_NAME(obj), obj->fe_numb));
	block = *get_first_depth_first_block_ptr();
	if (SY_CLASS(sid) == SC_EXTERN) {
		relocation = NI_GLOBAL;
		if (!bv_belongs(obj->setid, block->live_on_entry))
			load = 0;
	}
	else if (SY_LEVEL(sid) == SL_EXTERN) {
		relocation = NI_FLSTAT;
		if (!bv_belongs(obj->setid, block->live_on_entry))
			load = 0;
	}
	else {
		relocation = NI_BKSTAT;
		if (!bv_belongs(obj->setid, block->live_on_entry))
			restore = load = 0;
	}
	if(TY_ISCONST(SY_TYPE(sid)))
		restore = 0;
	bind_global(cg_extname(sid), cg_tconv(SY_TYPE(sid),0), regno, 
			load, restore, relocation);
	cg_bind_global_sym(sid);
}



struct Alloc {
	SX sid;
	OFFSET end;
};
static struct Alloc *alloc_stack;
static struct Alloc *cur_alloc;
static OFFSET max_auto;

#ifdef	OPTIM_SUPPORT
static int regal_only,fp_regal_only;
static BITOFF regal_size;
static int
regal_sid(sid)
SX sid;
{   
	T1WORD t = SY_TYPE(sid);
	if (TY_SIZE(t) > SZLDOUBLE)
    	return false;
	if (!TY_ISSCALAR(t) || TY_ISVOLATILE(t))
	    return false;
	if (TY_TYPE(t) == TY_ENUM && ! TY_HASLIST(t))
		return false;
	if (SY_CLASS(sid) != SC_AUTO)
		return false;
	return true;	

}
#endif

static void
alloc_symbol(sid)
SX sid;
{
	T1WORD t = SY_TYPE(sid);
#ifdef	OPTIM_SUPPORT
	if (regal_only && TY_SIZE(t) != regal_size)	
		return; 
	if (regal_only && TY_ISFPTYPE(t) && ! fp_regal_only) /* FP regals are used by OPTIM */
		return;
	if (fp_regal_only && ! TY_ISFPTYPE(t)) /* FP regals only now */
		return;
	if (regal_only ^ regal_sid(sid)) 
		return;
#endif
	SY_OFFSET(sid) = next_temp(cg_tconv(t,0),TY_SIZE(t), TY_ALIGN(t));
	++cur_alloc;
	cur_alloc->sid = sid;
	cur_alloc->end = max_temp();
	max_auto = off_bigger(VAUTO, max_auto, cur_alloc->end);
}

static void
dealloc_symbol(sid)
SX sid;
{
	struct Alloc *p;

#ifdef	OPTIM_SUPPORT
	T1WORD t = SY_TYPE(sid);
	
	if (regal_only && TY_SIZE(t) != regal_size)	
		return; 
	if (regal_only && TY_ISFPTYPE(t) && ! fp_regal_only) /* FP regals are used by OPTIM */
		return;
	if (fp_regal_only && ! TY_ISFPTYPE(t)) /* FP regals only now */
		return;
	if (regal_only ^ regal_sid(sid)) 
		return;
#endif
	/* find sid on the allocation stack */
	for (p=cur_alloc; p>=alloc_stack && p->sid != sid; --p) ;
	if(p < alloc_stack) {
		DEBUG(aflags[ra].level&1,
		("Could not find sid(%d) in allocation stack.\n", sid));
		return;
	}

	/* if not deallocing cur_alloc, mark the Alloc entry for sid as free */
	if (p != cur_alloc) {
		p->sid = SY_NOSYM;	/* marks it free */
		return;
	}

	/* pop any free symbols */
	for (--cur_alloc; cur_alloc->sid == SY_NOSYM; --cur_alloc) ;

	/* reset allocation */
	set_next_temp(cur_alloc->end);

}

#ifdef	OPTIM_SUPPORT
static OFFSET
amigo_off1()
#else
OFFSET
amigo_off()
#endif
{
	Arena offset_arena = arena_init();
	alloc_stack = Arena_alloc(offset_arena, SY_SIZE+1, struct Alloc);
#ifndef OPTIM_SUPPORT
	off_init(VAUTO);
	max_auto = max_temp();
#else
	set_next_temp(max_auto);
#endif

	/* this dummy entry helps when the final symbol is deallocated */
	cur_alloc = alloc_stack ;
	cur_alloc->sid = (SX) -1;
	cur_alloc->end = max_auto;

	CGQ_FOR_ALL(root,index)
		switch (root->cgq_op) {
		case CGQ_CALL_SID: {
			SX sid = root->cgq_arg.cgq_sid;
			if (root->cgq_func == db_symbol && 
			    SY_CLASS(sid) == SC_AUTO && 
			    SY_REGNO(sid) == SY_NOREG &&
			    ! (root->cgq_flags & CGQ_DELETE)) {
			    alloc_symbol(sid);

			}
			else if (root->cgq_func == db_sy_clear &&
			    SY_CLASS(sid) == SC_AUTO && 
			    SY_REGNO(sid) == SY_NOREG &&
			    ! (root->cgq_flags & CGQ_DELETE)) {
			    dealloc_symbol(sid);
			}
			break;
		}
		case CGQ_START_SCOPE:
			if (SY_REGNO(root->cgq_arg.cgq_sid) == SY_NOREG)
				alloc_symbol(root->cgq_arg.cgq_sid);
			break;
		case CGQ_END_SCOPE:
			if (SY_REGNO(root->cgq_arg.cgq_sid) == SY_NOREG)
				dealloc_symbol(root->cgq_arg.cgq_sid);
			break;
		}
	CGQ_END_FOR_ALL

	arena_term(offset_arena);

	return max_auto;
}

#ifdef	OPTIM_SUPPORT
OFFSET
amigo_off()
{
	fp_regal_only = false;
	regal_only = true;
	off_init(VAUTO);
	max_auto = max_temp();
	regal_size = SZCHAR;
	amigo_off1();
	if(SZSHORT > SZCHAR) {
		regal_size = SZSHORT;
		amigo_off1();
	}
	if(SZINT > SZSHORT) {
		regal_size = SZINT;
		amigo_off1();
	}
	if(SZLONG > SZINT) {
		regal_size = SZLONG;
		amigo_off1();
	}
	if(SZLLONG > SZLONG) {
		regal_size = SZLLONG;
		amigo_off1();
	}
	fp_regal_only = true;
	regal_size = SZFLOAT;
	amigo_off1();
	if(SZDOUBLE > SZFLOAT) {
		regal_size = SZDOUBLE;
		amigo_off1();
	}
	if(SZLDOUBLE > SZDOUBLE) {
		regal_size = SZLDOUBLE;
		amigo_off1();
	}
	regal_only = fp_regal_only = false;
	max_auto = amigo_off1();
	return max_auto;
}
#endif			   

#ifndef NODBG

static void
print_obj(obj)
Object obj;
{
	SX sid = obj->fe_numb;
	int class = SY_CLASS(sid);
	if (sid <= 0)
		return;
	fprintf(stderr,"sid=%3d  name=%s weight=%d %s\n",
		sid, OBJ_TO_NAME(obj),
		obj->weight,
		class == SC_AUTO ? "auto  " : 
			(IS_GLOBAL(sid) ? "global" :"param "));
}

int
obj_debug(s,top)
char *s;
Object top;
{
	struct Object_info *obj;
	fprintf(stderr,"%s\n",s);
	for (obj = top; obj; obj=obj->next) {
		print_obj(obj);
	}
}

static void
reg_obj_print(obj)
Object obj;
{
	SX sid = obj->fe_numb;
	
	if (sid <= 0 || (SY_CLASS(sid) != SC_AUTO && SY_CLASS(sid) != SC_PARAM))
		return;
	DEBUG(aflags[ra].level&2,
		("%s obj =%d sid =%3d  name=%s weight=%9d  %s",
			COMMENTSTR,  obj->setid, sid, SY_NAME(sid),
			obj->weight,
			SY_CLASS(sid) == SC_AUTO ? "auto  " : "param "));
	printf("%s sid=%3d  name=%s  weight=%9d  %s",
			COMMENTSTR, sid, SY_NAME(sid),
			obj->weight,
			SY_CLASS(sid) == SC_AUTO ? "auto  " : "param ");
	if (SY_REGNO(sid) == SY_NOREG) {
		DEBUG(aflags[ra].level&2, 
			("%d(%%%d)\n",SY_OFFSET(sid), DB_FRAMEPTR)) ;
	    	printf("%d(%%%d)\n",SY_OFFSET(sid), DB_FRAMEPTR) ;
	}
	else {
	    DEBUG(aflags[ra].level&2, 
	        ("%s\n", rnames[SY_REGNO(sid)],
		obj->weight < REG_WT_THRESHOLD ? " +" : ""));
	    printf("%s\n", rnames[SY_REGNO(sid)],
		obj->weight < REG_WT_THRESHOLD ? " +" : "");
	}
}

void
amigo_regdebug()
/* Print objects lists. */
{
    struct Object_info *objct;
    
    DEBUG(aflags[ra].level&2,
	("%s Objects\n%s =======\n", COMMENTSTR, COMMENTSTR));
    printf("%s Objects\n%s =======\n", COMMENTSTR, COMMENTSTR);
    for (objct = get_object_last(); objct; objct = objct->next) 
	reg_obj_print(objct);
    objct = color_by_spill;
    if(objct) {
	DEBUG(aflags[ra].level&32 && aflags[ra].level&2,
		("%s \tmay_color_by_spill\n%s \t=======\n",
		COMMENTSTR, COMMENTSTR));
	printf("%s \tMay_color_by_spill\n%s =======\n", COMMENTSTR, COMMENTSTR);
	do {
		reg_obj_print(objct);
		objct = objct->next;
	} while(objct);
    }

    fflush(stdout);
    fflush(stderr);
    return;
}


static int
debug_cand_list(first_cand, last_cand)
struct Candidates *first_cand, *last_cand;
{
	struct Candidates *cand;
	fprintf(stderr, "Candidate list\n");
	for (cand=first_cand; cand <= last_cand;  ++cand) {
		fprintf(stderr,"\t");
		print_obj(cand->object);
		fprintf(stderr,"\tcand weight: %d neighbors: %d\n\n",cand->weight,
			bv_set_card(cand->object->set_interferes));
	}
}

static
spill_debug(str,victim,obj,scope)
char *str;
Object victim;
Object obj;
Scope scope;
{
	DEBUG(aflags[ra].level&1, (
		"%s victim %d %s(sid %d) scope(%d) around \
scope(%d) of object %d %s(sid %d)\n",
		str,
		victim->setid,
		OBJ_TO_NAME(victim), victim->fe_numb,
		debug_scope_id(victim->scope),
		debug_scope_id(scope),
		obj->setid,
		OBJ_TO_NAME(obj), obj->fe_numb)
	);
}
#endif
