#ident	"@(#)amigo:i386/mdopt.c	1.14"

/* The one optimization in this file tries to reduce sign-extensions.
** In the future, we should probably add the straight-forward change
** to search for zero-extensions also.  Because of the delicate tree
** fiddling here, we can take two approaches to ensuring the correctness
** of ensuing optimizations:  we can perform this optimization immediately 
** before register allocation and after all other optimizations, or
** we can mark "delicate" trees as unoptimizable.  The latter approach 
** has been taken here, since it is more flexible.
*/

#include "amigo.h"

PROTO(	static void find_candidates,(void));
PROTO(	static void contingent,(Setid, Setid));
PROTO(	static void reduce_contingencies,(void));
PROTO(	static void check_recurse,(ND1 *, int flag));
PROTO(	static void check_assign,(ND1 *));
PROTO(	static int cost_effective,(Setid));
PROTO(	static int recurse_change,(Setid, ND1 *, Block));
PROTO(	static void make_change,(Setid));
PROTO(	static void generate_init,(void));

static Arena md_arena;

/* To not duplicate code, some of the code does double
** duty: costing or optimizing.
*/ 

typedef enum { cost_phase, optimize_phase } phase_type;
static phase_type phase;
static int current_cost;

/* Candidates paired on the contingecy list can be optimized only if their
** partner does not have an invalid assign (i.e. the partner is known to 
** fit into a short so word length instructions can be done).
*/
typedef struct contingency {
	Setid cand1, cand2;
	struct contingency *next;
} contingency;

static contingency *contingency_list;

#define objid(n)	n->opt->object->setid

/* We keep the object we are currently optimizing as a global.  We need
** certain fields of the object that are easier to obtain if we keep
** the object in its node form.
*/
ND1 *working_node;
ND1 *working_temp;

#define GET_WORKING_EXPR() 	working_node->opt
#define GET_WORKING_TEMP_SID() 	working_temp->sid

#define SHORT_ICON(n)	((n)->op == ICON && short_icon(n))
#define NON_NEGATIVE_SHORT_ICON(n) \
	(SHORT_ICON(n) && num_scompare(&(n)->c.ival, &num_0) >= 0)

static Object_set invalid;
static Object_set invalid_assign;
static Object_set sign_extend;

#define INVALID_ASSIGN(setid)	bv_set_bit(setid, invalid_assign)
#define INVALID(setid)		bv_set_bit(setid, invalid)
#define SIGN_EXTEND(setid)	bv_set_bit(setid, sign_extend)
#define IS_INVALID_ASSIGN(setid)  bv_belongs(setid, invalid_assign)
#define IS_INVALID(setid)	bv_belongs(setid, invalid)
#define IS_SIGN_EXTEND(setid)	bv_belongs(setid, sign_extend)

static int
short_icon(ND1 *p)
{
	INTCON tmp = p->c.ival;

	return num_snarrow(&tmp, SZSHORT) == 0;
}

int
mdopt()
{
	int i;
	int changes = 0;
	int save_set_size = OBJECT_SET_SIZE;

	md_arena = arena_init();

	contingency_list = NULL;

	OBJECT_SET_SIZE *= 2;	/* we may create new objects */
	invalid = Object_set_alloc(md_arena);
	invalid_assign = Object_set_alloc(md_arena);
	sign_extend = Object_set_alloc(md_arena);
	bv_init(false, invalid);
	bv_init(false, invalid_assign);
	bv_init(false, sign_extend);

	find_candidates();
	reduce_contingencies();
	bv_or_eq(invalid, invalid_assign);

	BV_FOR(sign_extend, setid)
		if (!IS_INVALID(setid) && cost_effective(setid)){
START_OPT
			phase = optimize_phase;
			make_change(setid);
			changes++;
STOP_OPT
		}
	END_BV_FOR
	arena_term(md_arena);
	OBJECT_SET_SIZE = save_set_size;
	return changes;
}

/* Flags for check_recurse() */

#define NESTED		01
#define UNDER_SWITCH	02

static void
find_candidates()
{
	CGQ_FOR_ALL(flow, index)

		ND1 *node = HAS_ND1(flow);
		if (node) 
			if (flow->cgq_op==CGQ_FIX_SWBEG)
				check_recurse(node, UNDER_SWITCH);
			else
				check_recurse(node,0);

	CGQ_END_FOR_ALL
}

