#ident	"@(#)amigo:common/str_red.c	1.29"
#include "amigo.h"
#include <l_unroll.h>
#include <str_red.h>
#include <memory.h>
#include "costing.h"

	/* Codes for cand_type member */
#define C_ICON		1
#define C_CAND		2
#define C_IND_VAR	3
#define C_LOOP_INV	4

#define C_CAND_TYPE_BITS 3	/* Bits needed to encode cand_type */

	/* Codes for candidate operators */
#define C_MUL	0
#define C_LS	1
#define C_PLUS	2
#define C_MINUS 3
#define C_OPS_BITS 2	/* Bits required to encode candidate ops */

	/* PATTERN_SIZE: total possible patterns X OP Y */
#define PATTERN_SIZE 1 << 2*C_CAND_TYPE_BITS + C_OPS_BITS



#ifndef NODBG
static char *cand_strng[] = {
	"0",
	"C_ICON",
	"C_CAND",
	"C_IND_VAR",
	"C_LOOP_INV"
};
#endif

extern void PROTO(find_ind_vars,(Loop first_loop));
extern void PROTO(find_ind_var_defs,(Loop));

static void PROTO(find_candidates,(Loop first_loop));
extern void PROTO(loop_debug,(void));
static void PROTO(enter_candidate,(Cand_descr, ND1 *node, Loop loop));
static void PROTO(gen_update_code,(Loop));
static void PROTO(gen_init_code,(Loop));
static ND1 * PROTO(make_assign, (Expr,Loop));
static Cand_descr PROTO(candidate,(ND1 *node, Loop loop));
static Cand_descr PROTO(leaf,(ND1 *,Loop));
static int PROTO(index,(int left_type,int opcode,int right_type));
static void PROTO(init_pattern, (int pattern[]));
static void PROTO(save_cand, (Loop, Cand_descr));
static Arena str_red_arena;
	DEBUG_UNCOND(
void PROTO(debug_ind_var_defs,(Loop, Def_descr [])); )

static cand_ar_size;
		/* candidates in loop being processed, indexed by expr */
static Cand_descr *candidate_array; 

		/*
		** array of occurrences of assignments to induction variables,
		** indexed by ind_var (object id). Note that there is only
		** one defining statement per induction variable, because
		** variables with more than one update in a loop are ruled
		** out.
		*/
extern Def_descr *ind_var_defs;


static Expr_set candidate_set;
static Expr_set candidate_init_set; /* elements of candidate_set for which
				       init code has not yet been generated */
void
strength_reduction(first_loop)
Loop first_loop;
{
	Loop loop;
	str_red_arena = arena_init();
	loop_expr_kills(first_loop);	/* We may not need the expr_kills
					   member of Loop, in which case the
					   call is not needed. -- PSP */
	candidate_set = Expr_set_alloc(str_red_arena);
	candidate_init_set = Expr_set_alloc(str_red_arena);

	cand_ar_size = 2*get_expr_count() + 1;

	candidate_array = Arena_alloc(str_red_arena, cand_ar_size, Cand_descr);


	find_ind_vars(first_loop);

	for(loop = first_loop; loop; loop = loop->next) {	
		find_ind_var_defs(loop);
		find_candidates(loop);
		gen_init_code(loop);	/* initialize candidates in header */
		gen_update_code(loop);	/* update candidates at each define */
	}

	arena_term(str_red_arena);
}

static Boolean changed; /* Set if we need to call new_expr on top level treee */

			/* Identify all assignments to induction variables */
static void
find_candidates(loop)
Loop loop;
{
	(void)memset((char *)candidate_array,0,sizeof(Cand_descr)*cand_ar_size);
	
	bv_init(false,candidate_set);

		/* Recursively process each tree in loop
		   to find all candidates for strength reduction.
		*/
	FOR_ALL_TREES_IN_LOOP(loop,node,bl,flow,index)
		Cand_descr cand;
		changed = false;
		assert(node && loop);
		cand = candidate(node,loop);
		if(cand && cand->cand_type == C_CAND) enter_candidate(cand,node,loop);

		if(changed) {
			DEBUG(aflags[sr].level&2,
				("Calling new_expr() on top level node:\n"));
			DEBUG_COND(aflags[sr].level&2,
				tr_e1print(node,"T"));
			new_expr(node,bl->block);
		}

	END_FOR_ALL_TREES_IN_LOOP
}

