#ident	"@(#)amigo:common/l_unroll.c	1.53"
#include "amigo.h"
#include "l_unroll.h"
#include "str_red.h"
#include <memory.h>
#include "costing.h"
#include <unistd.h>
#include <pfmt.h>
#include "lang.h"

#define MIN_ITERATIONS 1
#define LU_FLAGS(loop) LOOP_FLAGS(loop)

#define IS_INVARIANT(node,loop)  (OPTABLE(node) && \
        !(bv_belongs(node->opt->setid, loop->expr_kills)))
  
#define IS_INDVAR(node,loop)  (OPTABLE(node)  && node->opt->object && \
	bv_belongs(node->opt->object->setid, loop->ind_vars))
  
#define IS_ICON(node) ((node)->op == ICON && (node)->sid == ND_NOSYMBOL)

#define COUNT(X,Y,Z) ( (Z)> 0 ? (((X) < (Y)) ?  ((Y)-(X))/(Z) + 1 : 0 ) : 0 )

typedef enum { UNKNOWN, NOT_LE, NOT_LT, NOT_GE, NOT_GT, NOT_NE, NOT_NAME} Branch_type;
typedef enum { start, body, cond, end } Loop_directive;

typedef struct name_map_elem *Name_list;

struct name_map_elem {
	Expr name;
	ND1 *mapped;
	long update;
	Name_list next;
	Boolean is_counter;
};

static Name_list name_map;

#ifndef NODBG
static char *pattern_st[] = {
	"UNKNOWN", "NOT_LE","NOT_LT", "NOT_GE", "NOT_GT", "NOT_NE", "NOT_NAME"
};
static void debug_lu_data();
static void debug_name_map();
static void print_lu_flags();
static int DB_CGQ_INDEX_NUM();
#endif

static Branch_type branch_op_type;
static int tree_count;

static Cgq_index PROTO(gen_spill,(Cgq_index, int, int));
static void PROTO(get_test_type,(Loop));
static void PROTO(get_initial_type,(Loop));
static long  PROTO(iterations,(Loop));
static int PROTO(cost_trees,(Loop));

static Arena table_arena;

	/* Move this decl into amigo.h */
extern Block_list init_block_list();

static Block PROTO(map_block,(Block));
static void PROTO(analize_sr_temps, (Loop));
static Boolean PROTO(block_map_alloc, (Loop, Arena));
static ND1 *PROTO(build_increment_node, (ND1 *, long, int op));
static Boolean PROTO(alloc_tables, (Loop, Arena));
static Boolean PROTO(complete_unroll,(Loop));
typedef enum { Middle_copy, First_copy, Last_copy } Copy;
static Cgq_index PROTO(copy_blocks, (Loop, Copy copy_flag, Cgq_index));
static Cgq_index PROTO(copy_loop_body, (Loop, int, Copy, Cgq_index,
	Cgq_index, Cgq_index));
static Cgq_index PROTO(copy_map_cgq,(cgq_t *,Copy,Cgq_index,Block));
static void PROTO(check_counter_update, (Loop));
static int PROTO(find_ind_init, (ND1 *node, Loop loop));
static Boolean PROTO(find_loop_label_index,(Loop));
static Boolean PROTO(fix_counter_update,(Cgq_index, Loop_unroll_info));
static long get_counter_val();
static void reset_counter_val();
static ND1 *PROTO(gen_cbranch,(ND1 *left, int label));
static ND1 *PROTO(gen_compare,(ND1 *left, int op, ND1 *right));
static Cgq_index PROTO(gen_os_loop,(Cgq_index place, int loop_code, int cgq_flags));
static ND1 *PROTO(gen_temp_init,(Loop_unroll_info, TEMP_SCOPE_ID));
static Boolean PROTO(is_loop_structure_index,(Cgq_index, Loop));
static Boolean PROTO(is_single_exit, (Loop));
static void PROTO(label_table_init, (Boolean, Loop));
static void PROTO(loop_type,(Loop));
static int PROTO(map_label,(int));
static void PROTO(name_map_init, (Loop, int, Copy));
static Boolean PROTO(partial_unroll,(Loop));
static Boolean PROTO(partial_const_unroll,(Loop));
static void PROTO(remove_loop_structure,(Loop));
static ND1 * PROTO(t1_copy,(ND1 *));
static ND1 * PROTO(t1_copy_and_map,(Boolean *, ND1 *));
static ND2 * PROTO(t2_copy,(ND2 *));

static min_label, max_label;

Boolean
loop_unrolling(first_loop)
Loop first_loop;
{
	Loop loop;
	int did_something = 0;
	
	for(loop = first_loop; loop; loop = loop->next) {

		int unrolled;
		unrolled = 0;
		DEBUG_COND(! aflags[sr].xflag, find_ind_var_defs(loop));

		if(loop->child) continue;

		loop_type(loop);

		loop->loop_unroll->body_begin = loop->header->next;
		loop->loop_unroll->body_end = loop->end;

		DEBUG(aflags[lu].level&2 && SR_TEMP_LIST(loop),
				("Sr candidates loop(%d):\n", loop->id));
		DEBUG_COND(aflags[lu].level&2,
			debug_str_cands(SR_TEMP_LIST(loop)));

START_OPT
		if( (LU_FLAGS(loop) & LIMIT_MASK ) == INVAR_LIMITS ) {
			DEBUG_COND(aflags[lu].level&4, debug_lu_data(loop));
			table_arena = arena_init();
			analize_sr_temps(loop);
			unrolled = partial_unroll(loop);
			DEBUG(unrolled && aflags[lu].level,
				("Unrolled invariant limits loop(%d)\n", loop->id));
			CGQ_ELEM_OF_INDEX(loop->loop_unroll->loop_start)->cgq_flags |= CGQ_UNROLLED;
			arena_term(table_arena);
		}
		else if( (LU_FLAGS(loop) & LIMIT_MASK &~CONST_LIMITS) == INVAR_LIMITS ) {
			DEBUG_COND(aflags[lu].level&4, debug_lu_data(loop));
			table_arena = arena_init();
			analize_sr_temps(loop);
			unrolled = partial_const_unroll(loop);
			DEBUG(unrolled && aflags[lu].level,
				("Partially unrolled const limits loop (%d)\n", loop->id));
				/*
				** Leave a mark in the cgq so that the next
				** time we build loops, we will know this
				** is an amigo created loop.
				*/
			CGQ_ELEM_OF_INDEX(loop->loop_unroll->loop_start)->cgq_flags |= CGQ_UNROLLED;
			arena_term(table_arena);
		}
		else if((LU_FLAGS(loop) & LIMIT_MASK) == CONST_LIMITS ) {
			table_arena = arena_init();
			analize_sr_temps(loop);
			DEBUG_COND(aflags[lu].level&4, debug_lu_data(loop));
			unrolled = complete_unroll(loop);
			DEBUG(unrolled && (aflags[lu].level), ("Unrolled fixed loop(%d) %d times\n",
				loop->id, ITERATIONS(loop)));
			arena_term(table_arena);
		}
#ifndef NODBG
		else if (aflags[lu].level&2) {
			fprintf(stderr, "Loop(%d) not unrollable\n", loop->id);
			print_lu_flags(loop);
		}
#endif
		did_something += unrolled;
STOP_OPT
	}

	return did_something;
}

#ifndef CLUSTER_SIZE
#define CLUSTER_SIZE 4
#endif

static Cgq_index
bump_counters(insert_index, multiple)
Cgq_index insert_index;
int multiple;
{
	Name_list nl;
	for(nl = name_map; nl; nl = nl->next) {
		insert_index =
			insert_nd1(insert_index,
			build_increment_node(
				expr_to_nd1(nl->name),
				multiple * nl->update,
				ASG PLUS),
			0);
	}
	return insert_index;
}

static long
begin_limit(ND1 *p)
{
	long v;

	if (p == ND1NIL || !IS_ICON(p))
		Amigo_fatal(gettxt(":0", "bad begin_limit in l_unroll"));
	(void)num_toslong(&p->c.ival, &v);
	return v;
}

static long
end_limit(ND1 *p)
{
	long v;

	if (p == ND1NIL)
		return 0;
	if (!IS_ICON(p))
		Amigo_fatal(gettxt(":0", "bad end_limit in l_unroll"));
	(void)num_toslong(&p->c.ival, &v);
	return v;
}

	/*
	** Convert a loop with invariant limits to a new loop
	** which executes the old loop in clusters of ( for now
	** 4 old loop bodies ) straight line code.  The loop
	** has invariant limits and a constant loop increment.
	**
	** The transformation is as follows:
	**
	**	do
	**		body(i); i++;
	**	while( i <= end );
	**	
	**	===>
	**
	**	temp = end - 3;
	**	body(i); i++;
	**	while(i < = temp) {
	**		body(i); i++;
	**		body(i); i++;
	**		body(i); i++;
	**		body(i); i++;
	**	}	
	**
	**	while(i <= end) {
	**		body(i); i++;
	**	}
	**
	**	While loops are treated as guarded do loops.
	**	
	**	Note: in the case where we know where
	**	the counter is bumped and where associated
	**	strength reduction temps are bumped,
	**	we eliminate the increments and replace
	**	uses of i and associated temps by i, i+1,
	**	etc...  The current algorithm could be
	**	improved to replace ALL increments in the
	**	prolog and body of the loop.  See further
	**	comments below.
	*/

