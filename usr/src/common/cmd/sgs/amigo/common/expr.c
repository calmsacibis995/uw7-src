#ident	"@(#)amigo:common/expr.c	1.43.1.61"
#include "amigo.h" 
#include <memory.h>
#include "costing.h"
#include <unistd.h>

/* estimate the number of expressions contained in each statement.
   used in allocating hash-headers
*/
#define EXPRS_PER_STATE 5

struct Expr_info **expr_info_array;

static struct Expr_info **chains;
static int bucket_count;
static struct Expr_info *free_list=0;

char *dots = (char *)0;
char *varargs = ""; /* different from any symbol string */

static int object_count;	/* number of objects in function */
int expr_count;			/* number of optable exprs in function */
static int occur_count;		/* number of nodes in function */
int expr_array_size;		/* maximum optable exprs allow for function */
int object_array_size;		/* maximum optable objects allow for function */

Bit_vector BVZERO;

DEBUG_UNCOND(static void hash_debug();)
DEBUG_UNCOND(static int total_expr_count;)


	/* head of linked list of Expr_info's */
static struct Expr_info *expr_last;


	/* head of linked list Object_infos */
static struct Object_info *object_last;

	/* kills file statics, escaped locals, and externs */
static struct Object_info *generic_deref;
int generic_deref_is_killed;
Object_set generic_deref_obj_kills;

	/* kills all of the above plus any floating point */
static struct Object_info *generic_call;
int generic_call_is_killed;
Object_set generic_call_obj_kills;

static void make_escape();


static void find_kills();
static void build_kills();
static void print_objects();
static struct Object_info * new_object_info();
static struct Object_info * new_name_info();
static struct Object_info * new_call_info();

static int init_chains() ;

void 
new_expr(expr,block) ND1 *expr; Block block;
{
	struct Expr_info *curr;
	struct Expr_info *old_last = expr_last;
	struct Object_list *obj_kills;
	Loop loop;

	(void) amigo_hash(expr);

	/* kills for new expressions, add new expressions to expr_info_array*/
	for (curr=expr_last; curr != old_last; curr=curr->next) {
		/* new_expr_info guarantees that expr_info_array will
		   not overflow
		*/
		if ((curr->flags&OPTIMIZABLE)) {
			find_kills(curr);
			expr_info_array[curr->setid] = curr;
		}
	}

	/* get objects killed by expr */
	obj_kills = node_kills(expr, 0, 0);

	if (!block)
		return;

	/* update object kills for block */
	block->kills = object_list_union_eq(block->kills, obj_kills, 0);

	/* update object kills for loops containing this block */
	for (loop = block->loop; loop; loop = loop->parent)
		loop->kills = object_list_union_eq(loop->kills, obj_kills, 0);
}



static int
extra(slots)
int slots;
{
	if (slots <= 16)
		return 32;
	if (slots < 100)
		return slots * 2;
	if (slots < 1000)
		return slots * 3 / 2;
	return slots * 6 / 5;
}

Bit_vector
bvclone(a, bv)
Arena a;
Bit_vector bv;
{
	Bit_vector x;

	if (!bv_set_card(bv))
		return BVZERO;
	x = Expr_set_alloc(a);
	bv_assign(x, bv);
	return x;
}

	/* create hash data structures, count label references and label
	  definition and drive the hashing of every ND1 in function to
	  an Expr_info *
	*/
void
hash_exprs() 
{
	int count = 0;	/* ND1 count */
	struct Expr_info *curr;
	Object obj;
	DEBUG_UNCOND(int label_count = 0;)

	if (dots == 0) {
		/* Look up name strings for "..." and alternate varargs name. */
		dots = st_lookup("...");
#ifdef	VA_ALIST
		varargs = st_lookup(VA_ALIST);
#endif
	}

	generic_call_is_killed = generic_deref_is_killed = 0;

	expr_count = object_count = 0;
	free_list = 0;
	DEBUG_UNCOND(total_expr_count = 0;)

	DEBUG_COND(aflags[fs].level,occur_count = 0);
	expr_last = 0;
	object_last=0;

	init_branch_count();
	init_max_label();
	init_min_label();

	/* get an estimate of number of hash buckets based on top level trees */
	/* also compute number of branches and largest label */
	CGQ_FOR_ALL(root,index)
		if (root->cgq_op==CGQ_EXPR_ND2 || root->cgq_op==CGQ_EXPR_ND1) {
			int lbl;
			switch (root->cgq_arg.cgq_nd2->op) {
			case LABELOP:
				lbl = root->cgq_arg.cgq_nd2->c.label;
				update_max_label(lbl);
				update_min_label(lbl);
				DEBUG_UNCOND(++label_count;)
				break;
      			case SWEND:
				{ ND2 *p = root->cgq_arg.cgq_nd2->left;

					/* Walk through its SWCASE list */
					while (p->op == SWCASE) {
						update_branch_count();
						p = p->left;
					}
				}
				/* check for default case */
      				if (root->cgq_arg.cgq_nd2->sid < 0) break;
      				/* FALLTHROUGH */
			case JUMP:
				/* may be set wrong if op_amigo_optim converts
				   CBRANCH to a JUMP
				*/
				root->cgq_op = CGQ_EXPR_ND2;
      				/* FALLTHROUGH */
			case CBRANCH:
				update_branch_count();
				break;
			} 
			if (HAS_ND1(root))
				++count;
		}
	CGQ_END_FOR_ALL
#ifndef NODBG
	if(get_max_label() - get_min_label() > 3*label_count) {
		DEBUG(aflags[jj].level&1,("Labels not dense. "));
		DEBUG(aflags[jj].level&1,
		("max_label: %d, min_label: %d, label_count: %d\n",
		    get_max_label(),get_min_label(),label_count));
	}
#endif
	bucket_count = init_chains(count*EXPRS_PER_STATE);
	expr_array_size = 0;

		/* make the generic deref */
	generic_deref = new_object_info(0);

		/* make the generic call */
	generic_call = new_object_info(0);

	CGQ_FOR_ALL(root,index)
		ND1 *node;
		if ((node = HAS_ND1(root)) != 0)
			(void) amigo_hash(node);
	CGQ_END_FOR_ALL

	DEBUG_COND(aflags[h].level, hash_debug(bucket_count));

	/* build expr_info array */
	expr_info_array = Arena_alloc(GLOBAL,expr_array_size=
	(expr_count >= object_count ? extra(expr_count) : extra(object_count)),
		struct Expr_info *);

	/* RMA temporary */
	object_array_size = expr_array_size;

	BVZERO = Expr_set_alloc(GLOBAL);
	bv_init(false, BVZERO);

	for (curr=expr_last; curr; curr=curr->next) {
		if (curr->flags&OPTIMIZABLE) 
			expr_info_array[curr->setid] = curr;
	}

	generic_deref_obj_kills = Object_set_alloc(GLOBAL);
	bv_init(false, generic_deref_obj_kills);

	generic_call_obj_kills = Object_set_alloc(GLOBAL);
	bv_init(false, generic_call_obj_kills);

	build_kills();
	for (obj = object_last;  obj ; obj = obj->next) {
		SX sid = obj->fe_numb;
		if (sid <= 0)
			continue;
		if (SY_CLASS(sid) == SC_AUTO || SY_CLASS(sid) == SC_PARAM)
			continue;
		if (IS_VALID_CANDIDATE(sid)) {
			obj->flags |= VALID_CANDIDATE;
			if (REGCLASS(sid) >= 0)
				obj->flags |= CAN_BE_IN_REGISTER;
		}
	}
}