#define CAND_OP(op)	((op) == MUL||(op) == PLUS||(op) == MINUS||(op) == LS)
	/* 
	   Following differs from notion of "loop invariant" used for code motion
	   and such, in that it does not have to be costed.
	 */
#define IS_INVARIANT(node,loop) (OPTABLE(node) && \
!(bv_belongs(node->opt->setid, loop->expr_kills)))

#define IS_INDVAR(node,loop) (OPTABLE(node) && node->opt->object && \
bv_belongs(node->opt->object->setid,loop->ind_vars))

	/* classify candidate patterns ( see AMIGO design doc ) */
#define NOPATTERN       	0
#define IV_MUL_ICON       	1	/* ind var * const */
#define LS_ICON			2	/* ind var or cand ls by const > 0 */
#define CAND_PLUS_ICON_OR_LI	3	/* cand +- loop inv or const */
#define CAND_MUL_ICON		4	/* cand * const */
#define CAND_PLUS_IV		5	/* cand +- ind var */
#define CAND_PLUS_CAND		6	/* cand +- cand */

static Cand_descr
copy_if_entered(cand)
Cand_descr cand;
{
	if(!cand)
		return 0;
	else if( ! cand->in_table)
		return cand;
	else {
		Cand_descr rtrn;
		rtrn = Arena_alloc(str_red_arena,1, struct cand_descr);
		*rtrn = *cand;
		rtrn->in_table = 0;	
		return rtrn;
	}
}