static Boolean
partial_unroll(loop)
Loop loop;
{
	Cgq_index insert_index, body_from_index, body_to_index;
	int label_1, label_2; /* ICON for CBRANCH targets */
	ND1 *new_temp;
	int relop1, relop2, spill_num;
	int i;
	extern get_new_spill_num();

	label_1 = new_label();
	label_2 = new_label();

	if(!alloc_tables(loop,table_arena))
		return false;

	spill_num = get_new_spill_num();

	body_from_index = loop->loop_unroll->body_begin->first;
	body_to_index = CGQ_PREV_INDEX(CGQ_ELEM_OF_INDEX(loop->loop_unroll->loop_cond));
        DEBUG(aflags[lu].level&4,("\tbody_from_index:%d body_to_index:%d\n",
		DB_CGQ_INDEX_NUM(body_from_index), DB_CGQ_INDEX_NUM(body_to_index)));
        DEBUG(aflags[lu].level&4,("\tBRANCH_OP_TYPE:%s INCREMENT:%d\n",
		pattern_st[branch_op_type], INCREMENT(loop)));

	switch(branch_op_type) {
		case NOT_LE: relop1 = LE; relop2 = GT; break;
		case NOT_LT: relop1 = LT; relop2 = GE; break;
		case NOT_GE: relop1 = GE; relop2 = LT; break;
		case NOT_NAME:
		case NOT_NE:
			if(INCREMENT(loop) > 0) {
						/*
						** must be counting up to 0
						*/
				relop1 = LT; relop2 = GE; break;
			}
			else {
				relop1 = GT; relop2 = LE; break;
			}
			break;
		case NOT_GT: relop1 = GT; relop2 = LE; break;
		default:
			Amigo_fatal(gettxt(":0","loop unroller confused by relop"));
	}

		/* Start insertions at END of loop->header */
	insert_index = 
		CGQ_PREV_INDEX(CGQ_ELEM_OF_INDEX(body_from_index));
			/* WHY NOT loop->header->last?? */
        DEBUG(aflags[lu].level&4,("\tstart insertions at:%d\n",
		DB_CGQ_INDEX_NUM(insert_index)));


	insert_index = copy_loop_body(loop, 0, First_copy, insert_index, body_from_index, body_to_index);

		/*
		** If we handle counter updates ourselves, we need one
		** here.  We could eliminate this code
		** by adjusting all uses of the counter and
		** other temps by appropriate constants in
		** the various loop copies.
		*/
	if( !(LU_FLAGS(loop) & LEAVE_COUNTER_INCR) )
		insert_index = bump_counters(insert_index, 1);

		/* Generate assignment temp = end -(+) incr * 3 */
	insert_index = 
		insert_nd1(insert_index,
			new_temp = gen_temp_init(loop->loop_unroll, loop->scope),
			0);

	insert_index = gen_os_loop(insert_index, OI_LSTART, CGQ_UNROLLED);

	new_temp = new_temp->left; /* The new temp is lhs of assign */
	
	insert_index =
		insert_nd1(
			insert_index,
			gen_cbranch(
				gen_compare(
					expr_to_nd1(loop->loop_unroll->loop_counter),
					relop1,
					t1_copy(new_temp)),
				label_2),
			0);
			

	insert_index = gen_spill(insert_index, CGQ_START_SPILL, spill_num);

	insert_index = gen_os_loop(insert_index, OI_LBODY, CGQ_UNROLLED);

	insert_index =
		insert_label(insert_index, label_1);

	for(i=1; i<=CLUSTER_SIZE; i++)
		insert_index = copy_loop_body(loop,i-1, Middle_copy, insert_index, body_from_index, body_to_index);

		/*
		** Increment the counter and other induction variables
		** all at once if it is not bumped in each copy of the loop.
		*/
	if( !(LU_FLAGS(loop) & LEAVE_COUNTER_INCR) )
		insert_index = bump_counters(insert_index, CLUSTER_SIZE);

	insert_index = gen_os_loop(insert_index, OI_LCOND, CGQ_UNROLLED);

	insert_index =
		insert_nd1(
			insert_index,
			gen_cbranch(
				gen_compare(
					expr_to_nd1(loop->loop_unroll->loop_counter),
					relop2,
					t1_copy(new_temp)),
				label_1),
			0);
			

	insert_index = gen_os_loop(insert_index, OI_LEND, CGQ_UNROLLED);

	insert_index = gen_spill(insert_index, CGQ_END_SPILL, spill_num);

	insert_index =
		insert_label(insert_index, label_2);

		/*
		** Generate a label for the end of loop -- it is not
		** present for do loops.
		*/
	label_2 = new_label();

	(void)insert_label(loop->loop_unroll->loop_end, label_2);

	{
	ND1 *node;

		/*
		** If the loop test is "!var" there is no
		** ICON in the CBRANCH, so we build one.
		*/
	if(branch_op_type == NOT_NAME) {
		node = tr_smicon(0L);	
		node->type = loop->loop_unroll->loop_counter->node.type;
	}	
	else
		node = t1_copy(loop->loop_unroll->end_limit);

		/* Guard for the tail */
	insert_index =
		insert_nd1(
			insert_index,
			gen_cbranch(
				gen_compare(
					expr_to_nd1(loop->loop_unroll->loop_counter),
					relop1, node),
				label_2),
			0);
	}
	{
		/*
		** Remove remaining loop structure from the tail.
		*/
	Loop_unroll_info l_u;
	l_u = loop->loop_unroll;
	amigo_delete(l_u->loop_start_block, l_u->loop_start);
	amigo_delete(loop->header, l_u->loop_body);
	amigo_delete(l_u->loop_cond_block, l_u->loop_cond);
	amigo_delete(l_u->loop_end_block, l_u->loop_end);
	}
	return true;
}

	/*
	** Convert a loop with constant limits to a new loop
	** which executes the old loop in clusters of ( for now
	** 4 old loop bodies ) straight line code.  The loop
	** has constant limits and a constant loop increment.
	**
	** The transformation is as follows:
	**
	**	for(i=start; i <= end; i++)
	**		body(i);
	**
	** gets transformed to
	**
	**	i=start;
	** #if x != 0
	**	body(start);
	**	body(start + 1);
	**	...
	**	body(start + x - 1);
	**	i += x;
	** #endif
	**	do {
	**		body(i);
	**		body(i+1);
	**		body(i+2);
	**		body(i+3);
	**		i += 4;
	**	} while(i<=end);
	**
	** Where x = iterations % 4, is computed at compile time.
	*/

static Boolean
partial_const_unroll(loop)
Loop loop;
{
	Cgq_index insert_index, body_from_index, body_to_index;
	int label_1; /* ICON for CBRANCH targets */
	int relop2, spill_num;
	int i;
	extern get_new_spill_num();
	int tail_count;

	label_1 = new_label();

	if(!alloc_tables(loop,table_arena))
		return false;

	spill_num = get_new_spill_num();

	body_from_index = loop->loop_unroll->body_begin->first;
	body_to_index = CGQ_PREV_INDEX(CGQ_ELEM_OF_INDEX(loop->loop_unroll->loop_cond));
        DEBUG(aflags[lu].level&4,("\tbody_from_index:%d body_to_index:%d\n",
		DB_CGQ_INDEX_NUM(body_from_index), DB_CGQ_INDEX_NUM(body_to_index)));
        DEBUG(aflags[lu].level&4,("\tBRANCH_OP_TYPE:%s INCREMENT:%d\n",
		pattern_st[branch_op_type], INCREMENT(loop)));

	DEBUG(aflags[lu].level&4,("begin_limit(%d):\n", BEGIN_LIMIT(loop)));
	DEBUG(aflags[lu].level&4,("end_limit(%d):\n", END_LIMIT(loop)));

	switch(branch_op_type) {
		case NOT_LE: relop2 = GT; break;
		case NOT_LT: relop2 = GE; break;
		case NOT_GE: relop2 = LT; break;
		case NOT_NAME:
		case NOT_NE:
			if(INCREMENT(loop) > 0) {
						/*
						** must be counting up to 0
						*/
				relop2 = GE; break;
			}
			else {
				relop2 = LE; break;
			}
			break;
		case NOT_GT: relop2 = LE; break;
		default:
			Amigo_fatal(gettxt(":0","loop unroller confused by relop"));
	}

		/* Start insertions at END of loop->header */
	insert_index = 
		CGQ_PREV_INDEX(CGQ_ELEM_OF_INDEX(body_from_index));
			/* WHY NOT loop->header->last?? */
        DEBUG(aflags[lu].level&4,("\tstart insertions at:%d\n",
		DB_CGQ_INDEX_NUM(insert_index)));

	tail_count = ITERATIONS(loop) % CLUSTER_SIZE;

		/*
		** Generate code that compensates for the "tail"
		*/
	for(i=1; i<=tail_count; i++) {
		insert_index = copy_loop_body(loop,i-1,
						(i == 1?First_copy:Middle_copy),
						insert_index,
						body_from_index,
						body_to_index);
	}

		/*
		** Increment the counter all at once if it is not
		** bumped in each copy of the loop.
		*/
	if( tail_count && !(LU_FLAGS(loop) & LEAVE_COUNTER_INCR) ) {
		Name_list nl;
		for(nl = name_map; nl; nl = nl->next) {
			insert_index =
				insert_nd1(insert_index,
				build_increment_node(
					expr_to_nd1(nl->name),
					tail_count * nl->update,
					ASG PLUS),
				0);

		}
	}

	insert_index = gen_os_loop(insert_index, OI_LSTART, CGQ_UNROLLED);

	insert_index = gen_spill(insert_index, CGQ_START_SPILL, spill_num);

	insert_index = gen_os_loop(insert_index, OI_LBODY, CGQ_UNROLLED);

		/*
		** Generate the top of the cluster.
		*/
	insert_index =
		insert_label(insert_index, label_1);

	for(i=1; i<=CLUSTER_SIZE; i++) {
		Copy copy_flag;
		if(i == CLUSTER_SIZE)
			copy_flag = Last_copy;
		else if(i == 1 && tail_count == 0)
			copy_flag = First_copy;
		else
			copy_flag = Middle_copy;
		insert_index = copy_loop_body(loop,i-1,
						copy_flag,
						insert_index,
						body_from_index,
						body_to_index);
	}

		/*
		** Increment the counter and associated induction variables
		** all at once if it is not bumped in each copy of the loop.
		*/
	if( !(LU_FLAGS(loop) & LEAVE_COUNTER_INCR) ) {
		Name_list nl;
		for(nl = name_map; nl; nl = nl->next) {
			insert_index =
				insert_nd1(insert_index,
				build_increment_node(
					expr_to_nd1(nl->name),
					CLUSTER_SIZE * nl->update,
					ASG PLUS),
				0);

		}
	}

	insert_index = gen_os_loop(insert_index, OI_LCOND, CGQ_UNROLLED);

	{
	ND1 *node;

	if(branch_op_type == NOT_NAME) {
		node = tr_smicon(0L);	
		node->type = loop->loop_unroll->loop_counter->node.type;
	}	
	else
		node = t1_copy(loop->loop_unroll->end_limit);


	insert_index =
		insert_nd1(
			insert_index,
			gen_cbranch(
				gen_compare(
					expr_to_nd1(loop->loop_unroll->loop_counter),
					relop2,
					node),
				label_1),
			0);
	}

	insert_index = gen_os_loop(insert_index, OI_LEND, CGQ_UNROLLED);

	insert_index = gen_spill(insert_index, CGQ_END_SPILL, spill_num);

		/*
		** Remove remaining loop structure from the tail.
		*/
	remove_loop_structure(loop);
	return true;
}

	/*
	** Build .temp = ( END -(+) ICON ) , where
	** ICON is -(+)(CLUSTER_SIZE-1)*loop_increment.
	**
	** Here's the tree:
	**
	**	      =
	**	     / \
	**	    /   \
	**       .temp	 - 
	**		/ \
	**	       /   \
	**	     END  ICON 
	*/

static ND1 *
gen_temp_init(l_unroll,scope)
Loop_unroll l_unroll;
TEMP_SCOPE_ID scope;
{
	ND1 *t1, *t2;
	T1WORD type;
	int sign = -1; /* FOR NOW */
	type = l_unroll->loop_counter->node.type;

	assert(TY_ISSCALAR(type));

	t1 = tr_smicon((sign)*(CLUSTER_SIZE-1) * (l_unroll->increment));

	if(branch_op_type != NOT_NAME) {
		/*
		** Build an ICON node on the right with value
		** [-](CLUSTER_SIZE - 1) * loop_increment. 
		*/	
		t2 = t1_copy(l_unroll->end_limit);
		t1 = tr_build(PLUS, t2, t1);
	}

	t1 = op_amigo_optim(t1);
	new_expr(t1, 0);
	t2 = make_temp(t1->opt, scope);
			/*
			** We have to fiddle the type:
			** for example, t1 might be const.
			*/
	t2->type = type;
	t1 = tr_build(ASSIGN,t2,t1);
	t1 = op_amigo_optim(t1);
	new_expr(t1, 0);
	DEBUG_COND(aflags[lu].level&4,dprint1(t1));
	return t1;
}

	/*
	** Return a new ND1 * of the form
	**		op
	**		/\
	**	       /  \
	**	     left right
	**
	** Caller allocates left and right.
	*/
static ND1 *
gen_compare(left, op, right)
ND1 *left, *right;
int op;
{
	ND1 *node;
	node = t1_copy(left);
	node->left = left;
	node->op = op;
	if(right) {
		node->right = right;
	}
	return node;
}

	/* move next two to blocks.c or util.c */