struct Expr_info *
alloc_expr_info() {
	struct Expr_info *new;
	if (free_list) {
		new = free_list;
		free_list = free_list->next;
	}
	else
		new = Arena_alloc(GLOBAL, 1, struct Expr_info );
	return new;
}

Boolean
is_generic_call(obj)
Object obj;
{
	return(obj == generic_call);
}

Boolean
is_generic_deref(obj)
Object obj;
{
	return(obj == generic_deref);
}

Object
get_generic_call()
{
	return(generic_call);
}

Object
get_generic_deref()
{
	return(generic_deref);
}

static void
free_expr_info(expr)
struct Expr_info *expr;
{
	expr->next = free_list;
	free_list = expr;
}

static  void new_expr_info();
static unsigned hash_to_int();
static int node_equal();

/* hashes the expr n, if new expression then an expr_info is created */
struct Expr_info *
amigo_hash(n) ND1 *n; 
{
	struct Expr_info **header;
	struct Expr_info *chain, *curr;

	curr = alloc_expr_info();

	/* normalize n */
	n->flags &= ~FF_WASOPT;
	if (optype(n->op) == UTYPE)
		n->right = 0;
	else if (optype(n->op) != BITYPE)       /* leaf node */
		n->right = n->left = 0;

	header = hash_to_int(n,curr) % bucket_count + chains;

	DEBUG_COND(aflags[fs].level,++occur_count);

	/* check on hash chain */
	for (chain= *header; chain; chain=chain->chain) {

		/* found on chain */
		if (node_equal(chain,curr)) {
			n->opt = chain;
			free_expr_info(curr);
			return chain;
		}
	}

	n->opt = curr;			/* point ND1 to Expr_info */
	new_expr_info(n);

	/* splice into the next list */
	curr->next = expr_last;
	expr_last = curr;

	/* splice into hash chains */
	curr->chain = *header;
	*header = curr;

	return curr;
}