static Cand_descr
candidate(node,loop)
ND1 *node;
Loop loop;
{
	static int pattern[PATTERN_SIZE];
	static Boolean init = 0;
	Cand_descr lft,  rt, rtrn;
	int op = node->op;
	if(!init) {
		init_pattern(pattern);
		init = 1;
	}

	if(OPTABLE(node)) {
		assert(node->opt->setid >= 0 && node->opt->setid < cand_ar_size);
		if(candidate_array[node->opt->setid])
			return candidate_array[node->opt->setid];
	}
	lft = node->left?candidate(node->left,loop):0;
	rt = node->right?candidate(node->right,loop):0;

	/* for the time being we do ops on constants ourselves -- PSP
	** At some point we may want to handle trees instead of numbers
	** in which case the following should return trees.  We could
	** generalize "constant" to "loop invariant" in this way.
	*/
#define CONMUL(x,y) ((x)*(y))
#define CONADD(x,y) ((x)+(y))
#define CONSUB(x,y) ((x)-(y))

	if(lft && rt && CAND_OP(op)) {
		int sign = (op == MINUS? -1:1);
		switch(pattern[index(lft->cand_type,op,rt->cand_type)]) {
		case IV_MUL_ICON:     /* ind_var * ICON */
			if(lft->cand_type == C_ICON) {
				assert(rt->cand_type == C_IND_VAR);
				rtrn = copy_if_entered(lft);
				rtrn->update = CONMUL(rtrn->update,rt->update);
				rtrn->iv_id = rt->iv_id;
			}
			else {
				assert(rt->cand_type == C_ICON &&
					lft->cand_type == C_IND_VAR);
				rtrn = copy_if_entered(rt);
				rtrn->update = CONMUL(rtrn->update,lft->update);
				rtrn->iv_id = lft->iv_id;
			}
			rtrn->cand_type = C_CAND;
			break;
		case LS_ICON: /* (i_v or candidate) << (positive) ICON */
			if(TY_ISUNSIGNED(node->right->type)
				&& rt->update != 0 || rt->update > 0){

				rtrn = copy_if_entered(lft);
				rtrn->cand_type = C_CAND;
				rtrn->update = lft->update * 2<<(rt->update - 1); /* OK? -- PSP */
			}
			else
				rtrn = 0;
			break;
		case CAND_PLUS_ICON_OR_LI: /* cand +- ICON or LOOP_INV */
			
			if(lft->cand_type != C_CAND) {
				rtrn = copy_if_entered(rt);
				rtrn->update = sign * rtrn->update;
			}
			else rtrn = lft;
			break;
		case CAND_MUL_ICON: /* cand * ICON */
			if(lft->cand_type == C_CAND) {
				assert(rt->cand_type == C_ICON);
				rtrn = copy_if_entered(lft);
				rtrn->update = CONMUL(rtrn->update,rt->update);
			}
			else {
				assert(lft->cand_type == C_ICON);
				rtrn = copy_if_entered(rt);
				rtrn->update = CONMUL(rtrn->update,lft->update);
			}
			break;
		case CAND_PLUS_IV: /* cand +- ind var */
			if(lft->cand_type == C_CAND) {
				assert(rt->cand_type == C_IND_VAR);
				if(lft->iv_id != rt->iv_id)
					rtrn = 0;
				else {
					rtrn = copy_if_entered(lft);
					if(sign == -1)
						rtrn->update = CONSUB(rtrn->update,rt->update);
					else
						rtrn->update = CONADD(rtrn->update,rt->update);
				}
			}
			else {
				assert(lft->cand_type == C_IND_VAR &&
					rt->cand_type == C_CAND);
				if(lft->iv_id != rt->iv_id)
					rtrn = 0;
				else {
					rtrn = copy_if_entered(rt);
					if(sign == -1)
						rtrn->update = CONSUB(lft->update,rtrn->update);
					else
						rtrn->update = CONADD(lft->update,rtrn->update);
				}
			}
			break;
		case CAND_PLUS_CAND: /* cand +- cand */
			if(lft->iv_id != rt->iv_id)
				rtrn = 0;
			else {
				rtrn = copy_if_entered(lft);
				if(sign == -1)
					rtrn->update = CONSUB(rtrn->update,rt->update);
				else
					rtrn->update = CONADD(rtrn->update,rt->update);
			}
			break;
		case NOPATTERN:
			rtrn = copy_if_entered(leaf(node,loop));
			break;
		default:
			Amigo_fatal("Bad pattern code");
		}
	}
	else {
		rtrn = copy_if_entered(leaf(node,loop));
	}
	if(!rtrn || rtrn->cand_type != C_CAND) {
		if(lft && lft->cand_type == C_CAND) {
			enter_candidate(lft,node->left,loop);
		}
		if(rt && rt->cand_type == C_CAND) {
			enter_candidate(rt,node->right,loop);
		}
	}

	DEBUG(aflags[sr].level & 4,
		("returning %s\n",(rtrn?cand_strng[rtrn->cand_type]:"")));
	DEBUG_COND(aflags[sr].level & 4, tr_e1print(node,"T"));

	return rtrn;
#undef CONADD
#undef CONMUL
#undef CONSUB
}

	/*
	** This function gets called by candidate() when it
	** sees a node that doesn't fall into one of the patterns
	** but needs further analysis.
	*/
static Cand_descr
leaf(node,loop)
ND1 *node;
Loop loop;
{
	Cand_descr rtrn;
	long val;

	assert(node && node->opt);
	if(node->op == ICON) {
		if (num_toslong(&node->c.ival, &val))
			return 0; /* value is too big */
		rtrn = Arena_alloc(str_red_arena,1, struct cand_descr);
		rtrn->in_table = 0;	
		rtrn->cand_type = C_ICON;
		rtrn->update = val;
		return rtrn;
	}

	if(node->op == UNARY MINUS) {
		if(rtrn = copy_if_entered(candidate(node->left,loop)))
			rtrn->update = - rtrn->update;
		return rtrn;
	}

	if(IS_INDVAR(node,loop)) {
		ND1 *def_node = ind_var_defs[node->opt->object->setid].def;
		long sign = 1;;
		assert(def_node);
		rtrn = Arena_alloc(str_red_arena,1, struct cand_descr);
		rtrn->in_table = 0;	
		rtrn->cand_type = C_IND_VAR;
		rtrn->iv_id = node->opt->object->setid;
		switch(def_node->op) { /* figure out the update const for
					  this i_v by looking up the defining
					  occurrence. */
      		case ASG MINUS:
      		case DECR:
			sign = -1;
			/* FALLTHROUGH */
      		case INCR:
      		case ASG PLUS:
			assert(def_node && def_node->right);
			if (num_toslong(&def_node->right->c.ival, &val))
				Amigo_fatal("ind var value too big");
			rtrn->update = sign * val;
			break;
		default:
			Amigo_fatal("Bad defining occurrence for ind var");
		}
		return rtrn;
	}

	if(IS_INVARIANT(node,loop)) {
		rtrn = Arena_alloc(str_red_arena,1, struct cand_descr);
		rtrn->in_table = 0;	
		rtrn->cand_type = C_LOOP_INV;
		return rtrn;
	}
	return 0;
}