int
new_label()
{
	int label = getlab();
	update_min_label(label);
	update_max_label(label);
	return label;
}

static Cgq_index
copy_loop_body(loop, i, copy_flag, insert_index, from_index, to_index)
Loop loop;
int i;
Copy copy_flag;
Cgq_index insert_index, from_index, to_index;
{
	Boolean leave_counter_incr;
	leave_counter_incr = (LU_FLAGS(loop) & LEAVE_COUNTER_INCR) != 0;

	name_map_init(loop, i, copy_flag);

	label_table_init(copy_flag == First_copy, loop);

	CGQ_FOR_ALL_BETWEEN(elem, index, from_index, to_index)
		if(leave_counter_incr && (index == loop->loop_unroll->loop_increment_index) || ! is_loop_structure_index(index, loop)) {

		DEBUG(aflags[lu].level&256, (
			"copy_loop_body index(%d) to insert_index(%d)\n",
			DB_CGQ_INDEX_NUM(index),
			DB_CGQ_INDEX_NUM(insert_index)));

			insert_index = copy_map_cgq(elem, copy_flag, insert_index, 0);
			
		}
	CGQ_END_FOR_ALL_BETWEEN
	
	return insert_index;
}

static Cgq_index
gen_spill(place, code, spill_num)
Cgq_index place;
int code; /* CGQ_START_SPILL or CGQ_END_SPILL */
int spill_num;
{
	cgq_t *elem;
	elem = cg_q_insert(place);
	elem->cgq_op = code;
	elem->cgq_arg.cgq_int = spill_num;
	return CGQ_INDEX_OF_ELEM(elem);
}

static Cgq_index
gen_os_loop(place, loop_code, flag)
Cgq_index place;
int loop_code; /* OI_LSTART, etc... See manifest.h */
int flag;
{
	cgq_t *elem;
	elem = cg_q_insert(place);
	elem->cgq_op = CGQ_CALL_INT;
	elem->cgq_func = os_loop;
	elem->cgq_arg.cgq_int = loop_code;
	elem->cgq_flags = flag;
	return CGQ_INDEX_OF_ELEM(elem);
}

static ND1 *
gen_cbranch(left, label)
ND1 *left;
int label;
{
	ND1 *node = tr_cbranch(left, label);

	new_expr(node, 0);
	update_branch_count();
	return node;
}

	/*
	** Convert a loop with constant limits to fully unrolled
	** straight line code.
	*/
static Boolean
complete_unroll(loop)
Loop loop;
{
	int i;
	Cgq_index insert_index;

	insert_index =
		CGQ_PREV_INDEX(CGQ_ELEM_OF_INDEX(loop->loop_unroll->body_begin->first));
	if(!alloc_tables(loop,table_arena))
		return false;

	for(i = 1; i <= ITERATIONS(loop); i++) {
		Copy copy_flag;

		if(i == 1)
			copy_flag = First_copy;
		else if(i == ITERATIONS(loop))
			copy_flag = Last_copy;
		else
			copy_flag = Middle_copy;

		name_map_init(loop, i-1, copy_flag);

		insert_index = copy_blocks(loop, copy_flag, insert_index);
	}

		/*
		** Note: we assume there is no reason to update any mapped
		** name other than the counter ( i.e., any str red temps )
		** because presumably they are dead.
		*/
	reset_counter_val(loop, Last_copy);
	insert_index = insert_nd1(
				insert_index,
				build_increment_node(
					expr_to_nd1(loop->loop_unroll->loop_counter),
					get_counter_val(),
					ASSIGN
				),
				0);

	remove_loop_structure(loop);
	return true;
}

static void PROTO(delete_branch,(Loop));

		/*
		** Find the cgq_index of the target label of
		** the loop test ( in the source loop ).
		*/
static Boolean
find_loop_label_index(loop)
Loop loop;
{
	Cgq_index label_index;

	label_index = CGQ_NULL_INDEX;
		/*
		** Check that the first interesting tree
		** in the loop body is what we think is the top
		** of loop label.
		*/
	CGQ_FOR_ALL_BETWEEN(elem, index,
		loop->header->next->first, loop->header->next->last)

		if(HAS_EXECUTABLE(elem)) {
			DEBUG(aflags[lu].level&128,("Check index %d cgq_op %d %d %d\n",
				CGQ_INDEX_NUM(index),
				elem->cgq_op,
				CGQ_EXPR_ND1,
				CGQ_EXPR_ND2));
			DEBUG_COND(aflags[lu].level&128 && elem->cgq_op == CGQ_EXPR_ND2,e22print(elem->cgq_arg.cgq_nd2,"CHECK:"));
			DEBUG_COND(aflags[lu].level&128 && elem->cgq_op == CGQ_EXPR_ND1,tr_e1print(elem->cgq_arg.cgq_nd1,"CHECK:"));
			if(elem->cgq_op == CGQ_EXPR_ND2
				&& elem->cgq_arg.cgq_nd2->op == LABELOP
				&& elem->cgq_arg.cgq_nd2->c.label
					== loop->loop_unroll->label)
				label_index = index;
			CGQ_FOR_ALL_BETWEEN_BREAK;
		}

	CGQ_END_FOR_ALL_BETWEEN


	if(label_index == CGQ_NULL_INDEX)
		return false;
	else {
		loop->loop_unroll->loop_label_index = label_index;
		return true;
	}
}

static Boolean
is_loop_structure_index(index, loop)
Cgq_index index;
Loop loop;
{
	Loop_unroll_info loop_unroll;
	loop_unroll = loop->loop_unroll;
	if ( index == loop_unroll->loop_increment_index
		|| index == loop_unroll->loop_body
		|| index == loop_unroll->loop_cbranch_index
		|| index == loop_unroll->loop_cond
		|| index == loop_unroll->loop_label_index )
		return true;
	else {
		Cand_descr list;
		for(list = SR_TEMP_LIST(loop); list; list = SR_NEXT_CAND(list)){
			if(index == SR_UPDATE_INDEX(list)) {
				return(true);
			}
		}
	}
	return false;
}

static void
remove_loop_structure(loop)
Loop loop;
{
	Loop_unroll_info l_u;

	l_u = loop->loop_unroll;
	DEBUG(aflags[lu].level&512,( "To delete? label_index: %d loop_increment_index: %d\n",
	CGQ_INDEX_NUM(l_u->loop_label_index), CGQ_INDEX_NUM(l_u->loop_increment_index)));

		/*
		** delete os_loop_calls
		*/
	amigo_delete(l_u->loop_start_block, l_u->loop_start);
	amigo_delete(loop->header, l_u->loop_body);
	amigo_delete(l_u->loop_cond_block, l_u->loop_cond);
	amigo_delete(l_u->loop_end_block, l_u->loop_end);
	delete_branch(loop);
	DEBUG(aflags[lu].level&512,( "cut from %d to %d\n",
		CGQ_INDEX_NUM (l_u->body_begin->first),
		CGQ_INDEX_NUM ( l_u->loop_test_block->last)));
	cg_q_cut(l_u->body_begin->first,
		l_u->loop_test_block->last);
		
}

	/*
	** Define maps for  all labels in the body.
	** Because the number of items to be mapped is small, we could
	** probably just use linked lists of map pairs.  But the
	** code here uses an existing array layout ( from blocks.c ).
	*/
static void
label_table_init(first_call, loop)
Boolean first_call;
Loop loop;
{
	Block block;
	int i;

		/*
		** Initially set limits for label search.  If we
		** see any labels, we will get min_label <= max_label.
		*/

	if(first_call) {
		min_label = get_max_label() + 1;
		max_label = get_min_label();

		label_table_zero();

		for(block = loop->loop_unroll->body_begin;
			block != loop->loop_unroll->body_end;
			block = block->next) {

		CGQ_FOR_ALL_BETWEEN(elem, index, block->first, block->last)
				/* Check for label, auto, parm or amigo temp */
			if(elem->cgq_op == CGQ_EXPR_ND2
				&& elem->cgq_arg.cgq_nd2->op == LABELOP) {

				int label = elem->cgq_arg.cgq_nd2->c.label;
				struct Label_descriptor *l_d;
				l_d = get_label_descriptor(label);
				l_d->map_label = label;
					/* Next needed ?? */
				l_d->block = block;
				if(label < min_label)
					min_label = label;
				if(label > max_label)
					max_label = label;
			}
		CGQ_END_FOR_ALL_BETWEEN

		}	
	}

	DEBUG_COND(aflags[lu].level&64,
		pr_label_table(min_label, max_label));

	for(i = min_label; i <= max_label; i++) {
		struct Label_descriptor *l_d;
		l_d = get_label_descriptor(i);
		if(l_d->map_label) 
			l_d->map_label = new_label(); 
	}

	DEBUG_COND(aflags[lu].level&8,
		pr_label_table(min_label, max_label));
}

static long counter_val;

static long
get_counter_val()
{
	return counter_val;
}

static void
reset_counter_val(loop, copy_flag)
Loop loop;
Copy copy_flag;
{
	if(copy_flag == First_copy)
		counter_val = BEGIN_LIMIT(loop);
	else counter_val += INCREMENT(loop);
}

	/*
	** We use the following " roll your own " routine
	** rather than tr_build, because tr_build will
	** adjust the constant to account for pointer arithmetic.
	** We don't want this when adding update values to str_red
	** temps: it's already been done.
	*/
static ND1 *
build_increment_node(lhs, val, op)
ND1 *lhs;
long val;
int op;
{
	ND1 *node;
	node = t1alloc();
	node->right = tr_smicon(val);	
	node->op = op;
	node->type = lhs->type;
	node->left = lhs;
	node->flags = FF_SEFF;
	new_expr(node, 0);
	return node;
}

static void
name_map_init(loop, iteration, copy_flag)
Loop loop;
int iteration;
Copy copy_flag;
{
	Name_list nl;

	if(copy_flag == First_copy) {
		Name_list new;
		Cand_descr list;
		name_map = 0;

			/*
			** Don't deal with sr temps if the update
			** location is weird.  Don't map counter.
			*/
		if(LU_FLAGS(loop) & LEAVE_COUNTER_INCR) {
			SR_TEMP_LIST(loop) = 0;
		}
#ifndef NODBG
		if(junk)
			SR_TEMP_LIST(loop) = 0;
#endif

		for(list = SR_TEMP_LIST(loop); list; list = SR_NEXT_CAND(list)) {
			new = Arena_alloc(table_arena, 1, struct name_map_elem);
			new->name = sid_to_expr(SR_TEMP(list));
			new->mapped = 0;
			new->is_counter = false;
			new->update = SR_UPDATE(list);
			new->next = name_map;
			name_map = new;
		}

		DEBUG_COND(SR_TEMP_LIST(loop) && (aflags[lu].level&1),
			debug_str_cands(SR_TEMP_LIST(loop)));

		new = Arena_alloc(table_arena, 1, struct name_map_elem);
		new->name = loop->loop_unroll->loop_counter;
		new->update = INCREMENT(loop);
		new->next = name_map;
		new->is_counter = true;
		new->mapped = 0;
		name_map = new;
	}

	if(LU_FLAGS(loop) & LEAVE_COUNTER_INCR) { /* map counter to itself */
		name_map->mapped = expr_to_nd1(name_map->name);
		DEBUG_COND(aflags[lu].level&2, debug_name_map(loop));
		return;
	}

	for( nl = name_map; nl; nl = nl->next) {
		if( (LU_FLAGS(loop) & LIMIT_MASK &~CONST_LIMITS) == INVAR_LIMITS ) {
			nl->mapped = ( iteration ?
				build_increment_node(
					expr_to_nd1(nl->name),
					iteration*nl->update,
					PLUS):
				expr_to_nd1(nl->name));
		}
		else if( (LU_FLAGS(loop) & LIMIT_MASK ) == CONST_LIMITS ) {
			if(nl->is_counter) {
				reset_counter_val(loop, copy_flag);
				nl->mapped = tr_smicon(get_counter_val());
			}
			else  /* map a temp */
				nl->mapped = iteration ?
					build_increment_node(
						expr_to_nd1(nl->name),
						iteration*nl->update,
						PLUS):
					expr_to_nd1(nl->name);
		}
	}
	DEBUG_COND(aflags[lu].level&2, debug_name_map(loop));
}