/* copies the relevant information from input to node. If left or right
   not used for a particular node they are set to 0. Hashes words
   n1, n2 .. nk to an integer by: n1*65599**1 + n2*65599**2 + .. nk*65599**nk
*/
static unsigned
hash_to_int(input, expr)
register ND1 *input;
register Expr expr;
{
#define K (unsigned) 65599
#define K1 K*K
#define K2 K*K1
#define K3 K*K2
#define K4 K*K3
#define output (expr->node)
#define SET(member) output.member = input->member
#define SET_LEFT output.left = (ND1 *) amigo_hash(input->left)
#define SET_RIGHT output.right = (ND1 *) amigo_hash(input->right)

	register unsigned int sum;
	TWORD type;

	/*  dummy not optimizable */
	static const struct Expr_info dummy = {0, 0, TVOID, {ICON, TY_INT}};

	SET(op);
	SET(type);
	SET(flags);	/* always referenced by cg_p2tree */

	/* expr->type is the pass2 type which is used for hashing. Eg. If 
	   the long type is identical to the int type on a machine,
	   the pass1 types will differ, but the pass2 types will be
	   identical
	*/
	expr->type = cg_machine_type(input->type);

	sum = output.op*K + expr->type*K1;
	switch  (input->op) {
	case CBRANCH:
		SET_LEFT; output.right = 0;
		break;
	case FLD:
		SET(c.off); SET_LEFT; output.right = 0;
		sum +=  K2*(unsigned)output.left + K3*output.c.off;
		break;
	case STCALL:
	case STASG:
		SET(sttype); SET_LEFT; SET_RIGHT;
		sum +=  K2*(unsigned)output.left + K3*(unsigned)output.right +
			(unsigned)K4*output.sttype;
		break;
	case UNARY STCALL:
	case STARG:
		SET(sttype); SET_LEFT; output.right = 0;
		sum +=  K2*(unsigned)output.left + (unsigned)K3*output.sttype;
		break;

	/*
	** Commute operands if necessary to force the expr number
	** of the left operand to be smaller than the expr number
	** of right operand.  In this way a+b and b+a will share
	** the same Expr_info.
	*/
	case PLUS:
	case MUL:
	case AND:
	case OR:
		{
		ND1 * left = (ND1 *) amigo_hash(input->left);
		ND1 * right = (ND1 *) amigo_hash(input->right);
		if (((Expr)left)->setid > ((Expr)right)->setid) {
			output.left = right;
			output.right = left;
		}
		else {
			output.left = left;
			output.right = right;
		}

		sum +=  K2*(unsigned)output.left + K3*(unsigned)output.right;
		}
		break;
	case FCON:
		/* assumes c.fval is at least integer aligned and at least
		   as long as 2 ints
		*/
		SET(c.fval); output.left = output.right = 0;
		sum += K2*(*(unsigned *)&output.c.fval) +
		       K3*(*((unsigned *)&output.c.fval + 1));
		break;
	case ICON:
		/* assumes that useful integer info can be found in
		   the first integer of c.ival
		*/
		SET(c.ival); SET(sid); output.left = output.right = 0;
		sum += K2 * (*(unsigned *)&output.c.ival) + K3 * output.sid;
		break;
	case NAME:
		SET(c.off); SET(sid); output.left = output.right = 0;
		sum += K2 * output.c.off + K3 * output.sid;
		break;
	case STRING:
		SET(c.size); SET(sid); SET(string);
		output.left = output.right = 0;
		sum += K2 * output.c.size + K3 * output.sid + 
			K4*(unsigned)output.string;
		break;
	case EH_LABEL:
	case RETURN:
	case RNODE:
	case TYPE:
		output.left = output.right = 0;
		/* only op and type */
		break;
	case CSE:
		output.left = output.right = 0;
		SET(c.size);
		break;
	default:
		SET_LEFT;
		sum += K2 * (unsigned) output.left;
		if (input->right) {
			SET_RIGHT;
			sum += K3 * (unsigned) output.right;
		}
		else
			output.right = 0;
#ifndef NODBG
		if (!(output.left || output.right))
			Amigo_fatal(gettxt(":571","unidentified leaf output in hash"));
#endif

	}
	return sum;
}

static int
node_equal(e1, e2)
register Expr e1, e2;
{
#define COMP(member) (e1->node.member == e2->node.member)

	/* expr->type is the pass2 type which is used for hashing. Eg. If 
	   the long type is identical to the int type on a machine,
	   the pass1 types will differ, but the pass2 types will be
	   identical
	*/
	if (!(COMP(op) && e1->type == e2->type))
		return 0;
	switch  (e1->node.op) {
	case CSE:
		return COMP(c.size);
	case FLD:
		return COMP(flags) && COMP(c.off) && COMP(left);
	case STCALL:
	case STASG:
		return COMP(flags) && COMP(sttype) && COMP(left) && COMP(right);
	case UNARY STCALL:
	case STARG:
		return COMP(flags) && COMP(sttype) && COMP(left);
	case FCON:
		return !FP_CMPX(e1->node.c.fval, e2->node.c.fval);
	case ICON:
		if (!COMP(sid))
			return 0;
		return !num_ucompare(&e1->node.c.ival, &e2->node.c.ival);
	case NAME:
		return COMP(c.off) && COMP(sid);
	case STRING:
		return COMP(c.size) && COMP(sid) && COMP(string);
		break;
	case RETURN:
	case RNODE:
	case TYPE:
		return 1;
	default:
		return COMP(flags) && COMP(left) && COMP(right);
	}
}

/* initialize the number of hash headers based on the expr_count, make
   this number a prime
*/
static int
init_chains(count) 
{
	if (count <= 503)
		count = 503;
	else if (count <= 1009)
		count = 1009;
	else if (count <= 2003)
		count = 2003;
	else if (count <= 4001)
		count = 4001;
	else if (count <= 8009)
		count = 8009;
	else if (count <= 16001)
		count = 16001;
	else if (count <= 32003)
		count = 32003;
	else
		count = 64007;

	chains = Arena_alloc(GLOBAL, count, struct Expr_info *);
	memset(chains, 0, count * sizeof(struct Expr_info *));
	return count;
}


#define OK_POINTER(node,pointer,offset) \
TY_ISPTR((node)->type) && TY_ISPTR((pointer).type) && TY_ISINTTYPE((offset).type)


