#ident	"@(#)amigo:common/const.c	1.48"
#include "amigo.h"
#include "costing.h"
#include <unistd.h>

static Expr_set value_set;	/* Exprs whose value is known, actually
				   we only care about objects (NAMES) */

static Boolean PROTO(const_prop,(int context,ND1 * node));
static Boolean PROTO(const_is_cost_effective,(ND1 *));
static void PROTO(const_rewrite,(ND1 *node,int context));

static Boolean changed;
static Arena const_arena;

#define R_CONST(node,cntxt) const_prop(get_context(node,RIGHT,cntxt),\
					node->right)
#define L_CONST(node,cntxt) const_prop(get_context(node,LEFT,cntxt),\
					node->left)
#define R_REWRITE(node,cntxt) const_rewrite(node->right,\
					get_context(node,RIGHT,cntxt))
#define L_REWRITE(node,cntxt) const_rewrite(node->left,\
						get_context(node,LEFT,cntxt))
#define L_COST(node) (const_is_cost_effective(node->left) || node->op == CBRANCH)
#define R_COST(node) (const_is_cost_effective(node->right))

typedef struct c_list {
	SX c_list_symbol;
	SX c_function;
	ND1 c_list_node; /* NOT a pointer */
	struct c_list *next;
} *C_list;

static C_list const_list = 0; /* list of file scope constant scalars */

void
amigo_const(init_symbol, node)
SX init_symbol;
ND1 *node; /* the initial value */

/* Called by in_first() in init.c when acomp sees the initialization
** of a static or extern const.  Push the symbol and a copy of the
** initial value onto the const list.
*/

{
      	if(! file_arena ) file_arena = arena_init();

	DEBUG(aflags[cf].level&4,("init_symbol:%s\n",SY_NAME(init_symbol)));
	DEBUG_COND(aflags[cf].level&4, dprint1(node));
	if(node->op == ICON) {
		C_list new;
		new = Arena_alloc(file_arena,1,struct c_list);
		if (SY_CLASS(init_symbol) == SC_STATIC && SY_LEVEL(init_symbol) != SL_EXTERN)
			new->c_function = cg_getcurfunc();
		else
			new->c_function = (SX)0;
		new->c_list_symbol = init_symbol;
		new->c_list_node = *node; /* Might not need if we knew acomp
						would not blow the node away */
		new->next = const_list;
		const_list = new;
	}
}

static Expr_set init_const_set();

	/*
	** Check node for an assign on the left.  If so return node.
	** Otherwise, if left is a relop and right hand operand of this
	** relop has no side effects, go to the left and  keep looking.
	** So the precondition is a side effect on the left and no
	** side effect on the right.
	*/
static ND1 *
find_parent_of_assign(node)
ND1 * node;
{
	switch(node->left->op) {
	case ASSIGN:
	case ASG PLUS:
	case ASG MINUS:
	case ASG MUL:
	case ASG DIV:
	case ASG MOD:
	case ASG OR:
	case ASG AND:
	case ASG ER:
	case ASG LS:
	case ASG RS:
			/*
			** Optimization will do any side effect on the
			** left of the assign twice, so we have to rule
			** that case out.
			*/
		if(CONTAINS_SIDE_EFFECTS(node->left->left))
			return NULL;
		else
			return node;
		break;
	case EQ:
	case NE:
	case GT: case NGT:
	case GE: case NGE:
	case LT: case NLT:
	case LE: case NLE:
	case LG: case NLG:
	case LGE: case NLGE:
	case UGT: case UNGT:
	case UGE: case UNGE:
	case ULT: case UNLT:
	case ULE: case UNLE:
	case ULG: case UNLG:
	case ULGE: case UNLGE:
		if(CONTAINS_SIDE_EFFECTS(node->left->right))
			return NULL;
		/* FALLTHRU */
	
	case NOT:
		return(find_parent_of_assign(node->left));
		break;
	default:
		return NULL;
		break;
	}
}