#ifndef NODBG
static void
debug_name_map(loop)
Loop loop;
{
	Name_list nl;
	for( nl = name_map; nl; nl = nl->next) {
		fprintf(stderr,"=========\n");
		print_expr(nl->name);
		if(nl->mapped) {
			fprintf(stderr,"=========mapped to=========\n");
			tr_e1print(nl->mapped, "FOO");
		}
		else fprintf(stderr,"=========not mapped=========\n");
		fprintf(stderr,"Update(%d)\n", nl->update);
	}
	
}
#endif

static Boolean 
alloc_tables(loop, arena)
Loop loop;
Arena arena;
{
	label_table_alloc(arena);
	return(block_map_alloc(loop,arena));
}

typedef struct block_map_elem {
	Block old, new; /* may not need old */
} Block_map_elem;

static int block_map_size;
static Block_map_elem *block_map;
static lo_block, hi_block;	/*
				** array bounds
				*/

	/*
	** Build mapping of all blocks in the loop body.
	*/
static Boolean
block_map_alloc(loop, arena)
Loop loop;
Arena arena;
{
	int i;
	Block block, start_block, stop_block;

	start_block = loop->loop_unroll->body_begin;
	stop_block = loop->loop_unroll->body_end;

	lo_block = start_block->block_count; 
	hi_block = stop_block->block_count - 1; 
	block_map_size = hi_block - lo_block + 1;
	assert(block_map_size > 0 );
	block_map = Arena_alloc(arena, block_map_size, Block_map_elem);
	(void)memset((char *)block_map,0,sizeof(Block_map_elem)*block_map_size);
	for(block = start_block; block != stop_block;
		block = block->next) {
		i = block->block_count - lo_block;
		assert( 0 <= i && i < hi_block );
		block_map[i].old = block;
		block_map[i].new = 0;
	}
	if(! is_single_exit(loop) )
		return false;

	else
		return true;
}

#ifndef NODBG
static int
debug_block_map()
{
	int i;

	fprintf(stderr,"\t+=+=+=+=+=+=+%s+=+=+=+=+=+=+\n",
		"block map (index old new new_pred new_succ)");

	for(i=0; i < block_map_size; i++) {
		fprintf(stderr,"\t%d\t%d\t%d\t", i,
			block_map[i].old ? block_map[i].old->block_count : -1,
			block_map[i].new ? block_map[i].new->block_count : -1);
		
		if(block_map[i].new) {
			pr_block_list((block_map[i].new)->pred, "");
			pr_block_list((block_map[i].new)->succ, "\t");
		}
		fprintf(stderr,"\n");
	}
}

static void
debug_lu_data(loop)
Loop loop;
{
	Loop_unroll_info lu;
	lu = loop->loop_unroll;

	fprintf(stderr,
		"\tcbranch_index(i%d) label_index(i%d) loop_increment_index(i%d)\n",
		DB_CGQ_INDEX_NUM(lu->loop_cbranch_index),
		DB_CGQ_INDEX_NUM(lu->loop_label_index),
		DB_CGQ_INDEX_NUM(lu->loop_increment_index)
	);

	fprintf(stderr,
		"\tstart(i%d), cond(i%d), body(i%d) end(i%d)\n",
		DB_CGQ_INDEX_NUM(lu->loop_start),
		DB_CGQ_INDEX_NUM(lu->loop_cond),
		DB_CGQ_INDEX_NUM(lu->loop_body),
		DB_CGQ_INDEX_NUM(lu->loop_end)
	);

	fprintf(stderr,"\tstart_block(b%d), cond_block(b%d), end_block(b%d) \
test_block(b%d)\n",
		lu->loop_start_block->block_count,
		lu->loop_cond_block->block_count,
		lu->loop_end_block->block_count,
		lu->loop_test_block->block_count
	);

	fprintf(stderr,"\tbody_begin(%d), body_end(%d), header(%d)\n",
		lu->body_begin->block_count,
		lu->body_end->block_count,
		loop->header->block_count
	);
	fprintf(stderr,"\tincrement(%d)\n", lu->increment);
	fprintf(stderr,"\tloop counter:\n");
	print_expr(lu->loop_counter);
	
}

static void
print_lu_flags(loop)
Loop loop;
{
	int flags = LU_FLAGS(loop);
	DPRINTF("loop(%d) flags %s%s%s%s\n",
		loop->id,
		(flags&CHECK_LIMIT) ? "CHECK_LIMIT ":"",
		(flags&CONST_LIMITS) ? "CONST_LIMITS ":"",
		(flags&INVAR_LIMITS) ? "INVAR_LIMITS ":"",
		(flags&LEAVE_COUNTER_INCR) ? "LEAVE_COUNTER_INCR ":"",
		(flags == 0) ? "NO_UNROLL":"");
}

static int
DB_CGQ_INDEX_NUM(index)
Cgq_index index;
{
	int i;
	if(index == CGQ_NULL_INDEX)
		return -1;
	else
		return CGQ_INDEX_NUM(index);
}
#endif

static Block
map_block(block)
Block block;
{
	int block_num = block->block_count;
	if(block_num < lo_block || block_num > hi_block)
		return block;
	else
		return(block_map[block_num - lo_block].new);
}

	/* 
	** is_single_exit is called when the block map has first been
	** initialized.  At that time, all blocks in the loop body
	** are mapped to 0, all other blocks map to themselves ( as
	** usual ).
	** We check that every block in the body, except for the last
	** ( loop test ) block, has all its successors in the body.
	*/
static Boolean
is_single_exit(loop)
Loop loop;
{
	Block block;
	Block_list bl;
	Arena temp_arena = arena_init();
	Block_set loop_blocks = Block_set_alloc(temp_arena);
	bv_init(false, loop_blocks);
	for(bl = loop->blocks; bl; bl = bl->next) {
		bv_set_bit(bl->block->block_count, loop_blocks);
	}
	for(block = loop->loop_unroll->body_begin;
		block != loop->loop_unroll->loop_test_block; block = block->next) {
		Block_list bl;
		for(bl = block->succ; bl; bl = bl->next) {
			if(!bv_belongs(bl->block->block_count, loop_blocks) || bl->block == map_block(bl->block)) {
				DEBUG(aflags[lu].level&8,
				("Block %d has successor %d not in loop\n",
					block->block_count,
					bl->block->block_count));
				arena_term(temp_arena);
				return false;
			}
		}	
	}
	arena_term(temp_arena);
	return true;
}
	/*
	** Copy/edit the blocks of the loop body.
	** Replace every instance of the loop counter by its current
	** value.
	*/
static Cgq_index
copy_blocks(loop, copy_flag, insert_index)
Loop loop;
Copy copy_flag;
Cgq_index insert_index;
{
	Block block;
	Block start_block, stop_block;

	start_block = loop->loop_unroll->body_begin;
	stop_block = loop->loop_unroll->body_end;

	label_table_init(copy_flag == First_copy, loop);

	for(block = start_block; block != stop_block;
		block = block->next) {

		CGQ_FOR_ALL_BETWEEN(elem, index, block->first, block->last)

		if(! is_loop_structure_index(index, loop))
			insert_index = copy_map_cgq(elem, copy_flag,
				insert_index,0);

		CGQ_END_FOR_ALL_BETWEEN
	}
	return insert_index;
}

	/*
	** This horror inserts a new item into the cgq, mapping the
	** loop counter to counter_val.  If we are not in the first
	** copy of the loop we delete scope start constructs ( e.g.,
	** START_SCOPE.  If we are not in the last copy of the loop,
	** we delete scope end constructs ( e.g., END_SCOPE ).  This
	** broadens the scopes of variables declare in the loop, obviating
	** the need to map them to new variables ( which can cause the
	** number of expressions to explode ).
	*/
static Cgq_index
copy_map_cgq(source_elem,copy_flag,insert_index,new_block)
cgq_t *source_elem;
Copy copy_flag;
Cgq_index insert_index;
Block new_block;
{
	cgq_t *cgq_item;
	Boolean first_copy, last_copy;

	first_copy = (copy_flag == First_copy);
	last_copy = (copy_flag == Last_copy);
		
	cgq_item = cg_q_insert(insert_index);

	DEBUG(aflags[lu].level&8,
		("Copy old flow(%d) to new flow(%d) after flow(%d)\n",
		DB_CGQ_INDEX_NUM(CGQ_INDEX_OF_ELEM(source_elem)),
		DB_CGQ_INDEX_NUM(CGQ_INDEX_OF_ELEM(cgq_item)),
		DB_CGQ_INDEX_NUM(insert_index)));

	DEBUG_COND(aflags[lu].level&8,
		cgq_print_tree(source_elem,CGQ_INDEX_OF_ELEM(source_elem),3));
	cgq_item->cgq_op = source_elem->cgq_op;
	cgq_item->cgq_func = source_elem->cgq_func;
	cgq_item->cgq_ln = source_elem->cgq_ln; /* ?? PSP */
	cgq_item->cgq_dbln = source_elem->cgq_dbln; /* ?? PSP */
	cgq_item->cgq_scope_marker = source_elem->cgq_scope_marker; /* ?? PSP */
	cgq_item->cgq_arg = source_elem->cgq_arg; /* ?? PSP */

		/*
		** Copy the actual tree.  Attempt to minimize
		** the horrors of mixing pass1/pass2 structs.
		** For another approach, see routines in
		** expand.c!
		*/
	switch(cgq_item->cgq_op) {
	case CGQ_EXPR_ND1:
		{
		ND1 *node;
		Boolean changed = 0;
		node = t1_copy_and_map(&changed, source_elem->cgq_arg.cgq_nd1);

		if(changed) {
			node = amigo_op_optim(node);
				/*
				** op_amigo_optim() can change a CBRANCH
				** to a JUMP, so we have to make
				** the cgq_item sane.
				*/
			if(node->op == JUMP) {
				cgq_item->cgq_arg.cgq_nd2 = 
					(ND2 *)node;
				cgq_item->cgq_op = CGQ_EXPR_ND2;
					/* No call to new_expr */
				update_branch_count();
				break;
			}
		}

		cgq_item->cgq_arg.cgq_nd1 = node;
		if(cgq_item->cgq_arg.cgq_nd1->op != CBRANCH)
			new_expr(cgq_item->cgq_arg.cgq_nd1, new_block);
		else {
			update_branch_count();
			new_expr(cgq_item->cgq_arg.cgq_nd1->left,
				new_block);
		}
		break;
		}
	case CGQ_EXPR_ND2:

			/*
			** The code here for updating the branch
			** count mimics similar code in hash_exprs()	
			*/
		switch(cgq_item->cgq_arg.cgq_nd2->op) {
		case SWEND:
			{ ND2 *p = cgq_item->cgq_arg.cgq_nd2->left;

				/* Walk through its SWCASE list */
				while (p->op == SWCASE) {
					update_branch_count();
					p = p->left;
				}
			}
			if(cgq_item->cgq_arg.cgq_nd2->sid < 0)
				break;
			/* FALLTHROUGH */
		case JUMP:
			update_branch_count();
			break;
		}

		cgq_item->cgq_arg.cgq_nd2 =
			t2_copy(source_elem->cgq_arg.cgq_nd2);
		break;
	case CGQ_FIX_SWBEG:
		{
			/*
			** We need to copy the whole tree,
			** as cg will blow it away.
			*/
		ND2 *t2 = (ND2 *)talloc();
		*t2 = *(cgq_item->cgq_arg.cgq_nd2);
		cgq_item->cgq_arg.cgq_nd2 = t2;
		cgq_item->cgq_arg.cgq_nd2->left =
			(ND2 *)t1_copy_and_map(0, (ND1 *)(source_elem->cgq_arg.cgq_nd2->left));
		new_expr((ND1 *)(cgq_item->cgq_arg.cgq_nd2->left),
			new_block);
		break;
		}	
	case CGQ_CALL_SID:
		if( cgq_item->cgq_func == db_symbol && !first_copy) {
			cgq_item->cgq_flags |= CGQ_DELETE;
		}
		else if ((cgq_item->cgq_func == db_sy_clear ||
			cgq_item->cgq_func == os_symbol) && ! last_copy ){
			cgq_item->cgq_flags |= CGQ_DELETE;
		}
		break;
	case CGQ_START_SCOPE:
		if(!first_copy) cgq_item->cgq_flags |= CGQ_DELETE;
		break;
	case CGQ_START_SPILL:
		cgq_item->cgq_flags |= CGQ_DELETE;
		break;
	case CGQ_END_SCOPE:
		if(!last_copy) cgq_item->cgq_flags |= CGQ_DELETE;
		break;
	case CGQ_END_SPILL:
		cgq_item->cgq_flags |= CGQ_DELETE;
		break;
	}	

	return CGQ_INDEX_OF_ELEM(cgq_item);
}


	/*
	** Return a copy of tree in which occurrences of the loop_counter
	** have been replaced by counter_val. 
	*/