/* this routine fills a new expr_info, and determines which object addresses
   have escaped
*/
#define ADDCON_TO_OBJ(addcon) ( addcon ? addcon : generic_deref)
static void
new_expr_info(node)
ND1 *node;
{
	struct Expr_info *e = node->opt;
	struct Object_info  *o, *o1;
	int op_ok = 1;
	e->cost = -1;	/* Not costed */
	e->addcon = 0;
	e->flags = 0;
	INIT_SCOPE(e->temp_scope);
	e->flags = 0;
	e->object = 0;

	/* set object, partially set flags */
	switch (node->op) {
	case BMOVE:
	case BMOVEO:
		e->flags = SIDE_EFFECT;
		e->object = ADDCON_TO_OBJ(LEFT_INFO(RIGHT_INFO(e))->addcon);
		break;
	ASSIGN_CASES:
		if (LEFT_INFO(e)->flags&OBJECT) {
			e->flags = SIDE_EFFECT;
			e->object = LEFT_INFO(e)->object;
		}
		else {
			Amigo_fatal(gettxt(":572","lhs of assign not an object"));
		}
		if (node->op == ASG DIV || node->op == ASG MOD) {
		    if (node->right->op != ICON) 
			e->flags |= ARITH_EXCEPTION;
		    if (TY_ISFPTYPE(node->type))
			e->flags |= HAS_FP_DIVIDE;
		    else
			e->flags |= HAS_DIVIDE;
		
		}
		if (node->op != INCR && node->op != DECR)
			make_escape(RIGHT_INFO(e)->addcon);
		if (e->object == generic_deref)
			generic_deref_is_killed = 1;
		break;
	case DOT:
	case CONV: case FLD:
		if (LEFT_INFO(e)->flags&OBJECT) {
			e->flags = OBJECT;
			e->object = LEFT_INFO(e)->object;
		}
		break;
	case STASG:
		e->flags = SIDE_EFFECT;
		e->object = ADDCON_TO_OBJ(LEFT_INFO(e)->addcon);
		break;
	case CALL:
	case STCALL:
	case UNARY CALL:
	case UNARY STCALL:
	case INCALL:
	case UNARY INCALL:
		e->flags = HAS_CALL;
		e->object = new_call_info(node);
		e->flags |= SIDE_EFFECT;
		generic_call_is_killed = generic_deref_is_killed = 1;
		break;
	case STAR:
		e->object = ADDCON_TO_OBJ(LEFT_INFO(e)->addcon);
		e->flags = OBJECT|POINTER_EXCEPTION;
		if (e->object == generic_deref && version == V_CI4_1)
			e->flags |= CONTAINS_VOLATILE;
		else if (LEFT_INFO(e)->flags & 
			(CONTAINS_VOLATILE_ADDRESS|CONTAINS_VOLATILE)) {
			e->flags |= CONTAINS_VOLATILE;
      		}
		break;
	case CSE:
	case RNODE:
		op_ok = 0;
		/*FALLTHROUGH*/
	case STRING:
		e->flags = OBJECT;
		e->object = new_object_info(0);
		break;
	case MOD:
	case DIV:
		if (node->right->op != ICON)
			e->flags = ARITH_EXCEPTION;
		make_escape(LEFT_INFO(e)->addcon);
		make_escape(RIGHT_INFO(e)->addcon);
		if (TY_ISFPTYPE(node->type))
			e->flags |= HAS_FP_DIVIDE;
		else
			e->flags |= HAS_DIVIDE;
		break;
	case NAME:
		e->object = new_name_info(node);
		e->flags = OBJECT;
		if (node->sid == ND_NOSYMBOL) {
			if (version == V_CI4_1)
				e->flags = EXCEPTABLE|CONTAINS_VOLATILE|OBJECT;
			else
				e->flags = EXCEPTABLE|OBJECT;
		}
		if (node->sid <= 0)
			break;

		if (TY_ISVOLATILE(SY_TYPE(node->sid)))
			e->flags |= CONTAINS_VOLATILE;
		if (version == V_CI4_1 &&
			!TY_ISCONST(SY_TYPE(node->sid)) &&
			(SY_CLASS(node->sid) == SC_STATIC 
				||
			SY_CLASS(node->sid) == SC_EXTERN))

			e->flags |= CONTAINS_VOLATILE;

		/* if NAME has an offset (c.off), check that the
		   displacement plus the size of NAME fits into the fits
		   into the symbol type of name (note that type of NAME
		   node may be a scalar in the symbol associated with name
		   can be a non-scalar)
		*/
		if ( (node->c.off && TY_ISSCALAR(node->type) &&
			node->c.off*TY_SIZE(TY_CHAR) + TY_SIZE(node->type) > 
			TY_SIZE(SY_TYPE(node->sid))))

			e->flags = (OBJECT|POINTER_EXCEPTION);
		break;

	/* next four routines set addcon if the node is a pointer expression
	   that meets certain criteria
	*/
	case PLUS:
#ifndef NODBG
		if(aflags[ep].level && RIGHT_INFO(e)->flags & OBJECT &&
			( node->left->op == ICON ||
				node->left->op == FCON)) {
			fprintf(stderr,"FOUND OBJECT_PLUS_CONST RIGHT\n");
		};
#endif
		if(LEFT_INFO(e)->flags & OBJECT &&
			( node->right->op == ICON ||
				node->right->op == FCON)) {
			DEBUG_UNCOND(if (aflags[ep].xflag))
			e->flags |= OBJECT_PLUS_CONST;
		}
		o = LEFT_INFO(e)->addcon;
		o1 = RIGHT_INFO(e)->addcon;
		if (o && o1) {
			make_escape(o);
			make_escape(o1);
		}
		else if (!o && !o1)
			break;
		else if (o && OK_POINTER(node,LEFT_INFO(e)->node,
		    RIGHT_INFO(e)->node))
			e->addcon = o;
		else if (o1 && OK_POINTER(node,RIGHT_INFO(e)->node,
		    LEFT_INFO(e)->node))
			e->addcon = o1;
		break;
	case MINUS:
#ifndef NODBG
	
		if(aflags[ep].level && RIGHT_INFO(e)->flags & OBJECT &&
			node->left->op == ICON ) {
			fprintf(stderr,"FOUND OBJECT_PLUS_CONST RIGHT\n");
		};
#endif
		if(LEFT_INFO(e)->flags & OBJECT &&
			node->right->op == ICON) {
			DEBUG_UNCOND(if (aflags[ep].xflag))
			e->flags |= OBJECT_PLUS_CONST;
		}
		o = LEFT_INFO(e)->addcon;
		o1 = RIGHT_INFO(e)->addcon;
		if (!o && !o1)
			break;
		/* difference of two pointers valid if both point within
		   same composite object
		*/
		if (o && o1 && o != o1) {
			make_escape(o);
			make_escape(o1);
		}
		else if (o && OK_POINTER(node,LEFT_INFO(e)->node,
		    RIGHT_INFO(e)->node))
			e->addcon = o;
		else if (o1 && OK_POINTER(node,RIGHT_INFO(e)->node,
		    LEFT_INFO(e)->node))
			e->addcon = o1;
		break;
	case UNARY AND:
		if (LEFT_INFO(e)->flags & OBJECT) {
			e->addcon = (LEFT_INFO(e)->object);
                        if (LEFT_INFO(e)->flags & CONTAINS_VOLATILE)
                                e->flags |= CONTAINS_VOLATILE_ADDRESS;
		}
		else {
			DEBUG_UNCOND(dprint1(node));
			cerror(gettxt(":573","object must be under unary and"));
		}
		break;
	case ICON:
		if (node->sid > 0) {
			e->addcon  = SID_TO_OBJ(node->sid);
			/* if sid > 0 ICON is address of static 
			** or extern and may be volatile.
			*/
			if ((version == V_CI4_1 ||
			     TY_ISVOLATILE(SY_TYPE(node->sid))))
				e->flags |= CONTAINS_VOLATILE_ADDRESS;
		}
		else
			e->addcon = new_object_info(node->sid);
		break;

	case LET:
	case COMOP:
		e->addcon = RIGHT_INFO(e)->addcon;
		break;
	case FUNARG:
	case REGARG:
		if (TY_ISPTR(node->type))
			e->flags = POINTER_ARG;
		/* FALLTHRU */
	case CBRANCH:
	case STARG:
	case CM:
	case COLON:
	case RETURN:
		op_ok = 0;
		/* FALLTHRU */
	default:
		if (LEFT_INFO(e))
			make_escape(LEFT_INFO(e)->addcon);
		if (RIGHT_INFO(e))
			make_escape(RIGHT_INFO(e)->addcon);
	}
	if (LEFT_INFO(e)) {
		e->flags |= LEFT_INFO(e)->flags &
			(HAS_CALL| FLOATING_POINT|HAS_DIVIDE|HAS_FP_DIVIDE|

			/* for UNARY AND or STAR, VOLATILE info	 propgated
                           previously
			*/
			(node->op != (UNARY AND) && node->op != STAR ?
			CONTAINS_VOLATILE_ADDRESS|CONTAINS_VOLATILE : 0)

			|EXCEPTABLE|POINTER_EXCEPTION|ARITH_EXCEPTION
			|CONTAINS_SIDE|EXPENSIVE|POINTER_ARG|DEREF_KILLS);
	}
	if (RIGHT_INFO(e)) {
		e->flags |= RIGHT_INFO(e)->flags &
			(HAS_CALL|HAS_DIVIDE|HAS_FP_DIVIDE|
			FLOATING_POINT|CONTAINS_VOLATILE_ADDRESS|
			CONTAINS_VOLATILE|CONTAINS_SIDE|EXPENSIVE|
			EXCEPTABLE|POINTER_EXCEPTION|ARITH_EXCEPTION|
			POINTER_ARG|DEREF_KILLS);
	}
	if (callop(node->op))
		e->flags &= ~POINTER_ARG;
	if (TY_ISFPTYPE(node->type))
		e->flags |= (FLOATING_POINT);

	if (e->flags & FLOATING_POINT) {
		e->flags |= ARITH_EXCEPTION;
	}

	if(TY_ISPTR(node->type) && TY_ISVOLATILE(TY_DECREF(node->type)))
		e->flags |= CONTAINS_VOLATILE_ADDRESS;

	if (node->flags & FF_ISVOL)
		e->flags |= CONTAINS_VOLATILE;

	if (e->flags & SIDE_EFFECT)
		e->flags |= CONTAINS_SIDE;

	if (optype(node->op) == BITYPE)
		e->flags |= EXPENSIVE;

	DEBUG_UNCOND(e->setid = 0);

	/* conditions for optimizable expression */
	if (!(e->flags & CONTAINS_SIDE) && !(e->flags & CONTAINS_VOLATILE) && 

		TY_ISSCALAR(node->type) && op_ok) {

		/*
		** if called fron new_expr, expr_set_sizes have been fixed,
		** in this case make sure there is enough space in the
		** expr_array
		*/
		if (EXPR_SET_SIZE == 0 || expr_count+1 < expr_array_size) {
			e->flags |= OPTIMIZABLE;
			++expr_count;
			e->setid = expr_count;
			if (!(e->flags & EXPENSIVE) && 
			      COST(node) > LOAD_COST(node->type)+exp_threshold
			)
				e->flags |= EXPENSIVE;
		}
	}
	DEBUG_UNCOND(++total_expr_count);
}