static void
expose_cbranch_assigns()
{
	cgq_t *item;

	CGQ_FOR_ALL(flow,index)

	ND1 *node;
	if(flow->cgq_op == CGQ_EXPR_ND1 
		&& flow->cgq_arg.cgq_nd1->op == CBRANCH ){ 

		node = flow->cgq_arg.cgq_nd1;

		if(CONTAINS_SIDE_EFFECTS(node->left)) {
			node = find_parent_of_assign(node);

			if(node) {
START_OPT
				DEBUG(aflags[cf].level&16, ("BEFORE EXPOSE:\n\n"));
				DEBUG_COND(aflags[cf].level&16, cgq_print_tree(flow,index,3));
					/* Make the new item the CBRANCH */
				item = cg_q_insert(index);	
				item->cgq_op = flow->cgq_op;
				item->cgq_func = flow->cgq_func;
				item->cgq_ln = flow->cgq_ln; 
				item->cgq_dbln = flow->cgq_dbln; 
				item->cgq_arg = flow->cgq_arg;

					/* Convert the old CBRANCH
					** to the assign.
					**
					** First (bug fix), get flow again, in case
					** memory has been reorganized.
					*/	
				flow = CGQ_ELEM_OF_INDEX(index);	
				flow->cgq_arg.cgq_nd1 = tr_copy(node->left);
					/*
					** Remove the assignment from the
					** CBRANCH
					*/
				if(node->left->left->op == CONV && node->left->type == node->left->left->left->type) {
					
					/* Fixes a BUG */
					node->left = node->left->left->left;
				}
				else
				node->left = node->left->left;
				DEBUG(aflags[cf].level&16, ("AFTER EXPOSE:\n\n"));
				DEBUG_COND(aflags[cf].level&16, cgq_print_tree(flow,index,3));
				DEBUG_COND(aflags[cf].level&16, cgq_print_tree(item,CGQ_INDEX_OF_ELEM(item),3));
STOP_OPT
			}
		}
	}
	CGQ_END_FOR_ALL
}
static int distribute_changed = 0;

	/* Try to use the distributive law to cut down unnecessary
	** computation: (A && B || A && C) --> A && (B || C)
	** If B is false we only compute A once instead of twice.
	** This is cheaper than doing a cse.
	*/
static void
distribute(ND1 *node)
{
	switch (optype(node->op)) {
		case LTYPE: return;
		case UTYPE:
			distribute(node->left);
			return;
		case BITYPE:
			distribute(node->left);
			distribute(node->right);
			if (node->op == ANDAND || node->op == OROR) {
				int op1,op2;
				ND1 *temp;
				op1 = node->op;
				if (node->left->op == node->right->op
					&& (node->left->op == OROR || node->left->op == ANDAND)
					&& OPTABLE(node->left->left) && OPTABLE(node->right->left)
						&& node->left->left->opt->setid ==
							node->right->left->opt->setid) {
					/* rewrite*/
					temp = node->left->left;
					op2 = node->left->op;
					nfree(node->right->left);
					node->right->left = node->left->right;
					nfree(node->left);
					node->left = node->left->left;
					node->op = op2;
					node->right->op = op1;
					distribute_changed = 1;
				}
			}
	}
}

