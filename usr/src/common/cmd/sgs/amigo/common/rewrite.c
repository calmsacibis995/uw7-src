#ident	"@(#)amigo:common/rewrite.c	1.10"

/* Perform amigo-specific tree rewrites as opposed to tree rewrites
** done in op_amigo_optim().  
*/

#include <amigo.h>
#include <unistd.h>

static void pre_recurse(), post_recurse(), rewrite_mul(),
	rewrite_div();

static ND1 *canonicalize();

void
pre_amigo_rewrite()
{
	CGQ_FOR_ALL(flow, index)

		ND1 *node = HAS_ND1(flow);
		DEBUG_COND(aflags[pre].level,tr_e1print(node,"PRE_B:"));
		if (node)  {
			pre_recurse(node);
			DEBUG_COND(aflags[pre].level,tr_e1print(node,"PRE_A:"));
}

	CGQ_END_FOR_ALL
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

#define SAME_NAME(x) ( (x)->sid == assign_node->left->sid && \
(x)->c.off == assign_node->left->c.off && \
(x)->type == assign_node->left->type )

/* The type check fixes an interaction with the sign extension
optimization. If we do not disallow the rewrite when the types
differ, ushort vs. int, say, cg gets confused. */

#define IS_NAME(x) ((x)->op == NAME && (x)->sid > 0)

void
post_amigo_rewrite()
{
	Cgq_index last_nd1 = CGQ_NULL_INDEX;
	CGQ_FOR_ALL(flow, index)

	ND1 *node, *assign_node, *nll, *nlr, *save_node;
	int replace_flag;
	if(flow->cgq_flags == CGQ_DELETED)
		continue;
	if(flow->cgq_op == CGQ_EXPR_ND2 &&
		flow->cgq_arg.cgq_nd2->op == LABELOP) {
		last_nd1 = CGQ_NULL_INDEX;
		continue;
	}
	node = HAS_ND1(flow);
	if (node) {
		replace_flag = 0;
		switch(node->op) {
		ASSIGN_CASES:
			if(IS_NAME(node->left) &&
				(node->right->op == NAME || node->right->op == ICON)) {
				last_nd1 = index;
				assign_node = node;
			}
			else last_nd1 = CGQ_NULL_INDEX;
			break;

		case CBRANCH:
			if(last_nd1 == CGQ_NULL_INDEX)
				break;
			save_node = node;
			if(node->left->op == NOT)
				node = node->left;
			switch(node->left->op) {
			case EQ: case NE:
			case LT: case NLT:
			case LE: case NLE:
			case GT: case NGT:
			case GE: case NGE:
			case LG: case NLG:
			case LGE: case NLGE:
				nll = node->left->left;
				nlr = node->left->right;
				if(IS_NAME(nll) &&
					SAME_NAME(nll) &&
					(nlr->op == ICON || IS_NAME(nlr) && !SAME_NAME(nlr))
				) {
		DEBUG_COND(aflags[post].level,tr_e1print(node,"POST_B1:"));
					node->left->left = t1_copy(assign_node);
		DEBUG_COND(aflags[post].level,tr_e1print(node,"POST_A:"));
					replace_flag = 1;
					break;
				}
				if(IS_NAME(nlr) &&
					SAME_NAME(nlr) &&
					(nll->op == ICON || IS_NAME(nll) && !SAME_NAME(nll))
				) {
		DEBUG_COND(aflags[post].level,tr_e1print(node,"POST_B2:"));
					node->left->right = t1_copy(assign_node);
		DEBUG_COND(aflags[post].level,tr_e1print(node,"POST_A:"));
					replace_flag = 2;
					break;
				}
				break;
			case NAME:
				if(SAME_NAME(node->left)) {
		DEBUG_COND(aflags[post].level,tr_e1print(node,"POST_B3:"));
					node->left = t1_copy(assign_node); 	
		DEBUG_COND(aflags[post].level,tr_e1print(node,"POST_A:"));
					replace_flag = 3;
				}
				break;
			}
			node = save_node;
			if(replace_flag) {
				(CGQ_ELEM_OF_INDEX(last_nd1)->cgq_flags) = CGQ_DELETED;
				node = op_amigo_optim(node);
					/* Guard against dangling
					** pointer, as op_amigo_optim()
					** may not rewrite in place.
					*/
				CGQ_ND1(flow) = node;
			}
			/* FALLTHROUGH */
		default:
			last_nd1 = CGQ_NULL_INDEX;
		}
		post_recurse(node);
	}

	CGQ_END_FOR_ALL
}

static void
pre_recurse(node)
ND1 *node;
{
	switch (node->op) {
	case DIV:
	case ASG DIV:
		 rewrite_div(node);
	}

	switch(optype(node->op)) {
	case BITYPE:
		pre_recurse(node->right);
		/* FALLTHRU */
	case UTYPE:
		pre_recurse(node->left);
	}
}

static void
post_recurse(node)
ND1 *node;
{
	switch (node->op) {
	case MUL:
	case ASG MUL:
		 rewrite_mul(node);
	}

	switch(optype(node->op)) {
	case BITYPE:
		post_recurse(node->right);
		/* FALLTHRU */
	case UTYPE:
		post_recurse(node->left);
	}
}

#define FCON1(n) (n->op == FCON && FP_CMPX(n->c.fval, flt_1) == 0)