/* create a canonical NAME node for a symbol id and then hash it (to
   see if canonical NAME for this index has been created and
   return the associated object
*/
struct Expr_info *
sid_to_expr(index)
SX index;
{
	ND1 name;
	SX sameas;
	memset((myVOID *) &name, 0, sizeof(ND1));
	
	if (index > 0) {
		/* same name may have different entries in symbol table
		   find the root occurrence
		*/
		for (sameas = SY_SAMEAS(index); sameas; 
			sameas = SY_SAMEAS(sameas))
			index = sameas;

		 name.type = SY_TYPE(index);
	}
	else
		name.type = TY_INT;

	name.op = NAME;
	name.sid = (int) index;

	return amigo_hash(&name);
}

/* mark the object as escaped */
void 
make_escape(object)
struct Object_info *object;
{
	SX index;
	if (!object || (index = object->fe_numb) <= 0)
		return;

	if (SY_CLASS(index) == SC_AUTO || SY_CLASS(index) == SC_PARAM)
		object->flags |= ESCAPED_AUTO;
	else if (SY_CLASS(index) == SC_STATIC && SY_LEVEL(index) != SL_EXTERN)
		object->flags |= ESCAPED_STATIC;
}

static struct Object_info *
new_object_info(fe_numb)
int fe_numb;
{
	
	struct Object_info * o =
	Arena_alloc(GLOBAL, 1, struct Object_info);
	o->value = (ND1 *)0;
	o->flags = 0;
	
	o->setid = ++object_count;
	o->fe_numb = fe_numb;
	o->scope = 0;
	o->scope_usage = 0;
	DEBUG_UNCOND(o->kills = 0);
	if (EXPR_SET_SIZE !=0) {
		o->kills=bv_alloc(EXPR_SET_SIZE, GLOBAL);
		bv_init(false,o->kills);
	}
	o->next = object_last;
	object_last = o;
	if(fe_numb > 0)
		SY_FLAGS(fe_numb) |= SY_AMIGO_OBJECT;
	return o;
}