void
local_const_folding()
{
#define NL node->left
#define NR node->right
	int init_done = 1;
	Expr_set const_set; /* File scope const variables whose value is known */
	const_arena = arena_init();
	value_set = Expr_set_alloc(const_arena);
	const_set = init_const_set();
	bv_assign(value_set, const_set);	
	DEBUG(aflags[cf].level&4,("const_set: %s\n", bv_sprint(const_set)));

	expose_cbranch_assigns();
	CGQ_FOR_ALL(flow,index)

	ND1 *node;
	Boolean rewrote_assign, in_switch;
	rewrote_assign = false;	
	in_switch = false;

	if (flow->cgq_op == CGQ_EXPR_ND2) {
		switch (flow->cgq_arg.cgq_nd2->op) {
		case LABELOP:
		case SWEND: 
		case JUMP:
			if (!init_done) {
				/* init value set to file scope consts */
				init_done = 1;
				bv_assign(value_set, const_set);	
				}
			break;
		case SWBEG: 	/*
				** Currently does not arise: SWBEG all occur
				** under flow->cgq_op == CGQ_FIX_SWBEG.
				** Go figure!
				*/
			in_switch = true;
			break;
		}
	}
	else if(flow->cgq_op == CGQ_FIX_SWBEG)
		in_switch = true;
	if (!(node = HAS_ND1(flow)))
		continue;
	if (flow->cgq_arg.cgq_nd1->op == CBRANCH) {
		node = flow->cgq_arg.cgq_nd1;
		node->flags &= ~FF_WASOPT;
		distribute_changed = 0;
		distribute(node->left);
		if (distribute_changed) new_expr(node,0);
	}
	init_done = 0;

	DEBUG(aflags[cf].level&4,("value_set: %s\n", bv_sprint(value_set)));

		/*
		** Try to rewrite all funny assigns to plain jane
		** assigns ( i++ ==> i = i+1;, etc. ) except if
		** we have post incr or post decr in switch.
		*/
	if(!in_switch || (node->op != INCR && node->op != DECR))
#ifndef NODBG
		rewrote_assign = rewrite_assigns(node,aflags[cf].level&2);
#else
		rewrote_assign = rewrite_assigns(node);
#endif
	if(rewrote_assign) new_expr(node,0);

	changed = false;
	if (NL && CONTAINS_SIDE_EFFECTS(NL))
		get_recursive_kills(value_set,NL,REMOVE);
	if (NR && node->op != CBRANCH && CONTAINS_SIDE_EFFECTS(NR))
		get_recursive_kills(value_set,NR,REMOVE);
	if(NL && L_CONST(node,0) && L_COST(node)) {
START_OPT
		L_REWRITE(node,0);
STOP_OPT
	}
	if(NR && node->op != CBRANCH &&  R_CONST(node,0) && R_COST(node)) {
START_OPT
		R_REWRITE(node,0);
STOP_OPT
	}

	DEBUG(aflags[cf].level&2,("value_set: %s\n",
		bv_sprint(value_set)));

	if(changed) {
			/* prime global for error reports */
		extern int elineno; 
		elineno = flow->cgq_ln; 

		DEBUG(aflags[cf].level&2,("Old expr before op_amigo_optim:\n"));
		DEBUG_COND(aflags[cf].level&2, dprint1(node));
		*node = *op_amigo_optim(node);
		if (node->op == JUMP)	/* only case in which
					   op_amigo_optim returns an ND2
					*/
			flow->cgq_op = CGQ_EXPR_ND2;
		else
			new_expr(node,0);
		DEBUG(aflags[cf].level&1,("New expr from const folding (line=%d)\n",flow->cgq_ln));
		DEBUG_COND(aflags[cf].level&1, dprint1(node));
	}

	switch(node->op) {
	
	case ASSIGN:
		if(NL->op == NAME && EXCEPT_OPT(NL) && (NR->op == ICON ||
		  EXCEPT_OPT(NR) && bv_belongs(NR->opt->setid,value_set)) ) {

			ND1 *val;

			if(NR->op == ICON) 
				val = NR;
			else if(NR->op == NAME)
				val = NR->opt->object->value;
			else break;	/* for now we are assuming
					** rhs is an ICON or a NAME node,
					** otherwise we will need to 
					** rewrite it at some point 
					** -- PSP:
					*/
			
			if(! NL->opt->object->value)
				NL->opt->object->value =
					Arena_alloc(global_arena,1,ND1);
			*(NL->opt->object->value) = *val; /* copy ! */
			if (val->op == ICON)
				tr_truncate(&NL->opt->object->value->c.ival, NL->type);
			get_kills(value_set,node,REMOVE);
			bv_set_bit(NL->opt->setid,value_set);
			DEBUG(aflags[cf].level&2, ("Adding object %s value %s to value set: %s\n",
				SY_NAME(NL->sid),
				num_tosdec(&NL->opt->object->value->c.ival),
				bv_sprint(value_set)));
		}
		else
			get_kills(value_set,node,REMOVE);
		break;
	case JUMP:
		break;
	case RETURN:
	case CBRANCH:
			if (!init_done) {
				/* init value set to file scope consts */
				init_done = 1;
				bv_assign(value_set, const_set);	
				}
			break;
	default:
		get_kills(value_set,node,REMOVE);
		break;	
	}

	if( (!changed) && rewrote_assign) {
		*node = *op_amigo_optim(node);
		if (node->op == JUMP)	/* only case in which
					   op_amigo_optim returns an ND2
					*/
			flow->cgq_op = CGQ_EXPR_ND2;
		else
			new_expr(node,0);
		DEBUG(aflags[cf].level&2,("ASSIGN after reconversion:\n"));
		DEBUG_COND(aflags[cf].level&2, print_expr(node->opt));
	}

	CGQ_END_FOR_ALL

	arena_term(const_arena);
}