static int
index(left_type,opcode,right_type)
int left_type, opcode, right_type;
{
	int code;
	switch(opcode){
	case MUL:
		code = C_MUL;
		break;
	case LS:
		code = C_LS;
		break;
	case PLUS:
		code = C_PLUS;
		break;
	case MINUS:
		code = C_MINUS;
		break;
	default:
		Amigo_fatal("illegal opcode");
		break;
	}
	return((left_type << (C_CAND_TYPE_BITS + C_OPS_BITS))
		+ (code << C_CAND_TYPE_BITS) + right_type);
}

static void
init_pattern(pattern)
int pattern[];
{
		/* classify expr patterns ( see AMIGO design doc ) */
	memset((char *)pattern, 0, sizeof(int) * PATTERN_SIZE);
	pattern[index(C_IND_VAR,MUL,C_ICON)] = IV_MUL_ICON;
	pattern[index(C_ICON,MUL,C_IND_VAR)] = IV_MUL_ICON;
	pattern[index(C_IND_VAR,LS,C_ICON)] = LS_ICON;
	pattern[index(C_CAND,LS,C_ICON)] = LS_ICON;
	pattern[index(C_CAND,PLUS,C_ICON)] = CAND_PLUS_ICON_OR_LI;
	pattern[index(C_CAND,PLUS,C_LOOP_INV)] = CAND_PLUS_ICON_OR_LI;
	pattern[index(C_CAND,MINUS,C_LOOP_INV)] = CAND_PLUS_ICON_OR_LI;
	pattern[index(C_CAND,MINUS,C_ICON)] = CAND_PLUS_ICON_OR_LI;
	pattern[index(C_ICON,PLUS,C_CAND)] = CAND_PLUS_ICON_OR_LI;
	pattern[index(C_LOOP_INV,PLUS,C_CAND)] = CAND_PLUS_ICON_OR_LI;
	pattern[index(C_LOOP_INV,MINUS,C_CAND)] = CAND_PLUS_ICON_OR_LI;
	pattern[index(C_ICON,MINUS,C_CAND)] = CAND_PLUS_ICON_OR_LI;
	pattern[index(C_ICON,MUL,C_CAND)] = CAND_MUL_ICON;
	pattern[index(C_CAND,MUL,C_ICON)] = CAND_MUL_ICON;
	pattern[index(C_CAND,PLUS,C_IND_VAR)] = CAND_PLUS_IV;
	pattern[index(C_CAND,MINUS,C_IND_VAR)] = CAND_PLUS_IV;
	pattern[index(C_IND_VAR,PLUS,C_CAND)] = CAND_PLUS_IV;
	pattern[index(C_IND_VAR,MINUS,C_CAND)] = CAND_PLUS_IV;
	pattern[index(C_CAND,PLUS,C_CAND)] = CAND_PLUS_CAND;
	pattern[index(C_CAND,MINUS,C_CAND)] = CAND_PLUS_CAND;
}

#define SR_BENEFIT(node) ( COST(node) - sr_threshold ) 
/* - update_cost(type) - load_cost(type) ?? */

static Boolean
candidate_is_cost_effective(node)
ND1 * node;
{
	DEBUG_UNCOND(nodes_costed_count++;)
	if(( node->opt->flags & EXPENSIVE ) == 0) return false;
	return SR_BENEFIT(node) > 0;
}

	/* process a node that has just been identified as a candidate:
		i) Make sure this candidate is in the set of candidates
		ii) Add node to list of occurrences for this candidate
	*/