/* called to create Object_info's for a new NAME node */
static struct Object_info *
new_name_info(node)
ND1 *node;
{
	struct Object_info *o;

	if (node->sid == ND_NOSYMBOL)
		return generic_deref;

	/* new_name_info will be called only once for a NAME node with
	   a particular lval which is < 0, since these are created by
	   acomp for special purposes, such as holding the initializer
	   for an aggregage object
	*/
	else if (node->sid < 0)
		return new_object_info(node->sid);

	/* new_name_info may be called more than once for different NAME nodes
	   with the same lval (>0). If the NAME node is not canonical (checked
	   below, we call SID_TO_OBJ, which returns the same object as
	   a canonical node. Thus, all NAME nodes with the same (lval>0) map
	   to the same object
	*/
	else if ( (node->type != (int)SY_TYPE((SX)node->sid) /* painted */
		|| node->flags			/* caste to volatile, eg. */
		|| SY_SAMEAS((SX)node->sid)	/* aliased */
		|| node->c.off != 0))		/* offset */
		return SID_TO_OBJ((SX) node->sid);/* base object */

	else {
		SX sid = (SX)node->sid;
		o = new_object_info(sid);
		o->regclass = REGCLASS(sid);
		if (SY_LEVEL(sid) == SL_EXTERN)
			o->flags = EXTERN;
		else if((SY_CLASS(sid) == SC_AUTO || SY_CLASS(sid) == SC_PARAM)
		      && IS_VALID_CANDIDATE(sid)) {
			o->flags |= VALID_CANDIDATE;
			if (o->regclass >= 0)
				o->flags |= CAN_BE_IN_REGISTER;
		}
		return o;
	}
}

static struct Object_info *
new_call_info(node)
ND1 *node;
{
	node = node;
	return generic_call;
}

#ifndef NODBG
void 
print_exprs(level)
int level;
{
	struct Expr_info *curr;

	for (curr=expr_last; curr; curr = curr->next) {
		if(level == 8 ) break;
		if(level!=1 || curr->flags&OPTIMIZABLE)
			print_expr(curr);

	}
	print_objects(level);
	
	fflush(stderr);
}

static nNODE *
dformat(e,curr_free,last_free)
struct Expr_info *e; nNODE *curr_free; nNODE *last_free;
{
	nNODE *next=curr_free+1;
	ND1 *curr = (ND1 *)curr_free;

	*curr = e->node;
	curr->opt = e;
	curr_free->_node_no = e->setid;

	if (curr_free > last_free)
		return 0;
	if (CHILD(e,left)) {
		curr->left = (ND1 *)next;
		next = dformat(CHILD(e,left),next,last_free);
		if (!next)
			return 0;
	}
	if (CHILD(e,right)) {
		curr->right = (ND1 *)next;
		next = dformat(CHILD(e,right),next,last_free);
		if (!next)
			return 0;
	}
	return next;
}

static void
print_object(o,level)
struct Object_info *o;
int level;
{
	DPRINTF("object:%d",o->setid);
	DEBUG(aflags[exp].level&4,("(0x%x)", o));
 	DPRINTF(", flags=(%s) fe_numb=%d name=%s ",
		(o->flags&CAN_BE_IN_REGISTER) ? "CAN_BE_IN_REGISTER" :
		(o->flags&VALID_CANDIDATE) ? "VALID_CANDIDATE" :
		(o->flags&ESCAPED_STATIC) ? "ESCAPED_STATIC" :
		(o->flags&ESCAPED_AUTO) ? "ESCAPED_AUTO" :
		(o->flags&EXTERN) ? "EXTERN" : "",
		o->fe_numb, 
		(o->fe_numb ? OBJ_TO_NAME(o) :
			(is_generic_call(o) ? "gen_call":
			(is_generic_deref(o) ? "gen_deref" : "???")))
	);

	if(o->scope)
		scope_debug(o->scope);
	if (o->kills) {
		DPRINTF("kills=");
		bv_print(o->kills);
	}
}