static Expr_set
init_const_set()
{
	C_list p;
	Expr_set const_set = Expr_set_alloc(const_arena);
	bv_init(false,const_set);
	for(p = const_list; p; p = p->next) {
		Expr exp;
		SX sid = p->c_list_symbol;
		if (p->c_function != 0 && p->c_function != cg_getcurfunc()) 
			continue;
		exp = sid_to_expr(p->c_list_symbol);	
		if(exp && OPTABLE_EXPR(exp) && exp->object) {
			bv_set_bit(exp->setid,const_set);
			exp->object->value = &(p->c_list_node);
			DEBUG(aflags[cf].level&4, 
				("Adding object %s value %s to global value set\n",
				SY_NAME(p->c_list_symbol),
				num_tosdec(&exp->object->value->c.ival)));
			
		}
#ifndef NODBG
		else {
			DEBUG_COND(aflags[cf].level&1,
				Amigo_fatal(gettxt(":570","Bad const list?")));
		}
#endif
	}
	return const_set;
}



extern ND1 * op_amigo_optim(); /* in acomp/common/optim.c */

	/* check (recursively) if the value is known at compile time */
static Boolean
const_prop(cntxt,node)
int cntxt;	/* context of this node */
ND1 *node;
{
	Boolean rflag, lflag;
	assert(node);
	switch(node->op) {
	case NAME:
		DEBUG(aflags[cf].level&4,("Check NAME in value_set: %s\n",
			bv_sprint(value_set)));
		if(EXCEPT_OPT(node) && bv_belongs(node->opt->setid,value_set)) 
			return true;
		else return false;
	case ICON:
		return true;
	default:
		if(node->right && node->left) {
			rflag = R_CONST(node,cntxt);
			lflag = L_CONST(node,cntxt);
			if(rflag && lflag) {
				if(EXCEPT_OPT(node))
				return true;
			}
			if(rflag && R_COST(node)) {
START_OPT
				R_REWRITE(node,cntxt);
STOP_OPT
			}
			if(lflag && L_COST(node)) {
START_OPT
				L_REWRITE(node,cntxt);
STOP_OPT
			}
		}
		else if(node->left) {
			if(L_CONST(node,cntxt)) {
				if(EXCEPT_OPT(node)) return true;
				else if(L_COST(node)) {
START_OPT
					L_REWRITE(node,cntxt);
STOP_OPT
				}
			}
		}
		else if(node->right) {
			if(R_CONST(node,cntxt)) {
				if(EXCEPT_OPT(node)) return true;
				else if(R_COST(node)) {  
START_OPT
					R_REWRITE(node,cntxt);
STOP_OPT
				}
			}
		}
		return false;
	} /* switch */
}

static Boolean
const_is_cost_effective(node)
ND1 * node;
{
	DEBUG_UNCOND(nodes_costed_count++;)
	if(( node->opt->flags & EXPENSIVE ) == 0) return false;
	return COST(node) > cf_threshold;
}


	/* Walk the tree, replacing NAMEs with known values */
static void
const_rewrite(node,context)
ND1 *node;
int context;
{
	if(node->op == NAME && IS_RVAL(context)) {
		/* Replace NAME node with ICON node */
		DEBUG(aflags[cf].level&2,(
			"Replace NAME %s with ICON val %s\n",
			SY_NAME(node->sid),
			num_tosdec(&node->opt->object->value->c.ival)));
 		node->op = ICON;
 		node->flags = 0;
 		node->c.ival = node->opt->object->value->c.ival;
 		node->sid = node->opt->object->value->sid;
 		/* leave original type */
		changed = true;
	}
	else {
		if(node->left) L_REWRITE(node,context);
		if(node->right) R_REWRITE(node,context);
	}
}