static void
check_recurse(node, flags)
ND1 *node;
int flags;
{
	ND1 *l, *r;

	if (node->op == NAME) {
		if((node->type != TY_INT && node->type != TY_LONG)
		   || !(node->opt->object->flags&VALID_CANDIDATE)
		   || SY_CLASS(node->sid) != SC_AUTO)
			if (node->type != TY_SHORT)
				INVALID_ASSIGN(objid(node));
	}
	switch (optype(node->op)) {
	case BITYPE:
		l = node->left;
		r = node->right;
		while (l->op == CONV)
			l = l->left;
		while (r->op == CONV)
			r = r->left;
		if (!(l->op == NAME || r->op == NAME))
			break;
		switch (node->op) {
		case AND:
			if (l->op == NAME) {
				if (!NON_NEGATIVE_SHORT_ICON(r))
					INVALID(objid(l));
			}
			else if (!NON_NEGATIVE_SHORT_ICON(l))
				INVALID(objid(r));
			break;
		case EQ: case NE:
		case GT: case NGT:
		case GE: case NGE:
		case LT: case NLT:
		case LE: case NLE:
		case LG: case NLG:
		case LGE: case NLGE:
			/* We should not have unsigned versions here */
			if (l->op == NAME) {
				if (r->op == NAME) {
					contingent(objid(l), objid(r));
					break;
				}
				else if (SHORT_ICON(r))
					break;
				else if (r->type != TY_SHORT)
					INVALID(objid(l));
				break;
			}
			else if (SHORT_ICON(l)) 
				break;
			else if (l->type != TY_SHORT)
				INVALID(objid(r));
			break;
		default:
			/* Any object that is the target of a nested
			** assignment is invalid for the time being.
			** This restriction can be removed by improving
			** the tree rewriting routine.
			*/
			if (asgop(node->op)) {
				if (flags&NESTED) {
					ND1 *l = node->left;
					while (l->op == CONV)
						l = l->left;
					if (l->op == NAME)
						INVALID(objid(l));
				}
				check_assign(node);
				break;
			}
			/* Any other operation is not accepted */
			if (l->op == NAME)
				INVALID(objid(l));
			if (r->op == NAME)
				INVALID(objid(r));
			break;
		}
		break;
	case UTYPE:
		if (node->left->op != NAME)
			break;
		/* Permit NOT operator */
		if (node->op == NOT)
			break;
		/* Permit CONV to short */ 
		if (node->op == CONV && node->type == TY_SHORT)
			break;
		INVALID(objid(node->left));
		break;
	case LTYPE:
		/* Mark n in "switch(n)" invalid.  Maybe in the future
		** this can be valid, but it may never be cost effective.
		*/
		if (node->op == NAME && (flags&UNDER_SWITCH))
			INVALID(objid(node));
	}

	switch (optype(node->op)) {
	case BITYPE:
		check_recurse(node->right, NESTED);
		/* FALLTHRU */
	case UTYPE:
		check_recurse(node->left, NESTED);
	}
}

/* Only two valid assigns are of the form
** 	name1 = icon
** 	name1 = (conv) name2
** where name2 is a short and the type
** of name1 is the same as the type of
** the conv.  Icon must fit in a short.
*/
static void
check_assign(node)
ND1 *node;
{
	ND1 *l = node->left;
	ND1 *r = node->right;
	while (l->op == CONV)
		l = l->left;
	if (l->op == RNODE && r->op == NAME)
		INVALID_ASSIGN(objid(r));
	if (   r->op == NAME
	    && (   (   l->op == NAME
		    && (l->sid != ND_NOSYMBOL)
		    && SY_CLASS(l->sid) == SC_EXTERN)
		|| ispointer(l->type)))
		contingent(objid(l),objid(r));
	if (l->op != NAME)
		return;
	else if (node->op != ASSIGN) {
		if (node->left->type != TY_SHORT)
			INVALID_ASSIGN(objid(l));
		return;
	}
	else if (SHORT_ICON(r))
		return;
	else if (r->op == CONV && r->type == l->type && 
			r->left->type == TY_SHORT) {
		DEBUG(aflags[md].level&2, ("sign-extension into %s(object %d)\n", SY_NAME(l->sid), objid(l)));
		SIGN_EXTEND(objid(l));
		return;
	}
	if (node->left->type != TY_SHORT)
		INVALID_ASSIGN(objid(l));
}

/* Add a contingency between cand1 and cand2.  For
** two candidates to be contingent upon each-other 
** means that a candidate can be optimized only if 
** none of the the candidates it is contingent upon 
** has an invalid assign.
*/
static void
contingent(s1, s2) 
Setid s1, s2;
{
	contingency *cont;

	if (s1 > s2) {
		Setid s = s2;
		s2 = s1;
		s1 = s;
	}
	for (cont=contingency_list; cont; cont=cont->next)
		if (s1 == cont->cand1 && s2 == cont->cand2)
			return;
	DEBUG(aflags[md].level&4, ("contingent(%d,%d)\n", s1, s2));
	cont = Arena_alloc(md_arena, 1, contingency);
	cont->cand1 = s1;
	cont->cand2 = s2;
	cont->next = contingency_list;
	contingency_list = cont;
}