static void print_objects(level)
int level;
{
	struct Object_info *o;
	for (o = object_last; o; o = o->next) {
		print_object(o,level);
	}
}

void
print_expr(e)
Expr e;
{
#define MAX_EXPR 200
	nNODE n[MAX_EXPR];

	DPRINTF(((e->flags&IS_COPY)?"assign:%d":"expr:%d"), e->setid);
	DEBUG(aflags[exp].level&4,("(0x%x)",e));
	DPRINTF(",flags=(%s%s%s%s%s%s%s%s%s%s%s%s%s) ",
		(e->flags&OBJECT) ? "OBJECT " : "",
		(e->flags&CONTAINS_VOLATILE) ? "VOLATILE " : "",
		(e->flags&SIDE_EFFECT) ? "SIDE_EFFECT " : "",
		(e->flags&OPTIMIZABLE) ? "OPTIMIZABLE " : "",
		(e->flags&FLOATING_POINT) ? "FLOATING_POINT " : "",
		(e->flags&HAS_DIVIDE) ? "HAS_DIVIDE " : "",
		(e->flags&HAS_FP_DIVIDE) ? "HAS_FP_DIVIDE " : "",
		(e->flags&EXCEPTABLE) ? "EXCEPT " : "",
		(e->flags&POINTER_EXCEPTION) ? "POINTER_EXCEPTION " : "",
		(e->flags&ARITH_EXCEPTION) ? "ARITH_EXCEPTION " : "",
		(e->flags&POINTER_ARG) ? "POINTER_ARG " : "",
		(e->flags&DEREF_KILLS) ? "DEREF_KILLS " : "",
		(e->flags&EXPENSIVE) ? "EXPENSIVE " : "");
	if (e->object)
		DPRINTF("object_id=%d ",e->object->setid);

	if(dformat(e,n,&n[MAX_EXPR-1])) {
		DPRINTF("cost=%d\n",
			((e->flags&OPTIMIZABLE)?cost((ND1 *)n):e->cost));
		dprint1((ND1 *)n);
	}
	else
		DPRINTF("\nexpression too big\n");
	DPRINTF("\n");
	fflush(stderr);
}
#endif

int
get_expr_count() { return expr_count; }

struct Expr_info *
get_expr_last() { return expr_last; }	

int
get_object_count() { return object_count; }

struct Object_info *
get_object_last() { return object_last; }

void
set_object_last(obj) struct Object_info *obj; { object_last = obj; }

int
get_occur_count() { return occur_count; }

	/*
	** Remove obj from the object_list.  Caller must pass the
	** the prior object, if any.
	*/
void
remove_object(prior, obj)
Object prior, obj;
{
	if(prior) {
		prior->next = obj->next;
	}
	else {
		object_last = obj->next;	
	}
}

/* sets the kills for all objects */
static void
build_kills()
{
	struct Expr_info *curr;
	struct Object_info *o;

	/* initialize the escaped stuff */
	/* what arena do we put the escaped things in, phase_arena */

	for (o = object_last; o; o = o->next) {
		o->kills = Expr_set_alloc(GLOBAL);
		bv_init(false,o->kills);
	}

	for (curr=expr_last; curr; curr=curr->next) {

		/* sets escaped and kills */
		if (curr->flags&OPTIMIZABLE) {
			find_kills(curr);
		}
	}

	/* if compiled -Xt then any expression containing an escaped
	   object or the generic dereference is considered volatile
	*/

	/* RMA at this point don't need non-opt expressions in list */
	if (version == V_CI4_1) {
		for (o = object_last; o; o = o->next) {
			if (o->flags&ESCAPED_AUTO) {
				BV_FOR(o->kills, e)
					SETID_TO_EXPR(e)->flags &= ~OPTIMIZABLE;
					SETID_TO_EXPR(e)->flags |= CONTAINS_VOLATILE;
				END_BV_FOR
			}
		}
		return;
	}

}


/* set expr as killed by every escaped object */
static void
add_escaped_kills(expr) unsigned expr;
{
	struct Object_info *o;
	for (o = object_last; o; o = o->next) {
		if (o->flags&ESCAPED) {
			bv_set_bit(expr,o->kills);
		}
	}
}

/* if subexpr is an object then indicate that subexpr kills expr. If called
   from new_expr then any object that is aliased to subexpr also kills expr. 
   The only aliasing recognized by Amigo is each escaped object is aliased
   to the generic deref and the generic call
*/
static void
find_object_kills(expr, subexpr,in_star)
struct Expr_info *expr;
struct Expr_info *subexpr;
int in_star;