static ND1 *
t1_copy_and_map(changed, tree)
Boolean *changed;
ND1 *tree;
{
	ND1 *new;

	new = t1alloc();
	*new = *tree;

	switch(optype(tree->op)) {
	case BITYPE:
		new->left = t1_copy_and_map(changed, tree->left);
		new->right = t1_copy_and_map(changed, tree->right);
		break;
	case UTYPE:
		new->left = t1_copy_and_map(changed, tree->left);
		new->right = tree->right;
		if(tree->op == CBRANCH)
			new->c.label = map_label(new->c.label);
		break;
	case LTYPE:
		if(tree->op == NAME) {
			Name_list nl;
			for(nl = name_map; nl; nl = nl->next) {
				/*
				** Rather than matching on the exprs,
				** we have to match the object ids,
				** because integral promotions may have
				** changed the type of a NAME node,
				** hence the exprs will not match.
				** Likewise, we have to worry that
				** types may not be right in the mapped
				** tree.  ( This hack is a fix for a bug
				** exposed running -Xt. )
				*/
				if((tree->opt->flags & OBJECT) &&
					tree->opt->object->setid == nl->name->object->setid) {
					new = t1_copy(nl->mapped);
					new->type = tree->type;
					if(optype(new->op) == BITYPE) {
						new->right->type = tree->type;
						new->left->type = tree->type;
					}
					if(changed) *changed = 1;
					break;
				}
			}
		}
		break;
	default:
		Amigo_fatal(gettxt(":574","Bad op type in tree copy"));	
	}

	return(new);
}

static ND1 *
t1_copy(tree)
ND1 *tree;
{
	ND1 *new;

	new = t1alloc();
	*new = *tree;

	switch(optype(tree->op)) {
	case BITYPE:
		new->left = t1_copy(tree->left);
		new->right = t1_copy(tree->right);
		break;
	case UTYPE:
		new->left = t1_copy(tree->left);
		break;
	case LTYPE:
		break;
	default:
		Amigo_fatal(gettxt(":574","Bad op type in tree copy"));	
	}

	return(new);
}

static ND2 *
t2_copy(tree)
ND2 *tree;
{
	ND2 *new, *p;

	new = (ND2 *)tcopy(tree);

	switch(tree->op) {
	case LABELOP:
	case JUMP:
		new->c.label = map_label(new->c.label);	
		break;
	case SWEND:
		p = new->left;
		while (p->op == SWCASE) {
			p->sid = map_label(p->sid);
			p = p->left;
		}
		if (tree->sid >= 0)
			new->sid = map_label(new->sid);
		break;
	}
	return new;
}

static int
map_label(label)
int label;
{
	struct Label_descriptor *l_d;
	if(min_label > max_label) return label;
	l_d = get_label_descriptor(label);
	return l_d->map_label ? l_d->map_label : label;
}

	/*
	** Find the cg_q element which increments the counter in the loop
	** body.  The only complication is that this update may
	** be under a comma op ( because of an AMIGO optimization ).
	** To solve this, split the cg_q item apart while looking
	** for the update.  Update the loop info.
	** We might avoid all this by getting rid of all top level
	** comma ops in the cg_q.
	*/
static Boolean
fix_counter_update(index, l_u_i)
Cgq_index index;
Loop_unroll_info l_u_i;
{
	cgq_t *item;
	item = CGQ_ELEM_OF_INDEX(index);
	if(item->cgq_arg.cgq_nd1->op != COMOP) {
		if(item->cgq_arg.cgq_nd1->opt->object != 0
			&&
			item->cgq_arg.cgq_nd1->opt->object == l_u_i->loop_counter->object) {
			l_u_i->loop_increment_index = index;
			return true;
		}
		else {
			return false;
		}
	}
	else {	/* Split the comma into two cgq_items and look on
		** both sides.
		*/
		cgq_t *right;
			
		right = cg_q_insert(index);
		item = CGQ_ELEM_OF_INDEX(index);
		right->cgq_op = item->cgq_op;
		right->cgq_func = item->cgq_func;
		right->cgq_arg.cgq_nd1 = item->cgq_arg.cgq_nd1->right;
		item->cgq_arg.cgq_nd1 = item->cgq_arg.cgq_nd1->left;
		right->cgq_ln = item->cgq_ln;
		right->cgq_dbln = item->cgq_dbln;
		return
			fix_counter_update(index,l_u_i) ||
			fix_counter_update(CGQ_INDEX_OF_ELEM(right), l_u_i);
	}

}

static void
delete_branch(loop)
Loop loop;
{
	Cgq_index cbranch_index = loop->loop_unroll->loop_cbranch_index;
	Cgq_index prev_index;
	prev_index = CGQ_PREV_INDEX(CGQ_ELEM_OF_INDEX(cbranch_index));
	if(cbranch_index == loop->loop_unroll->loop_test_block->last)
		loop->loop_unroll->loop_test_block->last = prev_index;
	cg_q_delete(cbranch_index);
}

static void
loop_type(loop)
Loop loop;
{
	loop->loop_unroll->end_limit = 0;
	loop->loop_unroll->begin_limit = 0;

	DEBUG(aflags[lu].level&4,("\nLoop(%d) loop_cbranch_index(%d)\n",
		loop->id,
		DB_CGQ_INDEX_NUM(loop->loop_unroll->loop_cbranch_index) ));

	DEBUG(aflags[lu].level&4,("increment_index(%d) increment(%d)\n",
		DB_CGQ_INDEX_NUM(loop->loop_unroll->loop_increment_index),
		INCREMENT(loop)));

	/*
	**  If the index of the loop test was not set, then there
	**  was a problem finding the CBRANCH containing the loop test,
	**  so give up.  Similarly if we couldn't figure out the
	**  loop increment.
	*/

	if ( loop->loop_unroll->loop_cbranch_index == CGQ_NULL_INDEX ||
	     loop->loop_unroll->loop_increment_index == CGQ_NULL_INDEX || 
	     INCREMENT(loop) == 0 )
		return;

		/*
		** Following should be pulled out in separate costing
		** pass.
		*/
	if ( (tree_count = cost_trees(loop)) > lu_l_size_threshold)
	{
		DEBUG(aflags[lu].level&4,("\tnot unrollable: cost_trees (%d > %d):\n", tree_count, lu_l_size_threshold));
		LU_FLAGS(loop) = NO_UNROLL;
		return;
	}

	get_test_type(loop);

	DEBUG_COND(aflags[lu].level&4,
		pr_object(loop->loop_unroll->loop_counter->object));
	DEBUG(aflags[lu].level&4,("\n"));

	DEBUG(aflags[lu].level&4,("pattern(%s):\n", pattern_st[branch_op_type]));

	if(LU_FLAGS(loop) == CONST_LIMITS)
		get_initial_type(loop);

	if( LU_FLAGS(loop) == NO_UNROLL ) {
		
		DEBUG(aflags[lu].level&4,
			("not unrollable: returning after get_test_type/get_initial_type\n"));
		return;
	}

	if ( LU_FLAGS(loop) == CONST_LIMITS)
	{
		
		DEBUG(aflags[lu].level&4,("begin_limit(%d):\n", BEGIN_LIMIT(loop)));
		DEBUG(aflags[lu].level&4,("end_limit(%d):\n", END_LIMIT(loop)));

		ITERATIONS(loop) = iterations(loop);

		if (ITERATIONS(loop) > MIN_ITERATIONS)
		{
			
			DEBUG(aflags[lu].level&4,("ITERATIONS: %d\n",
				ITERATIONS(loop)));

			/* 
			** Determine if the amount of code expansion is
			** acceptable.  Should pull out into costing
			** code.
			*/

			if ( (double)tree_count * (double)(ITERATIONS(loop))  >= 
				lu_l_size_threshold * lu_threshold ) {
					/* Change for inv */

				DEBUG(aflags[lu].level&2,
				("\tchange %sconst_lim to invar_lim: \n\
\ttree_count(%d) * ITERATIONS(%d) >= lu_l_size_threshold(%d) * lu_threshold(%d)\n",
				(branch_op_type == NOT_NE) ? "NOT_NE ":"",
				tree_count,ITERATIONS(loop),lu_l_size_threshold,
				lu_threshold));

				LU_FLAGS(loop) |= INVAR_LIMITS;
			}
		}
		else {
			DEBUG(aflags[lu].level&4,
				("not unrollable: ITERATIONS(loop) <= MIN_ITERATIONS) ( %d <= %d )\n",
				ITERATIONS(loop), MIN_ITERATIONS));
			LU_FLAGS(loop) = NO_UNROLL;
		}
	}
	else if ( LU_FLAGS(loop) == INVAR_LIMITS ) {

		switch(branch_op_type) {
		case NOT_LE: 
		case NOT_LT:
			if(INCREMENT(loop) <= 0) {
				LU_FLAGS(loop) = NO_UNROLL;
				break;
			}
			break;
		case NOT_GE: 
		case NOT_GT: 
			if(INCREMENT(loop) >= 0) {
				LU_FLAGS(loop) = NO_UNROLL;
				break;
			}
			break;
		default:
			LU_FLAGS(loop) = NO_UNROLL;
		}
	}
	else
		LU_FLAGS(loop) = NO_UNROLL;

	if(LU_FLAGS(loop) == NO_UNROLL)
		return;

	if(! find_loop_label_index(loop) ) {
		DEBUG(aflags[lu].level&4,
			("can\'t find loop_label_index.\n"));
		LU_FLAGS(loop) = NO_UNROLL;
		return;
	}
	else if(! fix_counter_update(loop->loop_unroll->loop_increment_index,
		loop->loop_unroll)) {
		/* 
		** Change next to break, once everything works.
		*/
		DEBUG_COND(aflags[lu].level, debug_lu_data(loop));
#ifndef NODBG
		pfmt(stderr,MM_WARNING,":0: failed to find counter increment in loop\n");
#endif
		LU_FLAGS(loop) = NO_UNROLL;
		return;
	}
	check_counter_update(loop);
}