static void 
enter_candidate(candidate, node, loop)
Cand_descr candidate;
ND1 *node;
Loop loop;
{
	if(OPTABLE(node) && candidate_is_cost_effective(node)) {
START_OPT
		DEBUG_UNCOND(struct Object_info *p = debug_object(candidate->iv_id);)
		DEBUG(aflags[sr].level&8,
			("Node is candidate %d with update %ld benefit %d, induction var %s(%d):\n",
			node->opt->setid,
			candidate->update,
			SR_BENEFIT(node),
			p->fe_numb ? SY_NAME(p->fe_numb):"?",
			candidate->iv_id ));
		DEBUG_COND(aflags[sr].level&8, tr_e1print(node,"T"));

		replace_expr_by_temp(node->opt,loop->scope,node);

		DEBUG(aflags[sr].level&8,("New node:\n"));
		DEBUG_COND(aflags[sr].level&8, tr_e1print(node,"T"));

		DEBUG(aflags[sr].level&16,
			("node cost(sr): %d\n",node->opt->cost));

		if(!candidate_array[node->opt->setid]) {
			candidate->in_table = 1;
			candidate->temp = node->opt->temp;
			candidate->update_index = CGQ_NULL_INDEX;
			candidate_array[node->opt->setid] = candidate;
			bv_set_bit(node->opt->setid,candidate_set);
			save_cand(loop, candidate);
		}

		changed = true;

STOP_OPT
	}
#ifndef NODBG
	else {
		DEBUG(aflags[sr].level&8, 
			("Called enter_candidate but rejected node for s_r:\n"));
		DEBUG_COND(aflags[sr].level&8, tr_e1print(node,"T"));

	}
#endif
}


static ND1 *PROTO(edit,(Expr,Loop));

		/* 

		For every candidate, insert update code
		in the corresponding induction variable definition
		node.

		*/

static void
gen_update_code(loop)
Loop loop;
{
	DEBUG_UNCOND( Boolean didsomething = false;)

	BV_FOR(candidate_set, cand_id)

	Cand_descr cand = candidate_array[cand_id];
	ND1  *n1, *n2;
	DEBUG_UNCOND( if(!didsomething) didsomething = true;)
		/*
		** Following code replaces the definition node
		** for this candidate's induction variable ( call
		** it NODE ) by
		**		 ,
		**		/ \
		**	       /   \
		**	     NODE  +=
		**		   / \
		**		  /   \
		**		temp  ICON ( value = cand->update )
		**
		** This becomes the new definition node for the ind
		** variable.
		**
		** In the loop, all occurrences of the candidate
		** will be replaced by temp.
		*/

		/* First, start building the += node */
	n1 = t1alloc();
	n1->op = ASG PLUS;
	n1->left = make_temp(expr_info_array[cand_id],loop->scope);
	n1->type = n1->left->type;
	n1->flags = FF_SEFF;

		/* Build the ICON for the update */
	n2 = tr_smicon(cand->update);
	n2->type = n1->type;

	n1->right = n2;

	/* n1 is now: temp += update */	

		/* Make a copy of the old node */
	n2 = t1alloc();
	*n2 = *ind_var_defs[cand->iv_id].def; 

		/* Put the copy on the LHS of what will be the new def */
	ind_var_defs[cand->iv_id].def->left = n2;

	n2 = ind_var_defs[cand->iv_id].def; /* n2 is the new def now */
	n2->right = n1;
	n2->op = COMOP;
	n2->type = n2->right->type;
	n2->flags = FF_SEFF; /* OK ??? -- PSP */
	new_expr(n2, ind_var_defs[cand->iv_id].block);/* efficient? -- PSP */
	DEBUG(aflags[sr].level&8,
			("New induction var definition node \
updates candidate %d loop %d: \n", cand_id,loop->id));
		DEBUG_COND(aflags[sr].level&8,tr_e1print(n2,"T"));

	END_BV_FOR

	DEBUG_COND(didsomething && aflags[sr].level&1,
		debug_ind_var_defs(loop,ind_var_defs));
}

		/* 

		For every candidate, insert initialization code
		in the loop header for the corresponding temp.

		*/

