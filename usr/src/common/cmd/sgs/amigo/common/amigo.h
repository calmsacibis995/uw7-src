#ident	"@(#)amigo:common/amigo.h	1.44.1.60"
#include "p1.h"
#include "mfile2.h"
#ifdef NODBG
#define NDEBUG /* turn off assert */
#endif

#include <assert.h>
#include "arena.h"

#ifndef PROTO
#ifdef __STDC__
#define PROTO(x,y) x y
#else
#define PROTO(x,y) x()
#endif
#endif

#include "bitvector.h"

#define EXPR_SET_SIZE ( expr_array_size )
#define OBJECT_SET_SIZE ( object_array_size )
#define ASSIGN_SET_SIZE ( EXPR_SET_SIZE ) /* for now */

#define BLOCK_WEIGHT_INIT 100
#define LOOP_COUNT 8


#define SETID_TO_EXPR(setid) (setid < EXPR_SET_SIZE ? expr_info_array[setid] : \
	(Amigo_fatal("expr array expand"),(struct Expr_info *)0))

/* should include array in the bv? RMA */
#define SETID_TO_OBJ(setid) (obj_info_array[setid])


#define EMPTY_BLOCK(block) ( CGQ_NONE_BETWEEN(block->first, block->last) )

#define HAS_EXECUTABLE(flow) (HAS_ND1(flow) || flow->cgq_op == CGQ_EXPR_ND2 \
	&& \
	(flow->cgq_arg.cgq_nd2->op == JUMP || \
		flow->cgq_arg.cgq_nd2->op == LABELOP))