/* If cand1 is contingent upon cand2, and cand2 has an invalid
** assign, mark cand1 as an invalid candidate.
*/
static void
reduce_contingencies()
{
	contingency *cont, *prev = NULL;
	int change = 0;
	do {
		change = 0;
		for (cont=contingency_list; cont; cont=cont->next) {
			int fail = 0;
			if (IS_INVALID_ASSIGN(cont->cand1)
				&& !IS_INVALID_ASSIGN(cont->cand2)){
				INVALID_ASSIGN(cont->cand2);
				fail = 1;
				change = 1;
			}
			if (IS_INVALID_ASSIGN(cont->cand2)
				&& !IS_INVALID_ASSIGN(cont->cand1)){
				INVALID_ASSIGN(cont->cand1);
				fail = 1;
				change = 1;
			}
			if (fail)
				DEBUG(aflags[md].level&4, ("contingency(%d,%d) fails\n",
					cont->cand1, cont->cand2));
		}
	} while (change);
}

/* make_change() assumes there will be very few valid candidates.
** Otherwise, we will need to do something to prevent us from
** always chasing down the trees.
*/
static void
make_change(id)
Setid id;
{
#ifndef NODBG
	if (phase == optimize_phase)
		DEBUG(aflags[md].level, ("mdopt works on setid(%d)\n",id));
#endif
	if (phase == optimize_phase)
		generate_init();
	DEPTH_FIRST(bp)
		Block b = *bp;
		FOR_ALL_ND1_IN_BLOCK(b, flow, node, index)

			if (recurse_change(id, node, b)) {
				DEBUG(aflags[md].level&8,("**** New Tree\n"));
				DEBUG_COND(aflags[md].level&8, tr_e1print(node,"T"));
				node = amigo_op_optim(node);
				new_expr(node, b);
				CGQ_ND1(flow) = node;
			}

		END_FOR_ALL_ND1_IN_BLOCK

	END_DEPTH_FIRST
}

static void
generate_init()
{
	ND1 *assign_node;
	Block block;
	Cgq_index insert_index;
	TEMP_SCOPE_ID scope;
	cgq_t *spot;

	scope = get_global_temp_scope();

	working_temp = make_temp(GET_WORKING_EXPR(), scope);
	SY_TYPE(working_temp->sid) = TY_INT;
	assign_node = t1alloc();
	assign_node->type = TY_UINT;
	assign_node->op = ASSIGN;
	assign_node->c.off = 0;
	assign_node->sid = 0;
	assign_node->flags = FF_SEFF;

	assign_node->right = tr_smicon(0L);
	assign_node->right->type = TY_UINT;

	assign_node->left = t1alloc();
	assign_node->left->type = TY_UINT;
	assign_node->left->op = NAME;
	assign_node->left->c.off = 0;
	assign_node->left->sid = GET_WORKING_TEMP_SID();
	DEBUG(aflags[md].level&2,
		("\trewritten to temp sid = %d\n",GET_WORKING_TEMP_SID()));
	assign_node->left->flags = 0;

	DEPTH_FIRST(bp)

		block = *bp;
		CGQ_FOR_ALL_BETWEEN(flow, index, block->first, block->last)

			if (flow->cgq_op == CGQ_START_SCOPE
			 && flow->cgq_arg.cgq_sid == GET_WORKING_TEMP_SID()) {

				insert_index = index;
				goto found_index;
			}

		CGQ_END_FOR_ALL_BETWEEN

	END_DEPTH_FIRST

	cerror("Start scope not found for sid %d", GET_WORKING_TEMP_SID());

found_index:
	new_expr(assign_node, block);
	spot = amigo_insert(block, insert_index);
	spot->cgq_op = CGQ_EXPR_ND1;
	spot->cgq_arg.cgq_nd1 = assign_node;

	/* The generated code now looks like this:
	**
	**	VAR(int) = 0
	**	...
	**	VAR(short) = value
	**	...
	**
	** We must prevent the apparent, but incorrect,
	** dead store of VAR.
	*/
	assign_node->left->opt->flags &= ~OPTIMIZABLE;
}