/*
** Based upon the begin_limit (the induction variable's initial value,
** which must be a constant), the end_limit (the upper bound of the loop,
** which also must be a constant), and the type of loop test, determine 
** the number of iterations this loop will make.
*/

static long
iterations(loop)
Loop loop;
{
	switch(branch_op_type) {

	case NOT_LE: /* Need incr > 0, end > begin */
		return( COUNT(BEGIN_LIMIT(loop),END_LIMIT(loop), INCREMENT(loop)));
	case NOT_GE: /* Need incr < 0, begin > end */
		return( COUNT(-BEGIN_LIMIT(loop), -END_LIMIT(loop), -INCREMENT(loop)));

		/* E.g., while(name) */
	case NOT_NAME:
	case NOT_NE:
        	if( (BEGIN_LIMIT(loop)-END_LIMIT(loop)) % INCREMENT(loop) != 0 ) {
			
			DEBUG(aflags[lu].level&4,
				("not unrollable: (BEGIN_LIMIT-END_LIMIT) % INCREMENT != 0 ( %d - %d ) % %d\n",
				BEGIN_LIMIT(loop), END_LIMIT(loop), INCREMENT(loop)));
			return(0);
		}
		if(INCREMENT(loop) > 0 && BEGIN_LIMIT(loop) < END_LIMIT(loop)) 
			return( COUNT(BEGIN_LIMIT(loop), (END_LIMIT(loop)) - 1, INCREMENT(loop)));
		else if(INCREMENT(loop) < 0 && BEGIN_LIMIT(loop) > END_LIMIT(loop))
			return( COUNT(-BEGIN_LIMIT(loop), -(END_LIMIT(loop)) - 1, -INCREMENT(loop)));
		else {
			DEBUG(aflags[lu].level&4,
			("not unrollable: NOT_NE or NOT_NAME and can\'t figure ITERATIONS\n"));
			return 0;
		}
	case NOT_LT: /* Need incr > 0 */
       	 	return( COUNT(BEGIN_LIMIT(loop), (END_LIMIT(loop)) - 1, INCREMENT(loop)));
	case NOT_GT: /* Need incr < 0 */
       	 	return( COUNT(-BEGIN_LIMIT(loop), -(END_LIMIT(loop)) - 1, -INCREMENT(loop)));
	default:
        	return(0);
	} /* for switch */

}

static Boolean
uses(counter_id, node)
Setid counter_id;
ND1 *node;
{
	if(node->op == NAME) {
		if(! OPTABLE(node) )
			return false;
		else return (node->opt->object->setid == counter_id);
	}
	else if(node->right && node->left)
		return (uses(counter_id, node->right) || uses(counter_id, node->left));
	else if(node->right)
		return uses(counter_id, node->right);
	else if(node->left)
		return uses(counter_id, node->left);
	else return false;
}

	/*
	** For now, we check that the loop update immediately
	** precedes the loop test.  In the future, we should
	** also handle:
	**
	**     -counter update at top of loop
	**     -counter update at arbitrary point in loop ( but
	**	must be unconditionally executed ) 
	**     -counter update in loop test (while(i++)).
	**
	** This code assumes that the loop increment has
	** already been found in the same block as the loop
	** test.
	*/
static void
check_counter_update(loop)
Loop loop;
{
	Loop_unroll_info l_u_i;
	int counter_id;

	l_u_i = loop->loop_unroll;
	counter_id = l_u_i->loop_counter->object->setid;

	CGQ_FOR_ALL_BETWEEN_REVERSE(elem, index, 
		l_u_i->loop_increment_index,
		CGQ_PREV_INDEX(CGQ_ELEM_OF_INDEX(l_u_i->loop_cbranch_index)))

		ND1 *node = HAS_ND1(elem);
		if(node) {
			if(uses(counter_id, node) && index !=
				l_u_i->loop_increment_index) {
				if(LU_FLAGS(loop) == CONST_LIMITS) {
					LU_FLAGS(loop) = NO_UNROLL;
					DEBUG(aflags[lu].level&2,
					("\tchange const_lim to no_unroll\n"));
				}
				else
					LU_FLAGS(loop) |= LEAVE_COUNTER_INCR;
				DEBUG(aflags[lu].level,
					("\t counter use between update and test\n")); 
				return;
			}
		}
	CGQ_END_FOR_ALL_BETWEEN
}

/* 
** Determine if the loop test is of the appropriate form for
** unrolling.  Determine if the induction variable is being
** compared against a constant or a loop invariant. Categorize
** the loop test pattern - will be used for determining how
** many times the loop will be unrolled.
*/

static void
get_test_type(loop)
Loop loop;
{

	ND1 *node;
	int below_not;

	branch_op_type = 0;

		/* node is the CBRANCH, we think */
	node = CGQ_ELEM_OF_INDEX(loop->loop_unroll->loop_cbranch_index)->
			cgq_arg.cgq_nd1;

	if (node->op != CBRANCH ) {
		DEBUG(aflags[lu].level&4,
			("\t not unrollable: can\'t find loop test\n")); 
		LU_FLAGS(loop) = NO_UNROLL;
		return;
	}

	node = node->left;

	if (node->op == NOT) {
		below_not = 1;
		node = node->left;
	}
	else
		below_not = 0;

	if(node->op == NAME) {
		if(below_not && IS_INDVAR(node,loop)) {
				loop->loop_unroll->loop_counter = 
						node->opt;
				LU_FLAGS(loop) = CONST_LIMITS;
				branch_op_type = NOT_NAME;
		}
		else {
			DEBUG(aflags[lu].level&4,
				("\tnot unrollable: NAME but below_not && IS_INDVAR fails\n")); 
			LU_FLAGS(loop) = NO_UNROLL;
		}
	}
	else if( node->left->op != NAME || ! IS_INDVAR(node->left,loop) ||
		! (node->right) ||
		! IS_ICON(node->right) && ! IS_INVARIANT(node->right,loop)) {
		DEBUG(aflags[lu].level&4,
				("\tnot unrollable: can\'t dope out loop test\n")); 

		LU_FLAGS(loop) = NO_UNROLL;
	}
	else {
		if (IS_ICON(node->right)) {
			long v;
			if (num_toslong(&node->right->c.ival, &v)) {
				DEBUG(aflags[lu].level&4,
					("\tnot unrollable: big end value\n"));
				LU_FLAGS(loop) = NO_UNROLL;
				return;
			}
			LU_FLAGS(loop) = CONST_LIMITS;
		} else
			LU_FLAGS(loop) = INVAR_LIMITS;

		loop->loop_unroll->loop_counter = node->left->opt;
		loop->loop_unroll->end_limit = node->right;

		switch(node->op) {
		case NE:
			if (below_not)
				(branch_op_type = NOT_NE);
			else {
				DEBUG(aflags[lu].level&4,
					("\tnot unrollable: NE not below NOT\n")); 
				LU_FLAGS(loop) = NO_UNROLL;
			}
			break; 
		case EQ:
			if (below_not) {
				DEBUG(aflags[lu].level&4,
					("\tnot unrollable: EQ below NOT\n")); 
				LU_FLAGS(loop) = NO_UNROLL;
			}
			else 
				branch_op_type = NOT_NE;
			break;
					/*
					** < is the same as NOT >= , etc...
					*/
		case LT:
			branch_op_type = below_not ? NOT_LT : NOT_GE;
			break;
		case LE:
			branch_op_type = below_not ? NOT_LE : NOT_GT;
			break;
		case GT:
			branch_op_type = below_not ? NOT_GT : NOT_LE;
			break;
		case GE:
			branch_op_type = below_not ? NOT_GE : NOT_LT;
			break;
		default:
			DEBUG(aflags[lu].level&4,
				("\tnot unrollable: unrecognized comp op\n")); 
			LU_FLAGS(loop) = NO_UNROLL;
		}
	}
}

/*
** Find the initialization of the loop counter.  If it's not
** constant, we can only partially unroll.
** Set BEGIN_LIMIT(loop) if it's a constant.
*/

static void
get_initial_type(loop)
Loop loop;
{

	Block block;

		/*
		** Look for the block containing os_loop(start) 
		** and hope it contains the initialization of
		** the induction variable.  Otherwise, can only
		** partially unroll the loop.
		*/
	if(loop->header->block_flags & BL_PREHEADER) {
		DEBUG(aflags[lu].level&4,("Searching HEADER for ind_var init.\n"));
		block = loop->header;
	}
	else if(loop->header->pred->block->block_flags & BL_PREHEADER) {
		DEBUG(aflags[lu].level&4,("Searching PREHEADER for ind_var init.\n"));
		block = loop->header->pred->block;
	}
	else {
		return;
	}

	FOR_ALL_ND1_IN_BLOCK_REVERSE(block,flow,node,index)

		if ( find_ind_init(node,loop) ) {
			DEBUG_COND(aflags[lu].level&128,tr_e1print(loop->loop_unroll->begin_limit,"Found ind_var init:"));
			return;
		}

	END_FOR_ALL_ND1_IN_BLOCK
	
	LU_FLAGS(loop) = INVAR_LIMITS;
	
	DEBUG_COND(aflags[lu].level&128,"Can't find ind var init\n");
	return;  
}

/*
** Traverse the ND1 looking for the assignment of the 
** induction variable to it's initial value. If the assignment 
** is within a COMOP, must be able to find it.  
** If the assignment is to an ICON, then we know the start value 
** of the loop iterator. If it's to a loop invariant, then the
** start value is unknown at this time. 
** Return 0 if the search may be continued, 1 if the type of
** the initialization has been determined, i.e., the assignment to
** the induction variable has been seen.
*/

static int
find_ind_init(node,loop)
ND1 *node;
Loop loop;
{
	if ( node->op != COMOP) {
		if (node->op == ASSIGN && node->left->opt->object->setid ==
			loop->loop_unroll->loop_counter->object->setid) {

			loop->loop_unroll->begin_limit = node->right;
			if (IS_ICON(node->right)) {
				long v;
				if (num_toslong(&node->right->c.ival, &v)) {
					DEBUG(aflags[lu].level&4,
						("\tnot unrollable: big begin value\n"));
					LU_FLAGS(loop) = NO_UNROLL;
					loop->loop_unroll->begin_limit = 0;
					return 0;
				}
			} else {
				LU_FLAGS(loop) = INVAR_LIMITS;
			}
			return(1);
		}
	}
	else {
		return 
			find_ind_init(node->left,loop) ||
			find_ind_init(node->right, loop);
			
	}

	return(0);
}


int lu_f_call_penalty = LU_L_SIZE_THRESHOLD;