{
 	/* if subexpr is an object in expr that is not under a '&' then
           subexpr kills expr.
	*/
	if (subexpr->flags&OBJECT) {

			/* kills expr */
		bv_set_bit(expr->setid, subexpr->object->kills);
			/*
			** Killing the generic_call should also kill
			** generic_deref.  E.g., *p is killed by foo().
			*/
		if (subexpr->object == generic_deref) {
			bv_set_bit(expr->setid, generic_call->kills);
			bv_set_bit(subexpr->object->setid, generic_call_obj_kills);
		}

		if (subexpr->object->flags&ESCAPED) {

			/* generic deref kills expr */
			bv_set_bit(expr->setid, generic_deref->kills);
			bv_set_bit(subexpr->object->setid, generic_deref_obj_kills);

			/* generic call kills expr */
			bv_set_bit(expr->setid, generic_call->kills);
			bv_set_bit(subexpr->object->setid, generic_call_obj_kills);
		}
		else if (subexpr->object->fe_numb > 0 &&
			SY_CLASS(subexpr->object->fe_numb) == SC_STATIC) {
			bv_set_bit(expr->setid, generic_call->kills);
			bv_set_bit(subexpr->object->setid, generic_call_obj_kills);
		}
		else if (subexpr->object == generic_deref)
			/* escaped objects kill expr */
			add_escaped_kills(expr->setid);
	}

        /* RMA change this test when calls can be optimizable exprs */
	if (subexpr->node.op == UNARY AND && !in_star)
		return;

	else if (subexpr->node.op == STAR)
		in_star = 1;

	if (LEFT_INFO(subexpr)) {
		find_object_kills(expr,LEFT_INFO(subexpr),in_star);
	}
	if (RIGHT_INFO(subexpr)) {
		find_object_kills(expr, RIGHT_INFO(subexpr),in_star);
	}
}

static void
find_kills(expr)
struct Expr_info *expr;
{
	if (expr->flags&FLOATING_POINT)
		/* any call kills floating point expr */
		bv_set_bit(expr->setid, generic_call->kills);

	if (expr->flags&DEREF_KILLS) {
		bv_set_bit(expr->setid, generic_call->kills);
		bv_set_bit(expr->setid, generic_deref->kills);
	}

		/* a kill of any object in expr kills expr */
	find_object_kills(expr,expr,0);
}


void
get_recursive_kills(live, node, accum)
Expr_set live;
ND1 *node;
int accum;	/* if accum then the side-effects are accumulated into live
		   else the side-effects are subtracted from live
		*/
{
	if (! (node && CONTAINS_SIDE_EFFECTS(node)))
		return;
	get_recursive_kills(live,node->left, accum);
	get_recursive_kills(live,node->right, accum);
	get_kills(live,node, accum);
}

void
get_kills(live,node, accum)
Expr_set live;
ND1 *node;
int accum;
{
	unsigned int exceptions = EXCEPTABLE;
	if (node->opt->flags & SIDE_EFFECT) {
		if (accum == ACCUMULATE)
			bv_or_eq(live,node->opt->object->kills);
		else
			bv_minus_eq(live,node->opt->object->kills);
	}
	/* If we are obeying the letter of the law, a floating point
	** exception may be well-defined and the user may rely on it.
	*/
	if (ieee_fp())
		exceptions |= ARITH_EXCEPTION;
	if (node->opt->flags & exceptions) {
		if (accum == ACCUMULATE)
			bv_or_eq(live,generic_deref->kills);
		else
			bv_minus_eq(live,generic_deref->kills);
	}
}
int
get_context(node, side, context) 
ND1 *node; int side; int context;
{
	switch (node->op) {
	case FLD:
		break;
	case UNARY AND:
		context |= IS_LVAL;
		break;
	case ANDAND:
	case OROR:
		if (side == RIGHT) {
			context &= ~UNDER_OP_EQ;
			context |= IS_COND;
		}
		break;
	case COLON:
		context &= ~UNDER_OP_EQ;
		context |= IS_COND;
		break;
	ASSIGN_CASES:
		if (side == LEFT)
			context |= IS_LVAL;
		else
			context &= ~IS_LVAL;
		break;
	/* if CONV is an LVAL (see tr_islval in acomp:common/trees.c)
	   then child of CONV is also an LVAL
	*/
	case CONV:
		break;
	case PLUS:
	case MINUS:
	/* Need to add more cases PSP */
	{
		ND1 *sibling;
		if(side == LEFT)
			sibling = node->right;
		else
			sibling = node->left;
		if(sibling->op == ICON || sibling->op == FCON)
			context |= SIBLING_IS_CONST;
	}
		/* FALLTHRU */
	default:
		context &= ~IS_LVAL;
	}
	return context;
}

#ifndef NODBG
static void 
hash_debug (buckets) int buckets;
{
	
	register struct Expr_info **header;
	int min=expr_count;
	int max=0;
	int zero_count = 0;
	struct Expr_info **stop=chains+buckets;
	float expect;
	float chi_square = 0;
	if (buckets > total_expr_count) {
		zero_count = total_expr_count - buckets;
		buckets = total_expr_count;
	}
	expect = (float)total_expr_count / (float) buckets;
	for (header = chains; header < stop; ++header) {
		float diff;
		register struct Expr_info *curr_expr;
		register int chain_count = 0;

		for (curr_expr = *header; curr_expr; curr_expr=curr_expr->chain)
			++chain_count;

		if (chain_count > max)
			max = chain_count;
		else if (chain_count < min && chain_count > 0)
			min=chain_count;

		if (chain_count) {
			diff = expect - chain_count;
			chi_square += diff*diff;
		}
		else
			++zero_count;
	}
	if (zero_count) {
		chi_square += (float)zero_count*expect*expect;
		min = 0;
	}
	fprintf(stderr, "exprs=%d buckets=%d max=%d min=%d chi square=%f\n", 
		total_expr_count, buckets, max, min, chi_square/buckets);

}
	/* Given object id, return the object */
Object
debug_object(sid)
Setid sid;
{
	struct Object_info *obj;
	for (obj = object_last; obj; obj = obj->next) {
		if(sid == obj->setid) return obj;
	}
	return 0;
}
#endif