/* The following routine rewrites expressions of the form
** a / b to a * 1/b.  We hope to get a cse on the 1/b.
** This is a kind of strength reduction from division to
** multiplation.  If it doesn't work out, we will fold
** the multiplication later.
**
** To avoid obscurities, we will insist that the operator and operands
** all have the same type.
*/
static void
rewrite_div(node)
ND1 *node;
{
	if(TY_ISFPTYPE(node->type)
	 && node->type == node->left->type
	 && node->type == node->right->type
	 && !FCON1(node->left)
	 && !ieee_fp()
	) {
		ND1 *fcon = t1alloc();
		ND1 *div = t1alloc();

		fcon->op = FCON;
		fcon->type = node->type;
		fcon->c.fval = flt_1;
		fcon->flags = 0;

		div->op = DIV;
		div->type = node->type;
		div->left = fcon;
		div->right = node->right;
		div->flags = 0;

		node->op += MUL - DIV;
		node->right = div;
	}
}

/* The following routine rewrites expressions of the form
** a * 1/b to a/b.  This undoes the wasted work of rewrite_div().
*/
static void
rewrite_mul(node)
ND1 *node;
{
	if (TY_ISFPTYPE(node->type)
	 && node->type == node->left->type
	 && node->type == node->right->type
	 && node->right->op == DIV
	 && FCON1(node->right->left)
	 && !ieee_fp()
	) {
		ND1 *r = node->right->right;
		nfree(node->right->left);
		nfree(node->right);
		node->right = r;
		node->op += DIV - MUL;
	}
}

void
clear_wasopt_flags(ND1 *node)
{
	switch(optype(node->op)) {
	case BITYPE:	
		clear_wasopt_flags(node->right);
		/* FALLTHRU */
	case UTYPE:
		clear_wasopt_flags(node->left);
	}
	node->flags &= ~FF_WASOPT;
}

	/*
	** AMIGO creates trees that op_amigo_optim does not always
	** know how to deal with.  amigo_op_optim() is an
	** interface which tries to remedy this problem.
	*/
ND1 *
amigo_op_optim(node)
ND1 *node;
{
	node = canonicalize(node);
	clear_wasopt_flags(node);
	node = op_amigo_optim(node);
	return node;
}

static int
is_icon(ND1 *node)
{
	switch(optype(node->op)) {
	case BITYPE:	
		return(is_icon(node->left) && is_icon(node->right));
	case UTYPE:
		return 0;
	default:
		return(node->op == ICON && node->sid == ND_NOSYMBOL);
	}
	
}

static ND1 *
canonicalize(node)
ND1 *node;
{
	int op = node->op, nlop = 0;
	switch(optype(op)) {
	case BITYPE:	node->right = canonicalize(node->right);
			/* FALLTHRU */
	case UTYPE:	node->left = canonicalize(node->left);
	}
		/*
		** Put pointers on the left where op_amigo_optim 
		** expects them. Also pull constants up
		** the tree to be folded into array addresses.
		** Op_optim() does not seem to get these. The
		** whole point of this special case code is to
		** get better addressing for things like a[i][j+2],
		** in the form introduced by loop unrolling.
		*/
	if (op == PLUS && TY_ISPTR(node->right->type)) {

		ND1 *tmp = node->left;
		node->left = node->right;
		node->right = tmp;
			
		if(node->left->op == ICON) {
			while(node->right->op == PLUS && node->right->right->op == PLUS && is_icon(node->right->right->right)) {
				ND1 *tmp;
				tmp = node->right->right;
				node->right->right = tmp->left;
				tmp->left = node->left;
				node->left = tmp;
				node->left->type = node->type;
			}

		}	
	}
	if(op == PLUS && !TY_ISPTR(node->left->type)) {
		if(node->right->op != ICON && node->left->op == ICON) {
			ND1 *tmp = node->right;
			node->right = node->left;
			node->left = tmp;
		}
		if(node->right->op == PLUS && node->left->op != PLUS) {
			if( 
			TY_EQTYPE(node->type, node->right->type) &&
			TY_EQTYPE(node->type, node->left->type)
) {
			if(node->right->right->op == ICON) {
				ND1 *tmp = node->right;
				node->right = node->right->right;
				tmp->right = node->left;
				node->left = tmp;
				tmp = node->left->left;
				node->left->left = node->left->right;
				node->left->right = tmp;
				tmp = node->right;
			}
		}}
		else if(node->right->op == LS &&
			node->left->op != PLUS &&
			node->right->left->op == PLUS &&
			node->right->right->op == ICON &&
			node->right->left->right->op == ICON &&
			TY_EQTYPE(node->type, node->right->type) &&
			TY_EQTYPE(node->type, node->left->type) &&
			TY_EQTYPE(node->type, node->right->left->type) &&
			TY_EQTYPE(node->type, node->right->left->left->type)) {
			ND1 *t1;
			node->right->op = PLUS;
			node->right->left->op = LS;
			t1 = node->right->right;
			node->right->right = t1alloc();
			*(node->right->right) = *(node->right->left);
			node->right->right->right = t1;
			node->right->right->left = node->right->left->right;
			node->right->left->right = t1alloc();
			*(node->right->left->right) = *t1;
		}
	}
	node->flags &= ~FF_WASOPT;
	return node;
}