#define FOR_ALL_ND1_IN_BLOCK(block,flow,node,index) \
	if (block->block_flags & BL_HAS_EXPR) \
		CGQ_FOR_ALL_BETWEEN(flow,index,block->first,block->last) \
			ND1 *node = HAS_ND1(flow); \
			if(node) {

#define CGQ_FOR_ALL_IN_BLOCK_REVERSE(block,flow,index) { \
	cgq_t *flow; \
	Cgq_index index; \
	for (index=block->last; \
	    index != CGQ_ELEM_OF_INDEX(block->first)->cgq_prev;\
	    index = CGQ_ELEM_OF_INDEX(index)->cgq_prev ) { \
		flow = CGQ_ELEM_OF_INDEX(index); \
		{
#define END_CGQ_FOR_ALL_IN_BLOCK_REVERSE }}}
	/* Following is guaranteed to break from the iterator */
#define BREAK_CGQ_FOR_ALL_IN_BLOCK_REVERSE break

#define FOR_ALL_ND1_IN_BLOCK_REVERSE(block,flow,node,index) { \
	cgq_t *flow; \
	Cgq_index index; \
	ND1 *node; \
	if (block->block_flags & BL_HAS_EXPR) { \
		for (index=block->last; \
		    index != CGQ_ELEM_OF_INDEX(block->first)->cgq_prev;\
		    index = CGQ_ELEM_OF_INDEX(index)->cgq_prev ) { \
			flow = CGQ_ELEM_OF_INDEX(index); \
			node = HAS_ND1(flow); \
			if (node) {
			

#define END_FOR_ALL_ND1_IN_BLOCK } CGQ_END_FOR_ALL_BETWEEN

#define FOR_ALL_TREES_IN_LOOP(loop,node,blist,flow,index) \
	{ Block_list blist; for(blist=loop->blocks;blist;blist=blist->next) \
		FOR_ALL_ND1_IN_BLOCK(blist->block,flow,node,index)
#define END_FOR_ALL_TREES_IN_LOOP END_FOR_ALL_ND1_IN_BLOCK } 

#define DEPTH_FIRST(block_ptr) {\
	Block * block_ptr; \
	for(block_ptr = get_first_depth_first_block_ptr(); \
		block_ptr <= get_last_depth_first_block_ptr(); block_ptr++) {

#define REVERSE_DEPTH_FIRST(block_ptr) {\
	Block * block_ptr; \
	for(block_ptr = get_last_depth_first_block_ptr(); \
		block_ptr >= get_first_depth_first_block_ptr(); block_ptr--) {
#define END_DEPTH_FIRST }}

#define Amigo_fatal(msg) amigo_fatal(msg,__FILE__,__LINE__)

	/*
	** For Block_sets to be sane, they must be used
	** only while block count does not change.  Otherwise,
	** we have to do something else.
	*/
#define Block_set_alloc(arena) bv_alloc(get_block_count(),arena)

#define Expr_set_alloc(arena) bv_alloc(EXPR_SET_SIZE,arena)
#define Object_set_alloc(arena) bv_alloc(OBJECT_SET_SIZE,arena)
#define Assign_set_alloc(arena) bv_alloc(ASSIGN_SET_SIZE,arena)

/* context flags, returned by get_context */
#define IS_LVAL 	(1<<0)
#define IS_COND 	(1<<1)
#define UNDER_OP_EQ	(1<<2)
#define SIBLING_IS_CONST	(1<<3)

#define IS_RVAL(context) (!((context) & IS_LVAL))

extern char *dots;
extern char *varargs; /* different from any symbol string */

extern int generic_deref_is_killed, generic_call_is_killed;

#define IS_VALID_AUTO(sid) (1)	/* no additional constraints */
#define IS_VALID_PARAM(sid) (SY_NAME(sid) != dots && SY_NAME(sid) != varargs)
#define IS_VALID_EXTERN(sid) (!generic_deref_is_killed && version != V_CI4_1)
#define IS_VALID_STATIC(sid) (IS_VALID_EXTERN(sid))

#define IS_VALID_CANDIDATE(sid)	( \
	(sid) > 0				\
	&& !(SY_FLAGS(sid) & SY_UAND)		\
	&& TY_ISSCALAR(SY_TYPE(sid))		\
	&& !TY_ISVOLATILE(SY_TYPE((sid)))       \
	&& (					\
		(SY_CLASS(sid) == SC_AUTO && IS_VALID_AUTO(sid))	\
	     || (SY_CLASS(sid) == SC_PARAM && IS_VALID_PARAM(sid))	\
	     || (SY_CLASS(sid) == SC_EXTERN && IS_VALID_EXTERN(sid))	\
	     || (SY_CLASS(sid) == SC_STATIC && IS_VALID_STATIC(sid))	\
	) \
)

/* codes for second operator of get_context */
#define LEFT 0
#define RIGHT 1

	/* Flags for get_kills() and get_recursive_kills() */
#define ACCUMULATE 1
#define REMOVE 0


/* The first two macros decide if an expr or a tree may be precomputed even
** if they are not anticipated.  For example, an expression that may raise an 
** exception cannot be hoisted from a loop if it is conditionally executed in 
** the loop.
*/
#define OPTABLE_EXPR(expr) \
	(((expr)->flags & (OPTIMIZABLE|RAISES_EXCEPTION)) == OPTIMIZABLE)
#define OPTABLE(node) OPTABLE_EXPR(node->opt)

/* The next two macros ignore the exception flags.  These are to be used
** by optimizations which do not move code around, such as cse.
*/
#define EXCEPT_OPT_EXPR(expr) ((expr)->flags&OPTIMIZABLE)
#define EXCEPT_OPT(node) EXCEPT_OPT_EXPR(node->opt)

#define CONTAINS_SIDE_EFFECTS(node) (node->opt->flags&CONTAINS_SIDE)
#define CHILD(expr,direct)  (Expr) expr->node.direct 
#define OPTABLE_CHILD(expr,direct) OPTABLE_EXPR(CHILD(expr,direct))
#define EXCEPT_OPT_CHILD(expr,direct) EXCEPT_OPT_EXPR(CHILD(expr,direct))

typedef unsigned Setid;
typedef Bit_vector Expr_set;
typedef Bit_vector Object_set;
typedef Bit_vector Block_set;

typedef struct Object_list {
	struct Object_info *object;
	struct Object_list *next;
} *Object_list;

typedef struct Object_info *Object;

#define Object_list_alloc(arena) (Arena_alloc(arena,1,struct Object_list))

	/*
	** values for block_flags
	*/
#define BL_HAS_OS_LOOP	(1<<0)	/* os_loop(..) */
#define BL_HAS_EXPR	(1<<1)
#define BL_CONTAINS_CALL	(1<<2)
#define BL_CONTAINS_DIV	(1<<3)
#define BL_CONTAINS_FP_DIV	(1<<4)
#define BL_PREHEADER	(1<<5)	/* Contains os_loop(start) call */
#define BL_HAS_LOCAL	(1<<6)	/*
				** Starts or ends a lexical scope.
				** For now this means CGQ_START/END_SCOPE
				** or db_symbol or db_sy_clear.
				*/

typedef struct Loop_info *Loop;
#define Assign_set Expr_set /* For now, PSP */

	/* Info about edge from owning block to block_num */
typedef struct Edge_descriptor
{
	int block_num;
	Expr_set set_to_true;
	Expr_set set_to_false;
	struct Edge_descriptor *next;
} *Edge_descriptor;

typedef struct Block {
	int block_count;
	Cgq_index first, last;		/* trees in this block */
	struct Block *next;		/* for lexical order */
	int dfn;			/* depth-first number */
	int lex_block_num;
	struct Block_list *succ, *pred;	/* flow graph edges */
	Boolean seldom;
	Block_set control_reaches, dominators;
	Object_set use, def, live_on_entry;
	Object_set live_at_bottom;	/* RMA maybe change name */

	/* For now, reuse copy prop members */
#define bool_true avin
#define bool_false assigns_killed
	Edge_descriptor edge_list;

	Assign_set avin, reaches_exit, assigns_killed;
	Expr_set antic, loc_antic;	/* anticipated and locally anticipated */
	Object_list kills;		/* objects killed in block */
	Expr_set exprs_killed;		/* 
					** exprs killed in this block:
					** not current unless block_expr_kills()
					** has been called
					*/
	Object_set ind_vars;		/* objects satisfying:
					**  1) assigned to in this block
					**  2) all assigns of form += const
					**     or -= const
					*/
	Loop loop;		/* surrounding loop */
	TEMP_SCOPE_ID scope;
	int block_flags;
} *Block;

typedef struct Block_list {
	Block block;
	struct Block_list *next;
} *Block_list;

typedef struct def_descr {
	ND1 *def;
	Block block;
} Def_descr;

	/* Remove this and put fields in Loop_info, once it stablizes. */
typedef struct Loop_unroll_info *Loop_unroll;

	/*
	** Values for loop_flags field.  Currently, these are
	** used mostly for loop unrolling.
	*/
#define NO_UNROLL     (1<<0)
#define CONST_LIMITS  (1<<1)
#define INVAR_LIMITS  (1<<2)
#define LIMIT_MASK    (NO_UNROLL | CONST_LIMITS | INVAR_LIMITS)
#define LEAVE_COUNTER_INCR    (1<<3)
#define UNROLLED      (1<<4)
#define SIMPLIFIED    (1<<5)
#define CHECK_LIMIT    (1<<6)

#define LOOP_FLAGS(loop) (loop->loop_flags)

struct Loop_info {
	int id;
	Block header;	/*
			** Block containing the os_loop(body) call:
			** since this block always executes if the
			** loop body executes, this is the block
			** into which loop invariant code is moved.
			** The next block is the first block of the body.
			*/
	Block end;	/* The lexical successor to the last block
			   in the loop body: contains os_loop(end) call.
			*/
	Object_list kills;
	Expr_set expr_kills;
	Expr_set invariants;	/* Expr's that have been replaced by
				   tmps in this loop.
				*/
	Block_list blocks;	/* List of blocks properly belonging to
				   this loop, i.e., for which this loop
				   is the enclosing loop.
				*/
	Loop parent;		/* the enclosing loop */
	Loop prev, next;	/* Next for inner to outer, prev outer to inner
				*/
	Loop child, sibling;	/* child points to a list of nested
				   loops, all of which are siblings.
				*/
	Object_set ind_vars;	
	TEMP_SCOPE_ID scope;
	int header_pred_count;
	Loop_unroll loop_unroll;
	int loop_flags;
};

	/* Expression attributes (flags member) */
#define OBJECT 				(1<<1)
#define EXPENSIVE 			(1<<2)
#define OPTIMIZABLE 			(1<<3)
#define SIDE_EFFECT 			(1<<4)
#define CONTAINS_SIDE 			(1<<5)
#define EXCEPTABLE 			(1<<6)
#define CONTAINS_VOLATILE		(1<<7)
#define FLOATING_POINT 			(1<<8)
#define HAS_CALL			(1<<9)
#define CONTAINS_VOLATILE_ADDRESS 	(1<<10)
#define HAS_DIVIDE			(1<<11)
#define HAS_FP_DIVIDE			(1<<12)
#define IS_COPY				(1<<13)
#define POINTER_EXCEPTION		(1<<14)
#define ARITH_EXCEPTION			(1<<15)
#define POINTER_ARG			(1<<16)
#define DEREF_KILLS			(1<<17)
#define OBJECT_PLUS_CONST		(1<<18)

#define RAISES_EXCEPTION	(EXCEPTABLE|POINTER_EXCEPTION|ARITH_EXCEPTION)

typedef struct Expr_info {
	int cost;        /* estimated machine cycles to evaluate */
	unsigned flags;			
	TWORD type;			/* pass2 type of type in node
					   hashing uses this type
					*/
	ND1 node;			/* hash image of node, left and
					   right fields of this ND1 are
					   actually Expr_info *, rather 
					   than ND1 *
					*/
	Setid setid;			/* identifier for set operations */
	struct Object_info *object;
	struct Expr_info *next;		/* next in depth first order */
	struct Expr_info *chain;	/* next on hash chain */
	SX temp;			/* index for temp holding this expr */
	TEMP_SCOPE_ID temp_scope;		/* scope of temp */
	struct Object_info *addcon;	
} *Expr;

/* following flags for Object_info */
#define ESCAPED_STATIC 1	/* function static */
#define ESCAPED_AUTO 2		/* auto or param */
#define EXTERN 4		/* file or program statics and globals */
#define CAN_BE_IN_REGISTER 8	/* Non aliased local, not volatile, etc. */
#define VALID_CANDIDATE 16	/* Like CAN_BE_IN_REGISTER, but machine may
				** not have registers for its type (fp on 386)
				*/
#define ESCAPED (ESCAPED_STATIC|ESCAPED_AUTO|EXTERN)
/* one for every  optimizable object */

typedef struct scope_info *Scope;
typedef struct scope_usage *Scope_usage;

struct Object_info {
	int flags;
	int regclass;
	ND1 *value;	/* if not null gives value of constant */
	int fe_numb;
	Setid setid;
	Expr_set kills;
	int weight;
	int reg;
	Object_set set_interferes;
	struct Object_info *next;
	Scope scope;
	Scope_usage scope_usage; /* Linked list of (scope,count) pairs */
};

#define LEFT_INFO(info) ((struct Expr_info *)info->node.left)
#define RIGHT_INFO(info) ((struct Expr_info *)info->node.right)

extern void PROTO(	amigo_const,(SX, ND1 *));
extern void PROTO(	boolean_prop,(void));
extern void PROTO(	copy_prop,(Block));
extern void PROTO(	dead_store,(void));
extern void PROTO(	global_cse,(Block));
extern void PROTO(	local_const_folding,(void));
extern void PROTO(	loop_invariant_code_motion,(Loop));
extern Boolean PROTO(	loop_unrolling,(Loop));
extern void PROTO(	reg_alloc,( Arena, Block));
extern void PROTO(	redundant_loop,(Loop));

extern void PROTO(	dominate,(Arena));
extern Block PROTO(	get_f_end_block,(void));
extern Block PROTO(	get_f_end_block_pred,(void));
extern void PROTO(	enter_f_end_block_pred,(Block));

extern void PROTO(	strength_reduction,(Loop));
extern int PROTO(	mdopt,(void));
extern TEMP_SCOPE_ID PROTO( get_global_temp_scope, (void));
extern ND1 * PROTO(	amigo_op_optim,(ND1 *node));
extern void PROTO(	amigo_delete,(Block, Cgq_index));
extern void PROTO(	amigo_fatal,(char *, char *, int));
extern Expr PROTO(	amigo_hash, (ND1 *e));
extern cgq_t *PROTO(	amigo_insert,(Block, Cgq_index));
extern void PROTO(	amigo_remove,(Block, Cgq_index));
extern void PROTO(	anticipate,(Arena));
extern void PROTO(	avail_in,(Arena));
extern Block PROTO( 	basic_block,(Boolean *has_loop));
extern void PROTO(	boolean_avail_in,(Arena));
extern Cgq_index PROTO(	insert_label,(Cgq_index place, int label));
extern Cgq_index PROTO(	insert_nd1,(Cgq_index place, ND1 *tree, Block));
extern void PROTO(	block_expr_kills,(Block,int));
extern void PROTO(	clear_wasopt_flags,(ND1 *));
extern ND1 *PROTO(	expr_to_nd1,(Expr));
extern void PROTO(	fixup_live_on_entry,(void));
extern int PROTO(	get_block_count,(void));
extern int PROTO(	get_block_weight,(Block));
extern int PROTO(	get_context,(ND1 * node,int child, int in_context));
extern Block *PROTO(	get_first_depth_first_block_ptr,(void));
extern Block *PROTO(	get_last_depth_first_block_ptr,(void));
extern int PROTO(	get_expr_count,(void));
extern Expr PROTO(	get_expr_last,(void));
extern void PROTO(	get_kills,(Expr_set, ND1 *, int));
extern int PROTO(	get_object_count,(void));
extern Object PROTO(	get_object_last,(void));
extern int PROTO(	get_occur_count,(void));
extern void PROTO( 	get_recursive_kills,(Expr_set,ND1 *,int));
extern void PROTO(	get_use_def,(Arena, int cand_type_flag));
extern void PROTO(	hash_exprs,(void));
extern void PROTO(	insert_node,(ND1 *node,Block block));
extern Boolean PROTO(	is_generic_call,(Object));
extern Boolean PROTO(	is_generic_deref,(Object));
extern void PROTO(	live_at_bottom,(int bv_size, Arena, int flag));
extern void PROTO(	live_on_entry,(int bv_size, Arena, int flag));
extern void PROTO(	locally_anticipate,(Arena));
extern void PROTO(	local_info,(Block));
extern Loop PROTO(	loop_build,(Block, Boolean *loop_build_flag));
extern Boolean PROTO(	loop_simplify,(Loop));
extern void PROTO(	loop_expr_kills,(Loop));
extern ND1 * PROTO(	make_temp,(Expr,TEMP_SCOPE_ID));
extern int PROTO(	new_label,(void));
extern void PROTO(	seldom,(Arena));
extern void PROTO(	new_expr,(ND1 *, Block));
extern Object_list PROTO(node_kills,(ND1* node,Object_list kills,Object_set));
extern Object_list PROTO(object_list_union_eq,(Object_list, Object_list, int));
extern int PROTO(	order_blocks,(Block));
extern int PROTO(	remap_object_ids,(int));
void PROTO(		remove_object,(Object prior,Object obj));
extern void PROTO(	replace_expr_by_temp,(Expr, TEMP_SCOPE_ID, ND1 *));
#ifndef NODBG
extern Boolean PROTO(	rewrite_assigns,(ND1 *, int debug_flag));
#else
extern Boolean PROTO(	rewrite_assigns,(ND1 *));
#endif
extern void PROTO(	set_defs_refs,(Block, Expr_set kills, ND1 *node, int context, int cand_type_flag));
void PROTO(		set_object_last,(Object));
extern Expr PROTO(	sid_to_expr,(SX));
#define SID_TO_OBJ(sid)	(sid_to_expr(sid)->object)
extern void PROTO(	store_expr,(Expr, TEMP_SCOPE_ID, ND1 *));
extern void PROTO(	pre_amigo_rewrite,(void));
extern void PROTO(	post_amigo_rewrite,(void));
extern void PROTO(	get_flow_weights,(Block, Arena));
extern Object PROTO(	get_generic_call,(void));
extern Object PROTO(	get_generic_deref,(void));




extern Arena global_arena, file_arena; /* function wide and file wide arenas */
extern Arena copy_arena;

extern target_flag; /* From cg flags, see cg:i386/local.c */

extern struct Expr_info **expr_info_array;
extern int expr_array_size;
extern int object_array_size;

extern Object_set generic_call_obj_kills;
extern Object_set generic_deref_obj_kills;

#define GLOBAL global_arena

extern Bit_vector BVZERO;
extern Bit_vector PROTO(bvclone,(Arena,Bit_vector));

#ifdef NODBG
#define START_OPT
#define STOP_OPT
#define DEBUG_COND(cond,state)
#define DEBUG_UNCOND(state)
#define ACCUMULATE_TIME(x)
#define POP_TIME
#define PUSH_TIME

#else
#include "debug.h"

#endif