static int
cost_trees(loop)
Loop loop;
{
	Block block, start_block, stop_block;
	int trees = 0;
	int branch_count = 0, has_float = 0;
	
	start_block = loop->header->next;
	stop_block  = loop->end;

	for(block = start_block; block != stop_block; block = block->next) {

			/*
			** Loops containing a function call are not
			** worth unrolling.
			*/
		if(target_flag != 6 && (block->block_flags & BL_CONTAINS_CALL)) {
			DEBUG(aflags[lu].level&4,
				("\tloop(%d) cost set greater than theshold(%d): loop has fn call\n",
				loop->id,
				lu_l_size_threshold));
				trees += lu_f_call_penalty;
		}

                CGQ_FOR_ALL_BETWEEN(elem, index, block->first, block->last)
                                /* 
				** If there is a static definition in the 
				** loop don't unroll it
				*/

			ND1 *node;
			if(elem->cgq_op == CGQ_EXPR_ND2 && 
				elem->cgq_arg.cgq_nd2->op == DEFNAM)
				return(lu_l_size_threshold + 1);
			if(elem->cgq_op == CGQ_EXPR_ND1) {
				node = HAS_ND1(elem);

				if (language == Cplusplus_language && 
				   cg_tree_contains_op(node, EH_LABEL)) {
					/* Can't unroll a loop with an EH label
					   in it (would get duplicated)	*/
					DEBUG(aflags[lu].level&4,
					  ("\tloop(%d) with EH label not unrolled\n", loop->id));
					return (lu_l_size_threshold + 1);
				}

				if (node->op == CBRANCH) {
					branch_count++;
				}
				if (node->type == TY_LDOUBLE ||
					node->type == TY_DOUBLE ||
					node->type == TY_FLOAT) {
					has_float = 1;
				}
				trees++;
			}
		CGQ_END_FOR_ALL_BETWEEN
	}


	if(has_float && branch_count > 1) {
		DEBUG(aflags[lu].level,
				("\tloop(%d) cost set greater than theshold(%d): loop has floats & branches\n",
				loop->id,
				lu_l_size_threshold));
				trees += lu_l_size_threshold;
	}
	DEBUG(aflags[lu].level&4,("cost_of_trees(%d):\n", trees));
	return(trees);
}

	/*
	** Go down the list of strength reduction temps
	** and eliminate those which do not have the loop counter
	** as their associated induction variable.  ( Any such
	** temp may not move in lock step with the counter.
	** Also eliminate those for which we cannot find
	** the update trees.  Note: at present we may be missing
	** some of these under comma ops. -- PSP
	*/

static void
analize_sr_temps(loop)
Loop loop;
{
	Cand_descr list, remember_list;
	Cgq_index from_index, to_index;
	int loop_counter_obj_id;

	from_index = loop->loop_unroll->body_begin->first;
	to_index = CGQ_PREV_INDEX(CGQ_ELEM_OF_INDEX(loop->loop_unroll->loop_cond));
	loop_counter_obj_id =
		loop->loop_unroll->loop_counter->object->setid;

		/*
		** Don't update sr temps if usage is too complicated
		*/
	remember_list = 0;
	list = SR_TEMP_LIST(loop);

	while(list) {
		if( SR_IV_ID(list) !=  loop_counter_obj_id ) {
			if(remember_list) {
				SR_NEXT_CAND(remember_list) = SR_NEXT_CAND(list);
			}
			else {
				SR_TEMP_LIST(loop) = SR_NEXT_CAND(list);
			}
		}
		else
			remember_list = list;
		list = SR_NEXT_CAND(list);
	}

	CGQ_FOR_ALL_BETWEEN(elem, index, from_index, to_index)
		ND1 *node;
		SX nameid;
		node = HAS_ND1(elem);
		if(node) switch(node->op) {
		case ASG PLUS:
		case ASG MINUS:
		case INCR:
		case DECR:
			node = node->left;
			if(!(node->op == NAME)) break;
			nameid = node->sid; /* SID */
			for(list = SR_TEMP_LIST(loop);
				list; list = SR_NEXT_CAND(list)) {
				if(nameid == SR_TEMP(list)) {
					assert(SR_UPDATE_INDEX(list) == CGQ_NULL_INDEX);
					SR_UPDATE_INDEX(list) = index;
				}
			}	
			break;	
		default:
			break;
		}	
	CGQ_END_FOR_ALL_BETWEEN

		/*
		** Remember the last item that's OK
		*/
	remember_list = 0;

	list = SR_TEMP_LIST(loop);

	while(list) {
		if(SR_UPDATE_INDEX(list) == CGQ_NULL_INDEX) {

			if(remember_list) {
				SR_NEXT_CAND(remember_list) = SR_NEXT_CAND(list);
			}
			else {
				SR_TEMP_LIST(loop) = SR_NEXT_CAND(list);
			}
		}
		else {
			remember_list = list;
		}
		list = SR_NEXT_CAND(list);
	}
	DEBUG_COND(aflags[lu].level&2,
		debug_str_cands(SR_TEMP_LIST(loop)));
}


#ifndef NODBG
static debug_flag;
#endif

static Arena rl_arena;
static ND1 *init_node;
static ND1 *malloc_size_var; /* size argument to malloc */
static ND1 *malloc_table_var; /* target of malloc */
static Cgq_index save_memset_index;
static Object_set aliases, killed, live_after_memset;
static int rl_address_in_table(ND1 *node);

static int
rl_legal_ref(ND1 *node)
{
	switch(optype(node->op)) {
	case BITYPE:
		switch(node->op) {
		case PLUS:
		case MINUS:
			if(rl_legal_ref(node->left) &&
				rl_legal_ref(node->right))
				return 1;
			return rl_address_in_table(node);
			break;
		default:
			return(rl_legal_ref(node->left) &&
				rl_legal_ref(node->right));
		}
		break;
	case UTYPE:
		if(node->op == STAR) {
			return rl_address_in_table(node->left);
		}
		else return rl_legal_ref(node->left);
		break;
	case LTYPE:
		if(node->op == ICON)
			return 1;
		if(node->op == NAME) {
			Object object = node->opt->object;
			if(object->flags&VALID_CANDIDATE)
				return(!bv_belongs(object->setid,live_after_memset));
			else
				return(!bv_belongs(object->setid,killed));
		}
		break;
	}
	return 0;
}


static int
rl_address_in_table(ND1 *node)
{
		
	if(! TY_ISPTR(node->type) || TY_CHAR != TY_DECREF(node->type))
		return 0;
	switch(node->op) {
	case NAME:
		break;
	case PLUS:
	case MINUS:
		if(node->left->op != NAME || ! TY_ISPTR(node->left->type) || ! TY_ISINTTYPE(node->right->type))
			return 0;
		if(node->right->op != NAME && node->right->op != ICON)
			return 0;
		else node = node->left;
		break;
	default:
		return 0;
	}

	return bv_belongs(node->opt->object->setid, aliases);
}

static int
rl_check_legal(Loop loop)
{
/* CHECK LEGALITY OF ALL REFS/KILLS IN LOOP:
	Only trees with side effects are assigns.
	Assigns must be:
	1) Assigns of address in table to VALID_CANDIDATE, which
	becomes an alias. Candidate must be dead on entry to HEAD.
	2) Assigns to VALID_CANDIDATE which is dead on entry to HEAD.
	3) Pointer assigns throught an alias to table.
*/
	killed = bv_alloc(OBJECT_SET_SIZE, rl_arena);
	aliases = bv_alloc(OBJECT_SET_SIZE, rl_arena);
	live_after_memset = loop->header->next->live_on_entry;

	DEBUG(debug_flag,("live_after_memset: "));
	DEBUG_COND(debug_flag,bv_print(live_after_memset));
	DEBUG(debug_flag,("\n"));

	bv_init(false, killed);
	bv_init(false, aliases);
	bv_set_bit(malloc_table_var->opt->object->setid,aliases);
	CGQ_FOR_ALL_BETWEEN(elem, index,
		save_memset_index, loop->loop_unroll->loop_end)
		ND1 *node, *nl;
			/* Don't check the memset node or the update of
			** the loop counter,
			*/
		if(index == save_memset_index ||
			index == loop->loop_unroll->loop_increment_index)
			continue;
		if(node=HAS_ND1(elem)) {
				DEBUG(debug_flag,("\n"));
				DEBUG_COND(debug_flag, tr_e1print(node,"CHECK:"));
		}
		else continue;
		if(CONTAINS_SIDE_EFFECTS(node)) {
		switch(node->op) {
		ASSIGN_CASES:
			nl = node->left;
			switch(nl->op) {
			case NAME:
	/* Need to add check that no aliases to objects OTHER
	than table are created? */
				assert(nl->opt->object);
				bv_set_bit(nl->opt->object->setid,killed);
				DEBUG(debug_flag&4,("killed(%s(%d)): ",
				SY_NAME(nl->sid),nl->opt->object->setid));
				DEBUG_COND(debug_flag,bv_print(killed));
				DEBUG(debug_flag,("\n"));
				if(rl_address_in_table(node->right)) {
					bv_set_bit(nl->opt->object->setid,aliases);
					DEBUG(debug_flag&4,("alias(%s(%d)): ",
					SY_NAME(nl->sid),nl->opt->object->setid));
					DEBUG_COND(debug_flag,bv_print(aliases));
					DEBUG(debug_flag,("\n"));
				}	
				else if(bv_belongs(nl->opt->object->setid,
					
					live_after_memset)) {
					DEBUG(debug_flag&4,
				("object(%d) live after memset\n", nl->opt->object->setid));
					return 0;
				}
				break;
			case STAR:
				if(nl->left->op != NAME ||
					!bv_belongs(nl->left->opt->object->setid, aliases))
					return 0;
				break;
			default:
				return 0;
			}
			break;
		default:
			return 0;
		}}
	CGQ_END_FOR_ALL_BETWEEN
		/* We now know all assignments are OK
		** We need to check that all references are
		** to
		** a) objects not killed in the loop
		** b) or items which are dead following the memset
		** c) or derefs of pointers into the table
		** d) addresses in the table
		*/
	CGQ_FOR_ALL_BETWEEN(elem, index,
		save_memset_index, loop->loop_unroll->loop_end)
		ND1 *node;
			/* Don't check the memset node or the update of
			** the loop counter or the final loop test.
			*/
		if( index == save_memset_index ||
			index == loop->loop_unroll->loop_increment_index ||
			index == loop->loop_unroll->loop_cbranch_index)
			continue;
		if(!(node = HAS_ND1(elem)))
			continue;
		if(CONTAINS_SIDE_EFFECTS(node))
			node = node->right; /* It's an assign */
		if(! rl_legal_ref(node)) {
			DEBUG_COND(debug_flag,tr_e1print(node,"ILLEGAL REF:"));
			return 0;
		}
	CGQ_END_FOR_ALL_BETWEEN
	return 1;
}