static int
recurse_change(id, node, block)
Setid id;
ND1 *node;
Block block;
{
	int changed = 0;
	ND1 *l = node->left;
	ND1 *r = node->right;
	ND1 *conv;

	Setid left_oid, right_oid;

	left_oid  = (l && l->opt && (l->opt->flags & OBJECT)) ? objid(l) : 0;
	right_oid = (r && r->opt && (r->opt->flags & OBJECT)) ? objid(r) : 0;

	switch (node->op) {
	case ASSIGN:
		if (id == left_oid && r->op == CONV) {
			if (phase == cost_phase) {
				current_cost -= 2;
				break;
			}
			DEBUG(aflags[md].level&16,("**** Old Tree\n"));
			DEBUG_COND(aflags[md].level&16, tr_e1print(node,"T"));
			l->type = node->type = r->type = TY_USHORT;
		}
		break;
	case GT: case NGT:
	case GE: case NGE:
	case LT: case NLT:
	case LE: case NLE:
	case LG: case NLG:
	case LGE: case NLGE:
		/* Currently we only permit comparisons with ICONS,
		** shorts, or other candidates.  Since we need to do
		** signed comparisons, change to word-length compares.
		*/
		if (id == left_oid || id == right_oid) {
			ND1 *conv;
			if (phase == cost_phase) {
				current_cost += 1;
				break;
			}
			conv = t1alloc();
			conv->op = CONV;
			conv->type = TY_INT;
			conv->left = l;
			conv->flags = l->flags & FF_SEFF;
			node->left = conv;
			conv = t1alloc();
			conv->op = CONV;
			conv->type = TY_INT;
			conv->left = r;
			conv->flags = r->flags & FF_SEFF;
			node->right = conv;
			l->type = r->type = TY_SHORT;
			new_expr(node, block);
			node->left->opt->flags &= ~OPTIMIZABLE;
			node->right->opt->flags &= ~OPTIMIZABLE;
		}
		break;
	case EQ:
	case NE:
		if (id != left_oid && id != right_oid)
			break;

		/* One of the objects may be marked invalid.  This will
		** happen if that object failed some contingency.  Since
		** that object will not have an invalid assign, we can
		** still do a 16-bit comparison.
		*/
		if (id == left_oid && r->op == NAME && !IS_INVALID(right_oid))
			/* Can do full 32 bit compare */
			break;
		if (id == right_oid && l->op == NAME && !IS_INVALID(left_oid))
			/* Can do full 32 bit compare */
			break;
		if (id == right_oid && NON_NEGATIVE_SHORT_ICON(l))
			/* Can do full 32 bit compare */
			break;
		if (id == left_oid && NON_NEGATIVE_SHORT_ICON(r))
			/* Can do full 32 bit compare */
			break;

		/* We are here if we think we must do a 16 bit compare.
		** cg expects the operands to have type TY_INT, but
		** we will get a cmpw if the NAME has type short with
		** a CONV over it.
		*/
		if (phase == cost_phase) {
			current_cost += 1;
			break;
		}
		DEBUG(aflags[md].level&16,("**** Old Tree\n"));
		DEBUG_COND(aflags[md].level&16, tr_e1print(node,"T"));
		conv = t1alloc();
		conv->op = CONV;
		conv->type = TY_INT;
		conv->flags = 0;
		if (id == right_oid) {
			node->right = conv;
			conv->left = r;
			r->type = TY_SHORT;
			if (l->op == NAME) {
				l->type = TY_SHORT;
				conv = t1alloc();
				conv->op = CONV;
				conv->type = TY_INT;
				conv->flags = 0;
				conv->left = l;
				node->left = conv;
			}
		} else {
			node->left = conv;
			conv->left = l;
			l->type = TY_SHORT;
			if (r->op == NAME) {
				r->type = TY_SHORT;
				conv = t1alloc();
				conv->op = CONV;
				conv->type = TY_INT;
				conv->flags = 0;
				conv->left = r;
				node->right = conv;
			}
		}
		new_expr(node, block);
		node->left->opt->flags &= ~OPTIMIZABLE;
		node->right->opt->flags &= ~OPTIMIZABLE;
		break;
	case NOT:
		/* Can do full 32 bit compare */
		break;
	case AND:
		/* Can do full 32 bit and */
		break;
	}
	switch(optype(node->op)) {
	case BITYPE:
		changed += recurse_change(id, r, block);
		if (phase == optimize_phase && id == right_oid) {
			r->sid = GET_WORKING_TEMP_SID();
			changed++;
		}
		/* FALLTHRU */
	case UTYPE:
		changed += recurse_change(id, l, block);
		if (phase == optimize_phase && id == left_oid) {
			l->sid = GET_WORKING_TEMP_SID();
			changed++;
		}
		break;
	case LTYPE:
		if (node->op == NAME && objid(node) == id)
			working_node = node;	/* Stash in global */
	}
	return changed;
}

static int 
cost_effective(id)
Setid id;
{
	phase = cost_phase;
	current_cost = 0;
	make_change(id);
	if (current_cost < 0)
		return 1;
	DEBUG(aflags[md].level,("mdopt: Not cost effective: obj(%d)\n",id));
	return 0;
}