static void
gen_init_code(loop)
Loop loop;
{
	struct cand_list {
		struct cand_list *next;
		Setid cid; /* expr id of this candidate */
	} *list = 0;

	bv_assign(candidate_init_set, candidate_set);	

		/* Reverse the list so expressions are processed before
		   subexpressions, that way subexpressions get evaluated,
		   if they are candidates, in subtrees. */

	BV_FOR(candidate_set, cand_id)
		struct cand_list *p =
			Arena_alloc(str_red_arena,1,struct cand_list);
		p->cid = cand_id;
		p->next = list;
		list = p;
	END_BV_FOR

	for(;list;list = list->next) 
		if(bv_belongs(list->cid, candidate_init_set)) {
			Expr this_expr = expr_info_array[list->cid];
			ND1 *assign_node = make_assign(this_expr,loop);
			DEBUG(aflags[sr].level & 4,("Evaluating candidate %d \
in loop %d into temp #%d\n", list->cid, loop->id, assign_node->left->sid));
			DEBUG_COND(aflags[sr].level & 4,
				print_expr(expr_info_array[list->cid]));
			DEBUG(aflags[sr].level&1,
				("Init for str red on candidate %d loop %d temp_sid(%d) \
update %ld benefit:%d:\n",
				list->cid,loop->id,assign_node->left->sid,
				candidate_array[list->cid]->update,
				SR_BENEFIT(assign_node->right)));
			DEBUG_COND(aflags[sr].level&1,tr_e1print(assign_node->right,"T"));
			insert_node(assign_node,loop->header);
			new_expr(assign_node, loop->header);
			bv_clear_bit(list->cid,candidate_init_set);
		}
}

static ND1 *
make_assign(expr,loop)
Expr expr;
Loop loop;
{
	ND1 *assign_node = t1alloc();
	assign_node->op = ASSIGN;
	assign_node->left = make_temp(expr,loop->scope);
	assign_node->right = edit(expr,loop);
	assign_node->type = assign_node->right->type; 
	assign_node->flags = FF_SEFF;
	bv_clear_bit(expr->setid,candidate_init_set);
	return assign_node;
}

static ND1 *
edit(expr,loop)
Expr expr;
Loop loop;
{
	ND1 *new_nd1 = t1alloc();
	*new_nd1 = expr->node;

	if(CHILD(expr,left)) {
		Expr child = CHILD(expr,left);
		if(OPTABLE_EXPR(child))
			if(bv_belongs(child->setid,candidate_init_set)) {
				/* subexpression to init */
				new_nd1->left = make_assign(child,loop);
				DEBUG(aflags[sr].level&1,("Initializing subcandidate %d:\n",child->setid));
				DEBUG_COND(aflags[sr].level&1,tr_e1print(new_nd1->left->right,"T"));
			}
			else
				new_nd1->left = edit(child,loop);
		else
			new_nd1->left = expr_to_nd1(child);
	}

	if(CHILD(expr,right)) {
		Expr child = CHILD(expr,right);
		if(OPTABLE_EXPR(child))
			if(bv_belongs(child->setid,candidate_init_set)) {
				/* subexpression to init */
				new_nd1->right = make_assign(child,loop);
			}
			else
				new_nd1->right = edit(child,loop);
		else
			new_nd1->right = expr_to_nd1(child);
	}

	return new_nd1;
}

/* strength_reduction/loop_unrolling interface */


	/*
	** Make a copy of the candidate and save in loop unroll info
	*/
static void
save_cand(loop, cand)
Loop loop;
Cand_descr cand;
{
	Cand_descr copy;
	copy = Arena_alloc(global_arena,1, struct cand_descr);
	*copy = *cand;
	copy->next = SR_TEMP_LIST(loop);
	SR_TEMP_LIST(loop) = copy;
}

Cand_descr
next_candidate(cand)
Cand_descr cand;
{
	return(cand->next);
}

#ifndef NODBG
void
debug_str_cands(list)
Cand_descr list;
{
	while(list) {
		fprintf(stderr, "\tCand temp sid(%d) with iv(%d) update %d update index %d\n",
			list->temp, list->iv_id,
			list->update,
			list->update_index == CGQ_NULL_INDEX ? 0 : CGQ_INDEX_NUM(list->update_index));
		list = SR_NEXT_CAND(list);
	}
}
#endif