static int
rl_get_test_type(loop)
Loop loop;
{

	ND1 *node;
	int below_not;

	branch_op_type = 0;

		/* node is the CBRANCH, we think */
	node = CGQ_ELEM_OF_INDEX(loop->loop_unroll->loop_cbranch_index)->
			cgq_arg.cgq_nd1;

	if (node->op != CBRANCH ) {
		DEBUG(debug_flag&4,
			("\t can\'t find loop test\n")); 
		LU_FLAGS(loop) = NO_UNROLL;
		return 0;
	}

	node = node->left;

	if (node->op == NOT) {
		below_not = 1;
		node = node->left;
	}
	else
		below_not = 0;

	if(node->op == NAME) {
		if(below_not && IS_INDVAR(node,loop)) {
				loop->loop_unroll->loop_counter = 
						node->opt;
				LU_FLAGS(loop) = CONST_LIMITS;
				branch_op_type = NOT_NAME;
				return 1;
		}
		else {
			DEBUG(debug_flag&4,
				("\tnot rl_able: NAME but below_not && IS_INDVAR fails\n")); 
			LU_FLAGS(loop) = NO_UNROLL;
			return 0;
		}
	}

		/* op == NAME */
	if( ! IS_INDVAR(node->left,loop) || !(node->right)) {
		DEBUG(debug_flag&4,
			("\tnot rl_able: NAME but below_not && IS_INDVAR fails\n")); 
		LU_FLAGS(loop) = NO_UNROLL;
		return 0;
	}
	if(! IS_ICON(node->right) && ! IS_INVARIANT(node->right,loop)) {
		LU_FLAGS(loop) = CHECK_LIMIT;
		DEBUG(debug_flag&4,("CHECK_LIMIT\n"));
	}
	else {
		if (IS_ICON(node->right)) {
			long v;
			if (num_toslong(&node->right->c.ival, &v)) {
				DEBUG(aflags[lu].level&4,
					("\tnot unrollable: big end value\n"));
				LU_FLAGS(loop) = NO_UNROLL;
				return 0;
			}
			LU_FLAGS(loop) = CONST_LIMITS;
		} else
			LU_FLAGS(loop) = INVAR_LIMITS;
	}

	loop->loop_unroll->loop_counter = node->left->opt;
	loop->loop_unroll->end_limit = node->right;

	switch(node->op) {
	case NE:
		if (below_not)
			(branch_op_type = NOT_NE);
		else {
			DEBUG(debug_flag&4,
					("\tnot rl_able: NE not below NOT\n")); 
			LU_FLAGS(loop) = NO_UNROLL;
			return 0;
		}
		break; 
	case EQ:
		if (below_not) {
			DEBUG(debug_flag&4,
				("\tnot rl_able: EQ below NOT\n")); 
			LU_FLAGS(loop) = NO_UNROLL;
			return 0;
		}
		else 
			branch_op_type = NOT_NE;
		break;
			/*
			** < is the same as NOT >= , etc...
			*/
	case LT:
		branch_op_type = below_not ? NOT_LT : NOT_GE;
		break;
	case LE:
		branch_op_type = below_not ? NOT_LE : NOT_GT;
		break;
	case GT:
		branch_op_type = below_not ? NOT_GT : NOT_LE;
		break;
	case GE:
		branch_op_type = below_not ? NOT_GE : NOT_LT;
		break;
	default:
		DEBUG(debug_flag&4,
			("\tnot rl_able: unrecognized comp op\n")); 
		LU_FLAGS(loop) = NO_UNROLL;
		return 0;
	}
	DEBUG(debug_flag,("\n"));

	DEBUG(debug_flag,("pattern(%s):\n", pattern_st[branch_op_type]));
	return 1;
}

static int
rl_get_initial_type(loop)
Loop loop;
{

	Block block;

		/*
		** Look for the block containing os_loop(start) 
		** and hope it contains the initialization of
		** the induction variable.  Otherwise, can only
		** partially unroll the loop.
		*/
	if(loop->header->block_flags & BL_PREHEADER) {
		DEBUG(debug_flag,("Searching HEADER for ind_var init.\n"));
		block = loop->header;
	}
	else if(loop->header->pred->block->block_flags & BL_PREHEADER) {
		DEBUG(debug_flag,("Searching PREHEADER for ind_var init.\n"));
		block = loop->header->pred->block;
	}
	else {
		return 0;
	}

	FOR_ALL_ND1_IN_BLOCK_REVERSE(block,flow,node,index)

		init_node = node;
		if ( find_ind_init(node,loop) ) {
			DEBUG_COND(debug_flag,tr_e1print(loop->loop_unroll->begin_limit,"Found ind_var init:"));
			return 1;
		}

	END_FOR_ALL_ND1_IN_BLOCK
	
	LU_FLAGS(loop) = NO_UNROLL;
	
	DEBUG_COND(debug_flag,"Can't find ind var init\n");
	return 0;  
}

static int
rl_find_malloc_memset(Loop loop)
{
	Block block;
	Block_list predecessor;
	ND1 *node;
	int flag = 0;
	malloc_size_var = 0;
	malloc_table_var = 0;

		/*
		** Look for the block containing os_loop(start).
		** If it has a unique predecessor which dominates,
		** check that block for a malloc. Save the variable
		** being malloc'd and the size of the malloc.
		*/
	if(loop->header->block_flags & BL_PREHEADER) {
		block = loop->header;
	}
	else if(loop->header->pred->block->block_flags & BL_PREHEADER) {
		block = loop->header->pred->block;
	}
	else {
		return 0;
	}
	predecessor = block->pred;
	if(! predecessor)
		return 0;
	if(predecessor->next)
		return 0; /* Give up, too complicated */
DEBUG(debug_flag&4,("check BLOCK %d\n",predecessor->block->block_count));
	FOR_ALL_ND1_IN_BLOCK_REVERSE(predecessor->block,flow,node,index)
		switch(node->op) {
		case ASSIGN:
			flag = -1;
			if(
				node->right->op == CALL &&
				node->right->left->op == ICON &&
				(malloc_table_var = node->left)->op == NAME &&
				node->right->right->op == FUNARG &&
					/* found max */
				(malloc_size_var = node->right->right->left)->op == NAME &&
					/* found table */
				SY_CLASS(node->left->sid) == SC_AUTO &&
				TY_ISPTR(node->left->type) &&
				TY_CHAR == TY_DECREF(node->left->type) &&
 strcmp(SY_NAME(node->right->left->sid),"malloc")==0) {
				
				
				flag = 1;
			}
			break;
		default:
			if(CONTAINS_SIDE_EFFECTS(node))
				return 0;
			break;
		}
		if(flag) break;
	END_FOR_ALL_ND1_IN_BLOCK
	if(flag != 1) return 0;
	DEBUG_COND(aflags[ah].level&2,print_exprs(aflags[ds].level));
	CGQ_FOR_ALL_BETWEEN(elem, index,
		loop->loop_unroll->loop_body, loop->loop_unroll->loop_end)
		if(node = HAS_ND1(elem)) {
			save_memset_index = index;
			break;
		}
	CGQ_END_FOR_ALL_BETWEEN
	if(node == 0 || node->op != CALL) return 0;
	if(node->left->op != ICON || 
		 strcmp(SY_NAME(node->left->sid),"memset")!=0)
		return 0;
	
		/* Check memset is in 1st block of body */
	FOR_ALL_ND1_IN_BLOCK(loop->header->next,flow,node,index)
		if(index == save_memset_index)
			break;
		else
			return 0;
	END_FOR_ALL_ND1_IN_BLOCK
	if(node->right->op != CM ||
		node->right->right->op != FUNARG ||
                 node->right->right->left->op != NAME ||
		node->right->right->left->sid != malloc_size_var->sid)
		return 0;
	node = node->right->left;
	if(node->op != CM ||
		node->left->op != FUNARG ||
		node->left->left->op != NAME ||
		node->left->left->sid != malloc_table_var->sid ||
		node->right->op != FUNARG ||
		node->right->left->op != ICON)
		return 0;

		/* Now check the loop body for save kills:
		** Assignments to the malloc'd table,
		** assignments to unaliased autos which are
		** dead after the loop.
		*/
	flag = remap_object_ids(~0);
#ifndef NODBG
		/* Print exprs with level 1 detail */
	DEBUG_COND(aflags[ah].level&2,print_exprs(aflags[ds].level));
#endif
	if (flag) {
		OBJECT_SET_SIZE = flag;
		get_use_def(rl_arena, VALID_CANDIDATE);
		live_on_entry(OBJECT_SET_SIZE, rl_arena, VALID_CANDIDATE);
	}
	return rl_check_legal(loop);
}

static void
rl_adjust_loop_counter(Loop loop)
{
	ND1 *node, *init;

	switch(branch_op_type) {
	case NOT_GE:
	case NOT_GT:
		if(INCREMENT(loop)!= -1)
			return;	
		break;
	case NOT_LE:
	case NOT_LT:
		if(INCREMENT(loop)!=1)
			return;	
		break;
	default:
		return;
	}

	init = loop->loop_unroll->begin_limit;

	node = t1alloc();
	*node = *init;
	node->op = QUEST;
	node->left = t1alloc();
	*(node->left) = *init;
	switch(branch_op_type) {
		case NOT_GT:
			node->left->op = GE;
			break;
		case NOT_GE:
			node->left->op = GT;
			break;
		case NOT_LT:
			node->left->op = LE;
			break;
		case NOT_LE:
			node->left->op = LT;
			break;
		default:
			return;
	}
	node->left->left = t1alloc();
	*(node->left->left) = *(loop->loop_unroll->end_limit);
	node->left->right = init;
	node->right = t1alloc();
	*(node->right) = *init;
	node->right->op = COLON;
	node->right->left = t1alloc();
	*(node->right->left) = *init;
	node->right->right = t1alloc();
	*(node->right->right) = *init;
	switch(branch_op_type) {
	case NOT_LT:
	case NOT_GT:
		node->right->right->op = MINUS;
		node->right->right->left = t1alloc();
		*(node->right->right->left) = *(loop->loop_unroll->end_limit);
	
		node->right->right->right = tr_smicon(INCREMENT(loop));
		break;
	case NOT_LE:
	case NOT_GE:
		*(node->right->right) = *(loop->loop_unroll->end_limit);
		break;
	default:
		return;
	}
	node = op_amigo_optim(node);
	init_node->right = node;
	DEBUG_COND(debug_flag,tr_e1print(init_node,"NEW INIT:"));	
	new_expr(init_node,0);
}

void
redundant_loop(Loop first_loop)
{
	Loop loop;

	rl_arena = arena_init();
#ifndef NODBG
	debug_flag = aflags[ah].level;
#endif
	dominate(rl_arena);

	for(loop = first_loop; loop; loop = loop->next) {
		if(loop->parent) continue;
		DEBUG(debug_flag,("Check loop_id %d\n",loop->id));
		DEBUG_COND(! aflags[sr].xflag, find_ind_var_defs(loop));

		loop->loop_unroll->end_limit = 0;
		loop->loop_unroll->begin_limit = 0;

		DEBUG(debug_flag,("\nLoop(%d) loop_cbranch_index(%d)\n",
			loop->id,
			DB_CGQ_INDEX_NUM(loop->loop_unroll->loop_cbranch_index) ));

		DEBUG(debug_flag,("increment_index(%d) increment(%d)\n",
			DB_CGQ_INDEX_NUM(loop->loop_unroll->loop_increment_index),
			INCREMENT(loop)));
		if ( loop->loop_unroll->loop_cbranch_index == CGQ_NULL_INDEX ||
		     loop->loop_unroll->loop_increment_index == CGQ_NULL_INDEX ||
		     INCREMENT(loop) == 0 )
			break;
		rl_get_test_type(loop);
		if((LU_FLAGS(loop) & NO_UNROLL) && !(LU_FLAGS(loop) & CHECK_LIMIT))
			break;
		if(!rl_get_initial_type(loop))
			break;
		if(LU_FLAGS(loop) & NO_UNROLL){
			break;
		}
		DEBUG_COND(debug_flag,print_lu_flags(loop));
		DEBUG_COND(debug_flag,debug_lu_data(loop));
		if(!rl_find_malloc_memset(loop))
			break;
		else
			  DEBUG(debug_flag&2,("found malloc/memset\n"));

		rl_adjust_loop_counter(loop);
DEBUG(debug_flag,("Did redundant loop in %s loop(%d)\n", get_function_name(),loop->id));
	}
	arena_term(rl_arena);
}
