#ident	"@(#)cg:i386/local2.c	1.136"
/*	local2.c - machine dependent grunge for back end
 *	i386 CG
 *              Intel iAPX386
 */

# include "mfile2.h"
# include "string.h"
# include <unistd.h>
# include "manifest.h"


#ifdef NODBG
#ifndef NDEBUG
#define NDEBUG /* for assert */
#endif
#endif
#include <assert.h>

typedef	long	OFFSZ;		/* should be same as that defined in
				 * mfile1.h.  This became necessary
				 * to redefine with the change from
				 * 5.3.4 to 5.3.12
				 */
# define istnode(p) ((p)->in.op==REG && istreg((p)->tn.sid))

extern int zflag;       /* true if we print comments in assembly output */
extern int edebug;      /* true if we print expression debug info       */
extern int fpdebug;
static void blockmove();
static void starput();
extern int canbereg();
extern char * cg_escape_leading_dollar(char *);

int vol_opnd = 0, special_opnd = 0;	/* Attributes volatile, pos_offset auto operand */
int cur_opnd = 1;	/* current operand  */

#define CLEAN()     {vol_opnd = 0; special_opnd = 0; cur_opnd = 1;}

extern RST regstused;	/* List of registers used for structure moves */

/* The outreg array maps a number for each value of rnames[] for
 * sdb debugging purposes.  We use PUSHA order for cpu registers.
 */
int outreg[] = {
    0,      /* %eax */
    2,      /* %ecx */
    1,      /* %edx */
    9,      /* %st0 */
    3,      /* %ebx */
    6,      /* %esi */
    7,      /* %edi */
    5,      /* %ebp */
    4       /* %esp */
};

const char *const rnames[] = {  /* normal access register names */
    "%eax", "%edx", "%ecx", "%st(0)",		/*scratch registers*/
    "%ebx", "%esi", "%edi",			/*user registers*/
    "%ebp", "%esp"                              /*other registers*/
};

static const char *const rsnames[] = { /* register names for shorts */
    "%ax",  "%dx",  "%cx",  "ERROR",
    "%bx",  "%si",  "%di",
    "%bp",  "%sp"
};

static const char *const rcnames[] = { /* register names for chars */
    "%al",  "%dl",  "%cl",  "ERROR",
    "%bl",  "ERROR","ERROR",
    "ERROR","ERROR"
};

static const char *const ccbranches[] = { /* integers */
	"je",	"jne",		/* EQ, NE */
	"jle",	"jg",		/* LE, NLE */
	"jl",	"jge",		/* LT, NLT */
	"jge",	"jl",		/* GE, NGE */
	"jg",	"jle",		/* GT, NGT */
	"jne",	"je",		/* LG, NLG */
	"jmp",	"/ !<>=",	/* LGE, NLGE */
	"jbe",	"ja",		/* ULE, UNLE */
	"jb",	"jae",		/* ULT, UNLT */
	"jae",	"jb",		/* UGE, UNGE */
	"ja",	"jbe",		/* UGT, UNGT */
	"jne",	"je",		/* ULG, UNLG */
	"jmp",	"/ !<>="	/* ULGE, UNLGE */
};

static const int mwrelop[] = { /* low word relop for long longs */
	EQ,	NE,	/* EQ, NE */
	ULE,	UNLE,	/* LE, NLE */
	ULT,	UNLT,	/* LT, NLT */
	UGE,	UNGE,	/* GE, NGE */
	UGT,	UNGT,	/* GT, NGT */
	LG,	NLG,	/* LG, NLG */
	LGE,	NLGE,	/* LGE, NLGE */
	ULE,	UNLE,	/* ULE, UNLE */
	ULT,	UNLT,	/* ULT, UNLT */
	UGE,	UNGE,	/* UGE, UNGE */
	UGT,	UNGT,	/* UGT, UNGT */
	ULG,	UNLG,	/* ULG, UNLG */
	ULGE,	UNLGE	/* ULGE, UNLGE */
};

static const int noieeerel[] = { /* preferable relop when !ieee_fp() */
	NLG,	LG,	/* EQ, NE */
	NGT,	GT,	/* LE, NLE */
	NGE,	GE,	/* LT, NLT */
	GE,	NGE,
	GT,	NGT,
	LG,	NLG,
	LGE,	NLGE,
	UNGT,	UGT,	/* ULE, UNLE */
	UNGE,	UGE,	/* ULT, UNLT */
	UGE,	UNGE,
	UGT,	UNGT,
	ULG,	UNLG,
	ULGE,	UNLGE
};

static const char *const fpccbranches[] = { /* floating */
	"andb\t$0x44,%ah\n\tjnp",	/* EQ */
	"andb\t$0x44,%ah\n\tjp",	/* NE */
	"andb\t$0x41,%ah\n\tjnp",	/* LE */
	"andb\t$0x41,%ah\n\tjp",	/* NLE */
	"andb\t$0x05,%ah\n\tjnp",	/* LT */
	"andb\t$0x05,%ah\n\tjp",	/* NLT */
	"sahf\n\tjnc",	"sahf\n\tjc",	/* GE, NGE */
	"sahf\n\tja",	"sahf\n\tjna",	/* GT, NGT */
	"sahf\n\tjnz",	"sahf\n\tjz",	/* LG, NLG */
	"sahf\n\tjnp",	"sahf\n\tjp",	/* LGE, NLGE */
	"andb\t$0x41,%ah\n\tjnp",	/* ULE */
	"andb\t$0x41,%ah\n\tjp",	/* UNLE */
	"andb\t$0x05,%ah\n\tjnp",	/* ULT */
	"andb\t$0x05,%ah\n\tjp",	/* UNLT */
	"sahf\n\tjnc",	"sahf\n\tjc",	/* UGE, UNGE */
	"sahf\n\tja",	"sahf\n\tjna",	/* UGT, UNGT */
	"sahf\n\tjnz",	"sahf\n\tjz",	/* ULG, UNLG */
	"sahf\n\tjnp",	"sahf\n\tjp",	/* ULGE, UNLGE */
};

static const char *const fp6ccbranches[] = { /* floating for P6 */
	"shrl\t$8,%eax\n\tandl\t$0x44,%eax\n\tjnp",	/* EQ */
	"shrl\t$8,%eax\n\tandl\t$0x44,%eax\n\tjp",	/* NE */
	"shrl\t$8,%eax\n\tandl\t$0x41,%eax\n\tjnp",	/* LE */
	"shrl\t$8,%eax\n\tandl\t$0x41,%eax\n\tjp",	/* NLE */
	"shrl\t$8,%eax\n\tandl\t$0x05,%eax\n\tjnp",	/* LT */
	"shrl\t$8,%eax\n\tandl\t$0x05,%eax\n\tjp",	/* NLT */
	"sahf\n\tjnc",	"sahf\n\tjc",			/* GE, NGE */
	"sahf\n\tja",	"sahf\n\tjna",			/* GT, NGT */
	"sahf\n\tjnz",	"sahf\n\tjz",			/* LG, NLG */
	"sahf\n\tjnp",	"sahf\n\tjp",			/* LGE, NLGE */
	"shrl\t$8,%eax\n\tandl\t$0x41,%eax\n\tjnp",	/* ULE */
	"shrl\t$8,%eax\n\tandl\t$0x41,%eax\n\tjp",	/* UNLE */
	"shrl\t$8,%eax\n\tandl\t$0x05,%eax\n\tjnp",	/* ULT */
	"shrl\t$8,%eax\n\tandl\t$0x05,%eax\n\tjp",	/* UNLT */
	"sahf\n\tjnc",	"sahf\n\tjc",			/* UGE, UNGE */
	"sahf\n\tja",	"sahf\n\tjna",			/* UGT, UNGT */
	"sahf\n\tjnz",	"sahf\n\tjz",			/* ULG, UNLG */
	"sahf\n\tjnp",	"sahf\n\tjp",			/* ULGE, UNLGE */
};

/* Floating Point Stack Simulation Routines 
** 
** The stack grows in such a way that fpstack[fpsp] is always
** the current top of stack.  fpsp == -1 means the stack is empty.
*/

extern void fp_init(), fp_cleanup(), fp_push();
static void fp_fill(), fp_pop(), fp_save(), fp_xch(), fp_clear(); 
static int fp_onstack(), fp_keep(), fp_widen();
static char fp_suffix();

static struct fpstype {
	TWORD	fpop;
	OFFSZ	fpoff1;
	OFFSZ	fpoff2;
	char    *name;
	int	isqnode;
	TWORD	ftype;
}	fpstack[8];	/* current contents of FP stack */

#define NOOP	((TWORD)(~0))
#ifndef NODBG
extern char *typename();
#define TYPENAME(x) ( x == NOOP ? "NOOP" : typename(x))
#endif
#define BAD_OFFSET	0x80000000	/* used to signify empty fpoff2 */

static int	fpsp = -1;	/* current FP stack pointer */
#ifdef FIXED_FRAME
static OFFSET	cg_arg_offset, max_arg_byte_count, regarg_target;
static int move_args_flag; /* This flag is set/cleared in initial call to codgen */

void
set_max_arg_byte_count(i) OFFSET i; { max_arg_byte_count = i; }

OFFSET
get_max_arg_byte_count() { return max_arg_byte_count; }

#define update_max_arg_byte_count() \
{ if (cg_arg_offset > max_arg_byte_count) max_arg_byte_count = cg_arg_offset; }
#else
#define update_max_arg_byte_count()
#endif

#define ST(n)	(fpsp-n)

/* Needs to be changed to check if we have a string of qnodes */
/*
#define FP_ISEMPTY() (fpsp == -1||(fpsp==0 && (fpstack[ST(0)].isqnode == 1)))
*/
#define FP_ISEMPTY fp_isempty

static int
fp_isempty()
{
	int x = fpsp;

	for( x=fpsp; x >= 0; x--){
		if ( fpstack[ST(x)].isqnode == 0 ) return 0;
	}

	return 1;
}
#define FP_ISFULL()  (fpsp == 7)

/* Lower is true if ST(i) is lower on stack than j */
#define LOWER(i, j) (i < j)

#define CHKUFLOW	\
	if (fpsp < -1) cerror(gettxt(":694","floating point stack underflow"))
#define CHKOFLOW	\
	if (fpsp > 7) cerror(gettxt(":695","floating point stack overflow"))

/* Print out the stack */
#ifndef NODBG
#define FP_DEBUG(s, l, loc)	if (fpdebug) fp_print(s, l, loc);
#define FP_STIN(l)	if (zflag) fp_stin(l)
#else
#define FP_DEBUG(s, l, loc)
#define FP_STIN(l)
#endif

#ifndef NODBG
static void
fp_stin(l)
int l;
{
	if (l) fprintf(outfile, "\t/ %d", l);
}

static void
fp_print(s, l, loc)
char *s;
int l;
int loc;
{
	int i;
	fprintf(outfile, "\n/ %s\n", s);
	if (l) fprintf(outfile, "/ stin line = %d\n", l);
	if (loc) fprintf(outfile, "/ temp location = %d\n", loc);
	fprintf(outfile, "/ loc\tfpop\tname\tfpoff1\tfpoff2\tisqnode\tftype\n");
	for (i=fpsp; i>=0; i--) {
		if (fpstack[i].fpoff2 == BAD_OFFSET)
			fprintf(outfile, "/ st(%d)\t%s\t%s\t%d\tEMPTY\t%d\t%s\n",
				ST(i),
				fpstack[i].fpop == NOOP ? "NOOP"
					: opst[fpstack[i].fpop],
				fpstack[i].fpop == NAME ? fpstack[i].name : "",
				fpstack[i].fpoff1,
				fpstack[i].isqnode,
				TYPENAME(fpstack[i].ftype)
			);
		else
			fprintf(outfile, "/ st(%d)\t%s\t%s\t%d\t%d\t%d\t%s\n",
				ST(i),
				fpstack[i].fpop == NOOP ? "NOOP"
					: opst[fpstack[i].fpop],
				fpstack[i].fpop == NAME ? fpstack[i].name : "",
				fpstack[i].fpoff1,
				fpstack[i].fpoff2,
				fpstack[i].isqnode,
				TYPENAME(fpstack[i].ftype)
			);
	}
	putc('\n', outfile);
}
#endif

/* Init the floating point stack (At routine start) */

void
fp_init()
{
	static void fp_clear();
	register int i;

	for(i = 0; i < 8; i++) 
		fp_clear(i);
	fpsp = -1;
}

static NODE tnode;

static void
fp_remove()
/* Remove the item that is on the top of the stack. */
{
	/* Check to see if we have seen only one qnode.  If this
	** is so, this entry is from the first choice in question-
	** colon operation and is not really on the stack.  This
	** is a little black magic...
	*/
	if (fpstack[ST(0)].isqnode == 1)
		return;
	if (fpstack[ST(0)].fpop != TEMP) {
		fprintf(outfile, "\tfstp\t%%st(0)\n");
		fp_pop();
		return;
	}
	/* Top of the stack is a TEMP.  Check if we are going
	** to reference its value again.
	*/
	tnode.in.op = TEMP; 
	tnode.in.type = fpstack[ST(0)].ftype;
	tnode.in.c.off = fpstack[ST(0)].fpoff1;

	if (references(&tnode) > 0) {
		fprintf(outfile,"\tfstp%c\t" ,fp_suffix(tnode.in.type));
		printout_stack_addr(tnode.in.c.off);
		putc('\n',outfile);
	}
	else {
		fprintf(outfile, "\tfstp\t%%st(0)\n");
	}
	fp_pop();
	return;
}

void
fp_cleanup()
{
#ifndef NODBG
	if (fpdebug && !FP_ISEMPTY())
		fprintf(outfile, "/\tcleaning fp stack\n");
#endif
	while (!FP_ISEMPTY())
		fp_remove();
}

static void
fp_fill(i, op, name, off1, off2, isqnode, type)
int i;
TWORD op;
char *name;
OFFSZ off1, off2;
int isqnode;
TWORD type;
{
	register struct fpstype *fp = &fpstack[i];
	fp->fpop = op;
	fp->name = name;
	fp->fpoff1 = off1;
	fp->fpoff2 = off2;
	fp->isqnode = isqnode;
	fp->ftype = type;
}

void
fp_push(p)
NODE *p;
{
	TWORD ty = p->tn.type;

	fpsp++;
	CHKOFLOW;
	if (!(ty & (TFLOAT|TDOUBLE|TLDOUBLE)))
		ty = 0; /* can't really tell what it has become */
	fp_fill(fpsp,p->tn.op,p->tn.name,p->tn.c.off,BAD_OFFSET,0,ty);
}

static void
fp_pop()
{
	fpsp--;
	CHKUFLOW;
}

static void
fp_save()
{
	/* Save ST(0) at the end of the stack.  If the stack is too
	** full, put it out to memory.
	*/

	if (FP_ISEMPTY())
		cerror(gettxt(":696","fp_save(): empty stack"));
	if (!FP_ISFULL()) {
		fpsp++;
		fprintf(outfile, "\tfld\t%%st(0)\n");
		fpstack[fpsp] = fpstack[fpsp-1];
	}
	else {
		switch(fpstack[ST(0)].fpop) {
		case TEMP: 
			fprintf(outfile,"\tfst%c\t",
				fp_suffix(fpstack[ST(0)].ftype));
			printout_stack_addr(fpstack[ST(0)].fpoff1);
			putc('\n',outfile);
			break;
		default:
		 	cerror(gettxt(":697","fp_save: bad op: %d"), fpstack[ST(0)].fpop);
		}
	}
}

static int 
fp_count_temps(temp,p)
NODE *temp, *p;
{
	int ret = 0;
	if (p->in.op == TEMP && 
	    p->in.c.off == temp->in.c.off && 
     	    p->in.type == temp->in.type)
		return 1;
	switch(optype(p->in.op)) {
	case BITYPE:
		ret += fp_count_temps(temp, p->in.right);
		/* FALLTHRU */
	case UTYPE:
		ret += fp_count_temps(temp, p->in.left);
		/* FALLTHRU */
	case LTYPE:
		return ret;
	}
	/* NOTREACHED */
}

static void
fp_save_temps(p)
NODE *p;
{
	NODE tnode;
	int i;
	for (i=fpsp; i>=0; i--) {
		if (fpstack[i].fpop != TEMP)
			continue;
		switch(fpstack[i].ftype) {
			case TCHAR:
			case TUCHAR:
			case TSHORT:
			case TUSHORT:
			case TULONG:
			case TLONG:
			case TULLONG:
			case TLLONG:
			case TUNSIGNED:
			case TINT:
			case 0:
#ifndef NODBG
				FP_DEBUG("No save for non float temp on fp stack", 0, 0);
#endif
				continue;
			case TFLOAT:
			case TDOUBLE:
			case TLDOUBLE:
				break;
			default:

#ifdef NODBG
				cerror(gettxt(":1578","fp_save_temps: confused type for fp temp: %d"), fpstack[i].ftype);
#else
				cerror("fp_save_temps: confused type for fp temp: %s", TYPENAME(fpstack[i].ftype));
#endif
		}
		tnode.in.op = TEMP; 
		tnode.in.type = fpstack[i].ftype;
		tnode.in.c.off = fpstack[i].fpoff1;

		if (references(&tnode) > fp_count_temps(&tnode,p)) {
			if (i != ST(0))
				fprintf(outfile, "\tfxch\t%%st(%d)\n",ST(i));
			fprintf(outfile,"\tfst%c\t",
				fp_suffix(tnode.in.type));
			printout_stack_addr(tnode.in.c.off);
			putc('\n', outfile);
			if (i != ST(0))
				fprintf(outfile, "\tfxch\t%%st(%d)\n",ST(i));
		}
	}
			
}
	
static void
fp_xch(i)
{
	struct fpstype t;
	if (i >= fpsp) 
		cerror(gettxt(":698","fp_xch: illegal exchange value: %d"), i);
	t = fpstack[i];
	fpstack[i] = fpstack[fpsp];
	fpstack[fpsp] = t;
}
	
static void
fp_clear(i)
int i;
{
	register struct fpstype *fp = &fpstack[i];
	fp->fpop = 0;
	fp->name = 0;
	fp->fpoff1 = 0;
	fp->fpoff2 = BAD_OFFSET;
	fp->isqnode = 0;
	fp->ftype = TVOID;
}

static int
fp_onstack(p)
NODE *p;
/* Return the location where p is closest to the top of the stack.
** It may be on the stack more than once.
*/
{
	int i;

	/* If stin says p is in a register, it
	** must be in ST(0)
	*/

	if (p->tn.op == REG)
		return ST(0);

	/* I do not fully understand isqnode.  I believe it is intended
	** to signify that we just assigned to QNODE, and therefore just
	** took a branch.  Is the stack empty below this point?  I do
	** not know; however, we dot save things on the stack if there
	** are branches, so maybe we are ok.  This should be heavily tested,
	** though undoubtedly it will not be heavily used.
	*/

	for (i=fpsp; i>=0; i--)

		if (fpstack[i].isqnode) 
			return -1;

		else if (optype(fpstack[i].fpop) != LTYPE)
			continue;

		else if (fpstack[i].fpop != NOOP && 			
		 	p->tn.op == fpstack[i].fpop &&		
			(p->tn.op != NAME || p->tn.name == fpstack[i].name) &&
		 	fpstack[i].ftype == p->tn.type &&		
		    		(p->tn.c.off == fpstack[i].fpoff1 ||	
		     		 p->tn.c.off == fpstack[i].fpoff2) )
			break;
	return i;
}

static char
fp_suffix(type)
TWORD type;
{
	switch (type) {
		case TFLOAT: 	return 's';
		case TDOUBLE:	return 'l';
		case TLDOUBLE:	return 't';
		default:
#ifdef NODBG
			cerror(gettxt(":699","fp_suffix(): illegal type"));
#else
			cerror("fp_suffix(): illegal type %d",type);
#endif
	}
	/* NOTREACHED */
}

static void
#ifdef NODBG
fp_binop(l, op, r)
#else
fp_binop(l, op, r, stinline)
int stinline;
#endif
int op;
NODE *l, *r;
{
	int where_left = fp_onstack(l);
	int where_right = fp_onstack(r);
	int live_left = fp_keep(l, 1);
	int live_right = fp_keep(r, 1);
	TWORD ltype = l->in.type;
	TWORD rtype = r->in.type;
	char *opstring;
	int reverse = 0;

	switch(op) {
	case PLUS:	opstring = "fadd";			break;
	case MINUS:	opstring = "fsub";	reverse = 1;	break;
	case MUL:	opstring = "fmul";			break;
	case DIV:	opstring = "fdiv";	reverse = 1;	break;
	}

	/* If an operand is extended precision, it must be
	** on the stack.
	*/
	if (ltype == TLDOUBLE && where_left == -1) {
		fprintf(outfile, "\tfldt\t");
		adrput(l);
		fprintf(outfile, "\n");
		fp_push(l);
		where_left = ST(0);
		if (r->in.op != REG && where_right == -1)
			where_right = fp_onstack(r);	/* in case l == r */
	}
	if (rtype == TLDOUBLE && where_right == -1) {
		fprintf(outfile, "\tfldt\t");
		adrput(r);
		fprintf(outfile, "\n");
		fp_push(r);
		where_right = ST(0);
		if (l->in.op != REG && where_left == -1)
			where_left = fp_onstack(l);
	}
	if (where_left == -1 && where_right == -1) {

		/* At least one must be on the stack.  Put
		** the one on that is live.  If one is a 
		** complicated expression, we do not track its
		** liveness, so load the other one.
		*/

		NODE *load = l;
		if (live_right)
			load = r;
		else if (optype(r->in.op == LTYPE)) 
			load = r;

		fprintf(outfile, "\tfld%c\t", fp_suffix(load->in.type));
		adrput(load);
		FP_STIN(stinline);
		fprintf(outfile, "\n");
		fp_push(load);
		if (load == l) {
			where_left = ST(0);
			where_right = fp_onstack(r);	/* in case l == r */
		}
		else if (load == r) {
			where_right = ST(0);
			where_left = fp_onstack(l);	/* in case l == r */
		}
	}
	if (where_right != -1 && where_left == -1) {

		/* right on stack, left in memory. We want to
		** get the tree into a canonical form where
		** the left is on the stack and the right is
		** in memory.
		*/

		int tmp;
		NODE *ntmp;
		if (reverse == 1)
			reverse = 2;	/* operands have been reversed */

		/* reverse operands */
		tmp = where_left; where_left = where_right; where_right = tmp;
		tmp = live_left; live_left = live_right; live_right = tmp;
		tmp = ltype;  ltype = rtype;  rtype = tmp;
		ntmp = l; l = r; r = ntmp;
	}
	if (where_left != -1 && where_right == -1) {

		/* left on stack, right in memory */

		if (live_left) {

			/* Keep a copy on the stack */

			if (where_left == ST(0))
				fp_save();
			else {
				fprintf(outfile, "\tfld\t%%st(%d)\n", 
					ST(where_left));
				fp_push(l);
			}
		}
		else {
			/* We want left in ST(0) */

			if (where_left != ST(0)) {
				fprintf(outfile, "\tfxch\t%%st(%d)\n",
					ST(where_left));
				fp_xch(where_left);
			}
		}
		fprintf(outfile, "\t%s%s%c\t", 
			opstring, 
			reverse == 2 ? "r" : "",
			fp_suffix(rtype));
		adrput(r);
		fprintf(outfile, "\n");
		fpstack[ST(0)].fpop = NOOP;
		return;
	}
	/* both are on the stack */

	if (where_left == where_right) {
		/* same item, i.e. X + X */

		int live = fp_keep(l, 2);	/* we know about 2 refs */

		if (where_left == ST(0)) {
			if (live)
				fp_save();
		}
		else if (live) {
			fprintf(outfile, "\tfld\t%%st(%d)\n", 
				ST(where_left));
			fp_push(l);
		}
		else {
			fprintf(outfile, "\tfxch\t%%st(%d)\n",
				ST(where_left));
			fp_xch(where_left);
		}
		fprintf(outfile, "\t%s\t%%st(0),%%st\n", opstring);
		fpstack[ST(0)].fpop = NOOP;
		return;
	}
	/* Different locations on stack.  We want to get to a
	** canonical situation where the left is in ST(0).
	*/
	if (where_right == ST(0)) {
		int tmp;
		NODE *ntmp;
		if (reverse == 1)
			reverse = 2;	/* operands have been reversed */

		/* reverse operands */
		tmp = where_left; where_left = where_right; where_right = tmp;
		tmp = live_left; live_left = live_right; live_right = tmp;
		tmp = ltype;  ltype = rtype;  rtype = tmp;
		ntmp = l; l = r; r = ntmp;
	}
	else if (where_left != ST(0)) {
			fprintf(outfile, "\tfxch\t%%st(%d)\n",
				ST(where_left));
			fp_xch(where_left);
	}
	/* Both are now on the stack, the left is in ST(0) */

	if (live_left)
		fp_save();

	if (where_right == ST(1) && !live_right) {
		fprintf(outfile, "\t%s%s\n", opstring, reverse==2 ? "r" : "");
		fp_pop();
		fpstack[ST(0)].fpop = NOOP;
		return;
	}
	else {
		fprintf(outfile, "\t%s%s\t%%st(%d),%%st\n",
			opstring,
			reverse == 2 ? "r" : "",
			ST(where_right));
		fpstack[ST(0)].fpop = NOOP;
		return;
	}
}

static int
fp_keep(p, n)
NODE *p;
int n;
/* Should we keep p on the floating point stack?  n is the number
** of live references we already know about.  We are only interested
** in live references after the n'th.  Here are the series of criteria
** we are interested in:
**
**	1. The node must be live after the n'th use.
**	2. At least one of its live uses must not be an
**	   arithmetic operation that can just as easily access 
**	   memory.  (Should this include assigment?)
**	3. The node is live and it is long double.
**	4. The node is live and it is a TEMP
*/
{
	int i;
	NODE *t, *l, *r;
	extern int ref_entries;
	extern NODE **ref_trees;

	if (!is_live(p, n))
		return 0;	/* Criteria 1 failed */

#ifndef NODBG
	if (p->tn.op == REG)
		cerror("FP0 is live!");
#endif
	if (p->tn.type == TLDOUBLE)
		return 1;	/* Criteria 3 Passed */

	if (p->tn.op == TEMP)
		return 1;	/* Criteria 4 Passed */

	/* Check Second Criteria */

	for (i=n; i<ref_entries; i++) {
		t = ref_trees[i];
		if (t == NULL)
			return 1;	/* Just to be safe */
		switch (t->in.op) {

		case PLUS:
		case MINUS:
		case MUL:
		case DIV:	
		case ASG PLUS:
		case ASG MINUS:
		case ASG MUL:
		case ASG DIV:	
			break;
		default:
			return 1;	/* Second Criteria Passed */
		}

		/* Here with arithmetic operation */

		/* Strip widening conversions */

		l = t->in.left;
		if (l->in.op == CONV && fp_widen(l)) 
			l = l->in.left;
		r = t->in.right;
		if (r->in.op == CONV && fp_widen(r))
			r = r->in.left;

		/* If both sides are leafs, and
		** neither is on the stack, we will
		** want to save this on the stack.
		*/
		if (	optype(r->in.type) == LTYPE &&
			optype(l->in.type) == LTYPE &&
			fp_onstack(l) == -1 &&
			fp_onstack(r) == -1
		   )
			return 1;
	}
	return 0;
}

static int 
fp_widen(p)
NODE *p;
/* return non-zero if p has a wider fp type than p->in.left */
{
	return (	p->in.type & (TFLOAT|TDOUBLE|TLDOUBLE) &&
			p->in.left->in.type & (TFLOAT|TDOUBLE|TLDOUBLE) &&
			gtsize(p->in.type) >= gtsize(p->in.left->in.type)
	);
}

#ifdef FIXED_FRAME
static char a_l[FRAME_OFFSET_STRING_LEN];

static char *
arg_offset_string(init_flag)
int init_flag;
{
        static int count;
        if(init_flag)
                sprintf(a_l,"+.ASZ%d", ++count);
        return &a_l[1];
}
#endif

static lasttype = 0;
static adrput_regtype = 0;
static specialcmp = 0;
#define NEXTZZZCHAR	( *++(*ppc) )
#define PEEKZZZCHAR	(*((*ppc)+1))

static void
zeh( pnode, ppc, q )    /* Args passed through from zzzcode for c++ eh */
register NODE *pnode;   /* tree node being expanded */
char **ppc;             /* code string, at 'Z' of macro name */
OPTAB *q;               /* template */
{
	char nextzzzchar;
	nextzzzchar = NEXTZZZCHAR;
	switch(nextzzzchar) {
	case 'l':
		fprintf(outfile,"%d",pnode->bn.c.label);
		break;
	default:
		cerror(gettxt(":0", "botched ZEH macro"));
	}
}

#ifdef FIXED_FRAME
	/* For now, following is used only to compensate for push
	** before structure calls. Diddled by ZFF+ and ZFF- macros.
	*/
static ff_offset_adjust;
#endif

static void
zffmov(NODE *pnode, char *str, OPTAB *q, int nb)
{
	expand(pnode, FOREFF, str, q);
#ifdef FIXED_FRAME
	if (cg_arg_offset != 0)
		fprintf(outfile, "%ld", cg_arg_offset);
	cg_arg_offset += nb;
#endif
	emit_str("(%esp)\n");
}

	/*
	** Following function splits out modifications
	** required to implement fixed_frame.  It gets
	** called from zzzcode when a ZFF macro is detected.
	**	a argument from register
	**	b leave hole for argument
	**	c argument from ICON
	**	D argument from double FP0
	**	d argument from in-memory double
	**	e optionally emit alternate entry prefix
	**	F argument from float FP0
	**	f argument from in-memory float
	**	i emit just after "call" code
	**	l argument from in-memory integer
	**	r register argument
	**	X argument from long double FP0
	**	x argument from in-memory long double
	**	+ adjust up for structure function target argument
	**	- adjust down for structure function target argument
	*/
static void
zff(NODE *pnode, char **ppc, OPTAB *q)
{
	int nzch, fixed = 0;

	if ((nzch = NEXTZZZCHAR) != 'F')
		cerror(gettxt(":1576", "botched ZFF macro"));
	nzch = NEXTZZZCHAR;
	if (zflag) {
		fprintf(outfile, "\t%s ZZF%c stin:%d\n",
			COMMENTSTR, nzch, q->stinline);
	}
#ifdef FIXED_FRAME
	if (move_args_flag || fixed_frame())
		fixed = 1;
	else if (pnode->in.strat & MOVEARGS) {
		/* Exact frame w/reg args--open the space */
		switch (nzch) {
		default: /* must be FUNARG-like ZZF use */
			fprintf(outfile, "\tsubl\t$%s,%s\n",
				arg_offset_string(1), rnames[REG_ESP]);
			move_args_flag = 1;
			fixed = 1;
			break;
		case 'e':
		case 'i':
		case 'r':
		case '+':
		case '-':
			break;
		}
	}
#endif
	switch (nzch) {
	default:
		cerror(gettxt(":1576", "botched ZFF macro"));
		/*NOTREACHED*/
	case 'a': /* argument from register */
	case 'c': /* argument from ICON */
		/* These are directly available in either form */
		if (pnode->tn.type & (TLLONG|TULLONG)) {
			if (fixed) {
				zffmov(pnode, "\tmovl\tAL,", q, 4);
				zffmov(pnode, "\tmovl\tUL,", q, 4);
			} else {
				expand(pnode, FOREFF,
					"\tpushl\tUL\n\tpushl\tAL\n", q);
			}
		} else if (fixed) {
			zffmov(pnode, "\tmovl\tAL,", q, 4);
		} else {
			expand(pnode, FOREFF, "\tpushl\tAL\n", q);
		}
		break;
	case 'F': /* argument from float FP0 */
		if (fixed) {
			zffmov(pnode, "\tfstps\t", q, 4);
		} else {
			emit_str("\tsubl\t$4,%esp\n\tfstps\t(%esp)\n");
		}
		break;
	case 'D': /* argument from double FP0 */
		if (fixed) {
			zffmov(pnode, "\tfstpl\t", q, 8);
		} else {
			emit_str("\tsubl\t$8,%esp\n\tfstpl\t(%esp)\n");
		}
		break;
	case 'X': /* argument from long double FP0 */
		if (fixed) {
			zffmov(pnode, "\tfstpt\t", q, 12);
		} else {
			emit_str("\tsubl\t$12,%esp\n\tfstpt\t(%esp)\n");
		}
		break;
	case 'l': /* argument from in-memory integer */
		if (pnode->tn.type & (TLLONG|TULLONG))
			goto case_d;
	case 'f': /* argument from in-memory float */
		if (fixed) {
			zffmov(pnode, "\tmovl\tAL,A1\n\tmovl\tA1,", q, 4);
		} else {
			expand(pnode, FOREFF, "\tpushl\tAL\n", q);
		}
		break;
	case 'd': /* argument from in-memory double */
	case_d:
		if (fixed) {
			zffmov(pnode, "\tmovl\tAL,A1\n\tmovl\tA1,", q, 4);
			zffmov(pnode, "\tmovl\tUL,A1\n\tmovl\tA1,", q, 4);
		} else {
			expand(pnode, FOREFF, "\tpushl\tUL\n\tpushl\tAL\n", q);
		}
		break;
	case 'x': /* argument from in-memory long double */
		if (fixed) {
			zffmov(pnode, "\tmovl\tAL,A1\n\tmovl\tA1,", q, 4);
			zffmov(pnode, "\tmovl\tUL,A1\n\tmovl\tA1,", q, 4);
			zffmov(pnode, "\tmovl\tUUL,A1\n\tmovl\tA1,", q, 4);
		} else {
			expand(pnode, FOREFF,
				"\tpushl\tUUL\n\tpushl\tUL\n\tpushl\tAL\n", q);
		}
		break;
	case '+': /* make room for structure address */
		if (fixed) {
			assert(ff_offset_adjust == 0);
			ff_offset_adjust += 4;
		}
		break;
	case '-': /* remove room for structure address */
		if (fixed) {
			assert(ff_offset_adjust == 4);
			ff_offset_adjust -= 4;
		}
		break;
#ifdef FIXED_FRAME
	case 'b': /* leave placeholder in argument area */
		cg_arg_offset += 4;
		if (pnode->tn.type & (TLLONG|TULLONG))
			cg_arg_offset += 4;
		break;
	case 'r': /* move simple argument into register */
		switch (++regarg_target) {
		default:
			cerror(gettxt(":0", "botched ZFFr macro"));
			/*NOTREACHED*/
		case 1:
			fixed = REG_EDX;
			break;
		case 2:
			fixed = REG_ECX;
			break;
		case 3:
			fixed = REG_EAX;
			break;
		}
		fprintf(outfile, "%sLIVE: %s\n", COMMENTSTR, rnames[fixed]);
		expand(pnode, FOREFF, "\tmovl\tAL,", q);
		fprintf(outfile, "%s\n", rnames[fixed]);
		break;
#endif
	case 'e':
#ifdef FIXED_FRAME
		if (regarg_target != 0)
			emit_str(ALTENTRYSTR);
#endif
		break;
	case 'i':
#ifdef FIXED_FRAME
		if (!fixed_frame() && move_args_flag) {
			fprintf(outfile, "\t.set\t%s,%ld\n",
				arg_offset_string(0), cg_arg_offset);
		}
		move_args_flag = 0;
		cg_arg_offset = 0;
		regarg_target = 0;
#endif
		break;
	}
	update_max_arg_byte_count();
}

void
ll_cmp_jmp(const INTCON *vp, int regno, const INTCON *lb, const INTCON *hb,
		int eqlab, int ltlab, int gtlab)
{
	INTCON vlo, vhi;
	INTCON lbh, hbh;
	int skip = -1;

	vlo = *vp;
	vhi = *vp;
	(void)num_unarrow(&vlo, 32);
	(void)num_lrshift(&vhi, 32);
	if (lb != 0) {
		lbh = *lb;
		(void)num_lrshift(&lbh, 32);
		if (num_ucompare(&vhi, &lbh) != 0)
			lb = 0;
	}
	if (hb != 0) {
		hbh = *hb;
		(void)num_lrshift(&hbh, 32);
		if (num_ucompare(&vhi, &hbh) != 0)
			hb = 0;
	}
	if (lb == 0 || hb == 0) {
		if (num_ucompare(&vhi, &num_0) == 0) {
			fprintf(outfile, "\ttestl\t%s,%s\n",
				rnames[regno + 1], rnames[regno + 1]);
		} else {
			fprintf(outfile, "\tcmpl\t$%s,%s\n",
				num_tohex(&vhi), rnames[regno + 1]);
		}
		if (lb == 0 && ltlab >= 0)
			fprintf(outfile, "\tjl\t%s%d\n", LABSTR, ltlab);
		if (hb == 0 && gtlab >= 0)
			fprintf(outfile, "\tjg\t%s%d\n", LABSTR, gtlab);
		skip = getlab();
		fprintf(outfile, "\tjnz\t%s%d\n", LABSTR, skip);
	}
	if (num_ucompare(&vlo, &num_0) == 0) {
		fprintf(outfile, "\ttestl\t%s,%s\n",
			rnames[regno], rnames[regno]);
	} else {
		fprintf(outfile, "\tcmpl\t$%s,%s\n",
			num_tohex(&vlo), rnames[regno]);
	}
	if (eqlab >= 0)
		fprintf(outfile, "\tje\t%s%d\n", LABSTR, eqlab);
	if (ltlab >= 0)
		fprintf(outfile, "\tjb\t%s%d\n", LABSTR, ltlab);
	if (gtlab >= 0)
		fprintf(outfile, "\tja\t%s%d\n", LABSTR, gtlab);
	if (skip >= 0)
		fprintf(outfile, "%s%d:\n", LABSTR, skip);
}

/*
 * The following codes are currently used in zzzcode:
 *	AaBbCcDdEeFfGHIikLlMnOoPpQRrSsTtUuVvWwZz *+ 0123456789
 */
void
zzzcode( pnode, ppc, q )        /* hard stuff escaped from templates */
register NODE *pnode;   /* tree node being expanded */
char **ppc;             /* code string, at 'Z' of macro name */
OPTAB *q;               /* template */
{
    register NODE *pl, *pr, *pn;
    register int temp1, temp2;
    static OFFSZ ctmp2;
    static OFFSZ ctmp1;
    static int save_specialcmp;

    switch( NEXTZZZCHAR ) {

    case 'a': {
	/* Check if %ax is busy.  If so, spill, otherwise, %ax can be
	 * used - This is for fstsw instruction.
	 */
	OFFSZ tmp = 0;

	if (busy[REG_EAX]) {
		tmp = ( freetemp( 1 )) / SZCHAR;
		fprintf(outfile, "\tmovl\t%%eax,");
		printout_stack_addr(tmp);
		putc('\n',outfile);
	}
	fprintf(outfile, "\tfstsw	%%ax\n\tsahf\n");
	if (tmp) {
		fprintf(outfile, "\tmovl\t");
		printout_stack_addr(tmp);
		fprintf(outfile,",%%eax\n");
	}
	break;
    }

    case 'E':
	/* Print out the address of a free temp location.  This is
	 * used mainly in floating point conversions, where registers
	 * must be placed somewhere on the stack temporarily.
	 *
	 * ZEs: print temp for single-precision
	 * ZEd: print temp for double-precision
	 * ZED: print second word of temp for double-precision
	 * ZEx: print temp for extended-precision
	 * ZEH: Handle E_H zz escape
	 */
	{
		char type = NEXTZZZCHAR;
		if(type == 'H') {
			zeh(pnode,ppc,q);
			break;
		}
		if (ctmp2 == 0)
		{
			int words;
			switch (type) {
			case 's':       words = 1;      break;
			case 'D':
			case 'd':       words = 2;      break;
			case 'x':	words = 3;	break;
			default:        cerror(gettxt(":700","illegal argument to ZE"));
			}
			ctmp2 = ( freetemp( words ) ) / SZCHAR;
		}
		printout_stack_addr((type != 'D') ? ctmp2
						  : ctmp2 + SZINT/SZCHAR);
		break;
       }
    case 'e':
	/* Reset the temp location address.
	 */
	ctmp2 = 0;
	break;
    case 'k':
	/* Check that a temp really is on the stack.  If it is not,
	** push it on the stack and exchange the top two elements.
	** This is intended for uses such as an extended-precision
	** subtraction template with a Temp and FP0:  the template
	** assumes that the FP0 operand is the top of the stack, and
	** that the Temp is on the stack (unlike single- or double-
	** precision, where the Temp may be in memory.  If the tree
	** passed in is not of type TLDOUBLE, we will do nothing.
	** Thus, Zk may be called by a more generic template.
	*/ 
	pl = getadr(pnode, ppc);
	if (pl->tn.type != TLDOUBLE)
		break;
	FP_DEBUG("Zk: Ensure temp on stack", q->stinline, pl->tn.c.off);

	if (fp_onstack(pl) >= 0)
		break;

	/* Push temp on stack, swap top two elements */
	fp_push(pl);
	emit_str("\tfldt\t");
	adrput(pl);
	if( zflag )
	    emit_str("\t\t/ Zk expansion");
	fp_xch(ST(1));
	emit_str("\n\tfxch\n");
	FP_DEBUG("After Zk, temp not found", 0, 0);
	break;

    case 'f':
	/* Generate the necessary floating point stack pop operand,
	 * and the necessary stack manipulation operands.  Pop the
	 * simulated stack.
	 */
	pl = getadr(pnode, ppc);
	FP_DEBUG("Before expanding Zf", q->stinline, pl->tn.c.off);
	temp2 = fp_suffix(pl->in.type);
	temp1 = fp_onstack(pl);
	if (temp1 < 0) {		/* output std temp location */
		if (temp2 == 't')
			cerror(gettxt(":701","extended-precision operand must be on stack"));
		fprintf(outfile, "%c\t", temp2);
		adrput(pl);
	}
	else if (temp1 == ST(0)) 
		fprintf(outfile, "\t%%st(0),%%st");
	else if (temp1 == ST(1))
		fp_pop();
	else
		fprintf(outfile, "\t%%st(%d),%%st", ST(temp1));

	fpstack[ST(0)].fpop = NOOP;	/* What should this be? */
	FP_DEBUG("After expanding Zf", 0, 0);
	break;
    case 'I':
	/* Generate the necessary stack address if it is a register
	 * and then increment the stack. if temp2 == 'N' don't
	 * generate an address
	 */
	pl = getadr(pnode, ppc);
	FP_DEBUG("Before expanding ZI", q->stinline, pl->tn.c.off);
	temp2 = NEXTZZZCHAR;
	if (temp2 != 'N') {
		temp1 = fp_onstack(pl);
		if (temp1 < 0) { /* output std temp location */
			if (temp2 == 'T') {
				switch (pl->in.type) {
				default:
					cerror("nonfloating use of ZIxT");
				case TFLOAT:
					temp2 = 's';
					break;
				case TDOUBLE:
					temp2 = 'l';
					break;
				case TLDOUBLE:
					temp2 = 't';
					break;
				}
			}
			fprintf(outfile, "%c\t", temp2);
			adrput(pl);
		} else
			fprintf(outfile, "\t%%st(%d)", ST(temp1));
	}
	fp_push(pl);
	FP_DEBUG("After expanding ZI", 0, 0);
	break;
    case 'i':
	/* Pop a stack item.
	 */
	FP_DEBUG("Zi: pop stack", q->stinline, 0);
	fp_pop();
	FP_DEBUG("After expanding Zi", 0, 0);
	break;
    case '0':
	/* Make sure node really is in FP0 */
	pl = getadr(pnode, ppc);
	FP_DEBUG("Z0: Node must be in %%st(0)", q->stinline, pl->tn.c.off);
	temp1 = fp_onstack(pl);
	if (temp1 < 0) {

		/* Not yet on stack */

		fprintf(outfile, "\tfld%c\t", fp_suffix(pl->in.type));
		adrput(pl);
		FP_STIN(q->stinline);
		fprintf(outfile, "\n");
		special_instr_end();
		fp_push(pl);

	} else if (temp1 != ST(0)) {

		/* On stack, but not in FP0 */

		fp_xch(temp1);
		FP_STIN(q->stinline);
		fprintf(outfile, "\tfxch\t%%st(%d)\n", ST(temp1));
	}
	FP_DEBUG("After Z0", 0, 0);
	break;

    case '1':
	/* FP0 is holding a TEMP.  
	**
	**	if TEMP is not live but is referenced again:
	**	save TEMP to memory and pop the stack.  We 
	**	can do this because no claims are made that
	**	FP0 will hold the TEMP after Z1.
	**
	** If TEMP is left in FP0, paint FP0 as TEMP to avoid
	** exchanges.  Thus there will be two copies of TEMP on
	** the stack if the TEMP is still live.
	*/
	pl = getadr(pnode, ppc);
	FP_DEBUG("Z1: Keep live temp on stack", q->stinline, pl->tn.c.off);

	if (!is_live(pl, 0) && references(pl) > 0) {
		fprintf(outfile,"\tfstp%c\t",fp_suffix(pl->in.type));
		printout_stack_addr(pl->in.c.off);
		putc('\n',outfile);
		fp_pop();
	}
	else
		fp_fill(ST(0), TEMP, NULL, pl->tn.c.off, BAD_OFFSET, 0, pl->tn.type);
	FP_DEBUG("After Z1", 0, 0);
	break;

    case '2':
	/* Output correct operation for floating point binary
	** operation. 
	**
	** Z2t[i][r]
	**	t is the tree
	**	i is if we are loading from an integer
	**	r is if a sub or div must be reversed
	*/
	pl = getadr(pnode, ppc);

	/* The result will be in FP0.  If FP0 currently is holding
	** a live TEMP, we better save that TEMP.  For modularity, this
	** may want to go elsewhere, but it is here because it needs
	** go before the operation is output.
	*/
	if (fpstack[ST(0)].fpop == TEMP) {
#ifndef NODBG
                int r;
#endif
		tnode.in.op = TEMP; 
		tnode.in.type = fpstack[ST(0)].ftype;
		tnode.in.c.off = fpstack[ST(0)].fpoff1;
		if (is_live(&tnode, 0))
			fp_save();
#ifndef NODBG
                else if ((r = references(&tnode)) > 0) {
			fprintf(outfile,"\tfst%c\t",
				fp_suffix(tnode.in.type));
			printout_stack_addr(tnode.in.c.off);
                        if(zflag) fprintf(outfile,
                                "\t/Saving temp with %d refs",r);
#else
		else if (references(&tnode) > 0) {
			fprintf(outfile,"\tfst%c\t",
				fp_suffix(tnode.in.type));
			printout_stack_addr(tnode.in.c.off);
#endif
			putc('\n',outfile);
		}
	}
	fprintf(outfile, "\tf");
	if (PEEKZZZCHAR == 'i') {
		NEXTZZZCHAR;
		putc('i', outfile);
	}
	temp2 = PEEKZZZCHAR == 'r' ? NEXTZZZCHAR : 0;
	switch (pl->tn.op) {
		case DIV:
		case ASG DIV:
			if (temp2)
				fprintf(outfile, "divr");
			else
				fprintf(outfile, "div");
			break;
		case MINUS:
		case ASG MINUS:
			if (temp2)
				fprintf(outfile, "subr");
			else
				fprintf(outfile, "sub");
			break;
		case PLUS:
		case ASG PLUS:
			fprintf(outfile, "add");
			break;
		case MUL:
		case ASG MUL:
			fprintf(outfile, "mul");
			break;
		default:
			cerror(gettxt(":702","Bad operation in Z2 macro: %d"), pl->tn.op);
	}
	break;
    case '3':
	/* Eat line if node is on stack */
	pl = getadr(pnode, ppc);
	if (fp_onstack(pl) != -1) 
		while( NEXTZZZCHAR != '\n' ) 
			;
	break;

    case '4':
	/* Generate code for a binary floating point operation.  The
	** tree passed in assumed to have the operation, with the
	** operands being the left and right children respectivley.
	** We are free to generate whatever code we want.
	*/
	pn = getadr(pnode, ppc);
	FP_DEBUG("Generating Binary expression", q->stinline, 0);
#ifndef NODBG
	if (fpdebug)	e2print(pn);
#endif
	pl = pn->in.left;
	pr = pn->in.right;
	if (pl->in.op == CONV) 
		pl = pl->in.left;
	if (pr->in.op == CONV)
		pr = pr->in.left;
	fp_binop(pl, pn->in.op, pr);
	FP_DEBUG("Done Generating Binary expression", 0, 0);
	break;

    case '5':
	/* Assigning NTMEM to TEMP.  Check if NTMEM is on stack.  If
	** it is, just paint that as the TEMP.  Otherwise, load it
	** into st(0).
	*/
	pl = getadr(pnode, ppc);
	FP_DEBUG("Loading NTMEM into TEMP", q->stinline, 0);
	pr = pl->in.right;
	if (pr->in.op == CONV)
		pr = pr->in.left;
	if ((temp1 = fp_onstack(pr)) != -1) {
		fpstack[temp1].fpop = TEMP;
		fpstack[temp1].ftype = pl->in.type;
		fpstack[temp1].fpoff1 = pl->in.left->in.c.off;
	}
	else {
		fprintf(outfile,"\tfld%c\t",fp_suffix(pr->in.type));
		adrput(pr);
		fprintf(outfile, "\n");
		fp_push(pl->in.left);
	}
	FP_DEBUG("After Loading NTMEM into TEMP", 0, 0);
	break;

    case '6':
	/* Are we done with a CSE?  If so, output a 'p' (indicating
	** pop floating point stack) and pop the stack.
	*/
	FP_DEBUG("Before expanding Z6", q->stinline, 0);
	pl = getadr(pnode, ppc);
	if (pl->in.op == CSE && references(pl) == 1) {
		putc('p', outfile);
		fp_pop();
		FP_DEBUG("After expanding Z6", q->stinline, 0);
	}
	break;
    case '7':
	/* Mark ST(0) with NOOP */
	FP_DEBUG("Before expanding Z7", q->stinline, 0);
	fpstack[ST(0)].fpop = NOOP;
	FP_DEBUG("After expanding Z7", q->stinline, 0);
	break;

    case '8':
	/* Compare ST(0) with right */
	FP_DEBUG("Before expanding Z8", q->stinline, 0);
	fp_save_temps(pnode);
	pl = pnode->in.right;
	temp1 = fp_onstack(pl);
	if (temp1 == ST(0)) {
		emit_str(pnode->in.op == CMP
			? "\tfucomp\t%st(0)\n" : "\tfcomp\t%st(0)\n");
		fp_pop();
	}
	else if (temp1 != -1 && temp1 == ST(1)) {
		emit_str(pnode->in.op == CMP ? "\tfucompp\n" : "\tfcompp\n");
		fp_pop();
		fp_pop();
	} 
	else if (temp1 != -1) {
		cerror("Unexpected stack location in Z8");
	}
	else if (pnode->in.right->in.type == TLDOUBLE) {
		emit_str("\tfldt\t");
		adrput(pl);
		emit_str("\n\tfxch\n");
		emit_str(pnode->in.op == CMP ? "\tfucompp\n" : "\tfcompp\n");
		fp_pop();
	}
	else if (pnode->in.op == CMP) {
		fprintf(outfile, "\tfld%c\t", fp_suffix(pl->in.type));
		adrput(pl);
		emit_str("\n\tfxch\n\tfucompp\n");
		fp_pop();
	}
	else {
		fprintf(outfile, "\tfcomp%c\t", fp_suffix(pl->in.type));
		adrput(pl);
		emit_str("\n");
		fp_pop();
	}
	FP_DEBUG("After expanding Z8", q->stinline, 0);
	break;
    case '9':
	/* Compare ST(0) with left */
	FP_DEBUG("Before expanding Z9", q->stinline, 0);
	fp_save_temps(pnode);
	pl = pnode->in.left;
	temp1 = fp_onstack(pl);
	if (temp1 == ST(0)) {
		emit_str(pnode->in.op == CMP
			? "\tfucomp\t%st(0)\n" : "\tfcomp\t%st(0)\n");
		fp_pop();
	}
	else if (temp1 != -1 && temp1 == ST(1)) {
		emit_str("\tfxch\n");
		emit_str(pnode->in.op == CMP ? "\tfucompp\n" : "\tfcompp\n");
		fp_pop();
		fp_pop();
	} 
	else if (temp1 != -1) {
		cerror("Unexpected stack location in Z9");
	}
	else {
		fprintf(outfile,"\tfld%c\t", fp_suffix(pnode->in.right->in.type));
		adrput(pl);
		emit_str(pnode->in.op == CMP ? "\n\tfucompp\n" : "\n\tfcompp\n");
		fp_pop();
	}
	FP_DEBUG("After expanding Z9", q->stinline, 0);
	break;
	
    case 'Q':
	/* Increment the number of Qnodes currently loaded.   */

	/* DMK says...
	 * This is needed when ?: statements pare used as args to subroutines
	 * passing floats (extra stack elements popped from ZG).
	 */
#define IS_RIGHT_QNODE (pnode->in.left->in.strat & RIGHT_QNODE)

	FP_DEBUG("Before expanding ZQ", q->stinline, 0);

	if (IS_RIGHT_QNODE) {
		assert(fpstack[ST(1)].isqnode == 1);

		/*fpstack[ST(1)].fpoff2 = fpstack[ST(0)].fpoff1; */
		/*fpstack[ST(1)].isqnode = 2;*/

		fpstack[ST(1)].isqnode = 0;
		FP_DEBUG("Right node of ? ", q->stinline, 0);
		fp_pop();
			/*
			** The following is a hack to fix the
			** problem that we think we know what
			** is on the top of the stack.
			** Since control flow depends on
			** the value of the left operand to ?,
			** the stack will contain the value of
			** either the left or right QNODE.
			** Without the next line, we think
			** the value is the right QNODE.
			** The real problem is that ?: code
			** generation is brain dead.
			*/
		fpstack[ST(0)].fpop = NOOP;
	} else {
		FP_DEBUG("Left node of ? ", q->stinline, 0);
		assert(fpstack[ST(0)].isqnode == 0);
		fpstack[ST(0)].isqnode = 1;
	}
	FP_DEBUG("After expanding ZQ", 0, 0);
	break;
    case 'p':
	/* Used for assigning return value.  After this routine,
	** the value passed in must be in %st(0), and no other
	** value may be on the stack.  The simulated stack will
	** be cleared.
	*/
	pl = getadr(pnode, ppc);
	FP_DEBUG("Before expanding Zp", q->stinline, pl->in.c.off);

	if (pl->tn.op == REG) {
		if (fpsp > 0) {
			/* Move st(0) to st(fpsp) and pop fpsp.  
			** Then pop fpsp-1 elements off the stack.
			*/
			fprintf(outfile,"\tfstp\t%%st(%d)\n", fpsp);
			fpsp -= 2;	
			fp_cleanup();
		}
	}
	else {

		temp1 = fp_onstack(pl);

		/* Clean the stack until it is empty, or until the
		** value we want is at the top of the stack.
		*/
		while (temp1 != fpsp && !FP_ISEMPTY())
			fp_remove();

		/* Now the stack is empty, or pl is on the top of the stack */

		if (FP_ISEMPTY()) {
			fprintf(outfile, "\tfld%c\t", fp_suffix(pl->in.type));
			adrput(pl);
			putc('\n', outfile);
		} else if (fpsp != 0) {
			/* Move st(0) to st(fpsp) and pop fpsp.  
			** Then pop fpsp-1 elements off the stack.
			*/
			fprintf(outfile,"\tfstp\t%%st(%d)\n", fpsp);
			fpsp -= 2;	
			fp_cleanup();
		}
	}
	fpsp = -1;
	break;
    case 'G':
	FP_DEBUG("Before expanding ZG", q->stinline, 0);
	fp_cleanup();
	FP_DEBUG("After expanding ZG", 0, 0);
	break;

    case 'L': {
#define MAXLAB  5
	int tempval = NEXTZZZCHAR;
	switch( tempval ) {
	        static int labno[MAXLAB+1]; /* saved generated labels */
	        int i, n;
	case '.':                       /* produce labels */
	        /* generate labels from 1 to n */
	        n = NEXTZZZCHAR - '0';
	        if (n <= 0 || n > MAXLAB)
	            cerror(gettxt(":703","Bad ZL count"));
	        for (i = 1; i <= n; ++i)
	            labno[i] = getlab();
	        break;
	
	default:
	        cerror(gettxt(":704","bad ZL"));
		/*NOTREACHED*/
	
	/* generate selected label number */
	case '1':
	case '2':
	case '3':
	case '4':          /* must have enough cases for MAXLAB */
	case '5':
	        fprintf(outfile,".L%d", labno[tempval - '0']); 
	        break;
	}         
	break;
    }
    case 'R':
	/* Read and change the rounding mode in the 80?87 to chop; remember
	 * old value at ctmp1(%ebp).  Use Zr to restore.
	 */
	ctmp1 = ( freetemp( 1 ) ) / SZCHAR;
	fprintf(outfile,  "\tfstcw\t");
	printout_stack_addr(ctmp1);
	fprintf(outfile,"\n\tmovw\t");
	printout_stack_addr(ctmp1);
	putc(',',outfile);
	temp1 = lasttype;
	lasttype = TSHORT;
	expand( pnode, FOREFF, "ZA2\n\torw\t$0x0c00,ZA2\n\tmovw\tZA2,", q );
	lasttype = temp1;
	printout_stack_addr(ctmp1+2);
	fprintf(outfile,  "\n\tfldcw\t");
	printout_stack_addr(ctmp1+2 );
	if( zflag )
	    emit_str("\t\t/ ZR expansion");
	putc('\n',outfile );
	break;
    case 'r':
	/* restore old rounding mode */
	fprintf(outfile,  "\tfldcw\t");
	printout_stack_addr(ctmp1 );
	if( zflag )
	    emit_str("\t\t/ Zr expansion");
	putc('\n',outfile );
	break;

    case 'u':
	/* Set rounding mode to -Inf for unsigned conversion magic.
	** Assume ctmp1+2 already contains the "chop" rounding mode.
	*/
	fprintf(outfile, "\txorw\t$0x800,");
	printout_stack_addr(ctmp1+2);
	putc('\n',outfile);
	fprintf(outfile, "\tfldcw\t");
	printout_stack_addr(ctmp1+2);
	if( zflag )
	    emit_str("\t\t/ Zu expansion");
	putc('\n',outfile );
	break;

    case 'c':
	/* Pop the appropriate argsize from sp */
#ifdef FIXED_FRAME
	if(!fixed_frame())
#endif
	{
	if (pnode->stn.argsize == SZINT)
	    fprintf(outfile, "\tpopl\t%%ecx\n");
	else
	    fprintf(outfile, "\taddl\t$%d,%%esp\n",
		    (((unsigned)pnode->stn.argsize+(SZINT-SZCHAR))/SZINT)*(SZINT/SZCHAR));
	}
	break;

    case 'd':
	/* Output floating constant for signed_MAX+1 or unsigned_MAX+1:
	** Zd.i:   output double constant (2**31); Zdi: output address
	** Zd.u:   output double constant (2**32); Zdu: output address
	** Zd.iu:  output both of the above; Zdiu: output two addresses (sic)
	** Zd.li:  output double constant (2**63); Zdli: output address
	** Zd.lu:  output double constant (2**64); Zdlu: output address
	** Zd.liu: output both of the above; Zdliu: output two addresses (sic)
	*/
    {
	static int labs[4];	/* one for each of the following */
	static const char *const fmts[] = {
	    ".L%d:	.long	0,0x41e00000\n",	/* .double 0x1.p31 */
	    ".L%d:	.long	0,0x41f00000\n",	/* .double 0x1.p32 */
	    ".L%d:	.long	0,0x43e00000\n",	/* .ext 0x1.p63 */
	    ".L%d:	.long	0,0x43f00000\n",	/* .ext 0x1.p64 */
	};
	int index, c;
	int gendata = 0;
	int didprefix = 0;

	if ((c = NEXTZZZCHAR) == '.') {
	    gendata = 1;
	    c = NEXTZZZCHAR;
	}

	index = 0;
	if (c == 'l') {
	    index = 2;
	    c = NEXTZZZCHAR;
	}

	for (;;) {
	    int lab;

	    switch( c ){
	    case 'i':	break;
	    case 'u':	index++; break;
	    default:	cerror(gettxt(":705", "bad Zd%c"), c);
	    }
	    
	    lab = labs[index];
	    if (gendata) {
		if (lab == 0) {
		    labs[index] = lab = getlab();
		    if (!didprefix) {
			(void) locctr(FORCE_LC(CDATA));
			emit_str("\t.align\t8\n");
		    }
		    fprintf(outfile, fmts[index], lab);
		    didprefix = 1;
		}
	    }
	    else {
		if (picflag) {
		    fprintf(outfile, ".L%d@GOTOFF(%s)", lab, rnames[BASEREG]);
		    gotflag |= GOTREF;
		}
		else
		    fprintf(outfile, ".L%d", lab);
	    }
	    if (PEEKZZZCHAR == 'u')
		c = NEXTZZZCHAR;
	    else
		break;
	}
	if (didprefix)
	    (void) locctr(PROG);
	break;
    }
    case 'P':
	/* Allocate and generate the address for the appropriate amount
	 * of space for a return struct field.  This is a replacement
	 * for the 'T' macro when TMPSRET is defined.
	 */
	pl = talloc();
	pl->tn.op = TEMP;
	pl->tn.type = TSTRUCT;
	pl->tn.c.off = freetemp( (pnode->stn.stsize+(SZINT-SZCHAR))/SZINT );
	pl->tn.c.off /= SZCHAR;
	adrput(pl);
	tfree(pl);
	break;
    case 'B':
	/* The B and b macros set and reset the lasttype flag.  This
	 * variable is used by the T,t,A zzzcode macros to determine
	 * register name size to output.  T and t output the register
	 * names according to parameters in various subtrees, where as
	 * A prints the register names according to the last T,t,B.
	 */
	switch( *++(*ppc) ) {
	    case '1':
		if (*(*ppc + 1) == '0') {
		    lasttype = TLDOUBLE;
		    ++(*ppc);
		    break;
		}
		lasttype = TCHAR;
		break;
	    case '2':
		lasttype = TSHORT;
		break;
	    case '4':
		lasttype = TINT;
		break;
	    case '6':
		lasttype = TFLOAT;	/* can't be 4, but will probably
					 * never be used anyways... */
		break;
	    case '8':
		lasttype = TDOUBLE;
		break;
	}
	break;

    case 'b':
	/* reset the lasttype flag. */
	lasttype = 0;
	break;

    case 'T':
	/* Output the appropriate size for the given operation */
	/* 'T' Does not try to do zero/sign extend */
	pl = getadr(pnode, ppc);
	lasttype = pl->in.type;
	switch (pl->in.type) {
	    case TCHAR:
	    case TUCHAR:
		putc('b',outfile);
		break;
	    case TSHORT:
	    case TUSHORT:
		putc('w',outfile);
		break;
	    case TINT:
	    case TUNSIGNED:
	    case TLONG:
	    case TULONG:
	    case TPOINT:
		putc('l',outfile);
		break;
	    case TFLOAT:
		if (pl->in.op != REG)
		    putc('s',outfile);
		break;
	    case TDOUBLE:
		if (pl->in.op != REG)
		    putc('l',outfile);
		break;
	    case TLDOUBLE:
		if (pl->in.op != REG)
		    putc('t',outfile);
		break;
	    case TLLONG: /*for now*/
	    case TULLONG: /*for now*/
	    default:
		emit_str("ERROR");
		break;
	}
	break;
    case 't':
	/* Output the appropriate size for the given operation */
	/* 't' Does zero/sign extend  When appropriate */
	pl = getadr(pnode, ppc);
	lasttype = pl->in.type;
	if (pl->in.type != pnode->in.type) {
	    switch( pl->in.type) {
		case TCHAR:
		case TSHORT:
		    putc('s',outfile);
		    break;
		case TUCHAR:
		case TUSHORT:
		    putc('z',outfile);
		    break;
		default:
		    break;
	    }
	}
	switch (pl->in.type) {
	    case TCHAR:
	    case TUCHAR:
		putc('b',outfile);
		break;
	    case TSHORT:
	    case TUSHORT:
		putc('w',outfile);
		break;
	    case TINT:
	    case TUNSIGNED:
	    case TLONG:
	    case TULONG:
	    case TPOINT:
		putc('l',outfile);
		break;
	    case TLLONG: /*for now*/
	    case TULLONG: /*for now*/
	    default:
		emit_str("ERROR");
		break;
	}
	if (pl->in.type != pnode->in.type) {
	    switch( pnode->in.type) {
		case TSHORT:
		case TUSHORT:
		    if ((pl->in.type & (TSHORT|TUSHORT)) &&
			(pnode->in.type & (TSHORT|TUSHORT)))
			    break;
		    putc('w',outfile);
		    break;
		case TLLONG:
		case TULLONG:
		    /* we are only getting the low part... */
		case TLONG:
		case TULONG:
		case TINT:
		case TUNSIGNED:
		case TPOINT:
		case TFLOAT:		/* floats doubles/ go to int regs    */
		case TDOUBLE:		/* first and the templates know this */
		case TLDOUBLE:
		    if ((pl->in.type & (TLONG|TULONG|TINT|TUNSIGNED|TPOINT)) &&
			(pnode->in.type & (TLONG|TULONG|TINT|TUNSIGNED|TPOINT)))
			    break;
		    putc('l',outfile);
		    break;
		default:
		    break;
	    }
	}
	break;

    case 'A':
	/*
	 * This is a special output mode that allows registers of
	 * a lower type (specifically char) to me used in a movl.
	 * This is used in a case such as when you want to:
	 *	movl	%edi, %al	but really must do:
	 *	movl	%edi, %eax
	 * The assembler can't handle the first case.
	 */
	pl = getadr(pnode, ppc);
	adrput_regtype = lasttype;
	adrput(pl);
	adrput_regtype = 0;
	break;

    case 'U':
	/*
	 * Same as the above, except for upput().
	 */
	pl = getadr(pnode, ppc);
	adrput_regtype = lasttype;
	upput(pl, 1);
	adrput_regtype = 0;
	break;

    case 'F': {
	int fsz, foff, fty;

	switch (PEEKZZZCHAR) {
	case 'F':
		zff(pnode, ppc, q);
		temp1 = -1;
		break;
	case 'v': /* extract the value from long long FLD */
	case 't': /* extract FORCC from unsigned long long FLD */
	case 'p': /* assign to long long FLD from LLAWD/ICON pair */
	case 'P': /* case 'p' but also want the value */
	case 'r': /* assign to long long FLD from single register */
	case 'R': /* case 'r' but also want the value */
	case 'm': /* assign to long long FLD from single memory word */
	case 'M': /* case 'm' but also want the value */
		temp1 = NEXTZZZCHAR;
		break;
	default:
		temp1 = '\0';
		break;
	}
	if (temp1 < 0) /* already handled by zff() */
		break;
	if (temp1 == 'v' || temp1 == 't') { /* extract from long long FLD */
		fsz = UPKFSZ(pnode->in.c.off);
		foff = UPKFOFF(pnode->in.c.off);
		fty = pnode->in.type;
		temp2 = pnode->in.left->tn.type;
		if (foff >= SZLONG) {
			/* Entire field is in second word */
			expand(pnode, FOREFF, "\tmovl\tUL,A1\n", q);
			/* Cause expand() to see a single word based FLD */
			pnode->in.c.off = PKFIELD(fsz, foff - SZLONG);
		x1word:;
			pnode->in.type = (fty == TLLONG) ? TLONG : TULONG;
			if (temp1 != 't')
				expand(pnode, FOREFF, "\txorl\tU1,U1\n", q);
			expand(pnode, FOREFF,
				temp1 == 't' ?
					"\ttestl\t$MR,A1\n"
				: temp2 == TLLONG ?
					"\tshll\t$32-HR-SR,A1\n" /*CAT*/
					"\tsarl\t$32-SR,A1\n"
				: "H?R\tshrl\t$HR,A1\n\tandl\t$NR,A1\n", q);
			if (foff >= SZLONG)
				pnode->in.c.off = PKFIELD(fsz, foff);
			pnode->in.type = fty;
		} else if (fsz + foff <= SZLONG) {
			/* Entire field is in first word */
			expand(pnode, FOREFF, "\tmovl\tAL,A1\n", q);
			goto x1word;
		} else if (fsz <= SZLONG) {
			/* Entire field fits in a word, but spans both */
			expand(pnode, FOREFF,
				"\tmovl\tAL,A1\n" /*CAT*/
				"\tmovl\tUL,U1\n" /*CAT*/
				"H?R\tshrdl\t$HR,U1,A1\n", q);
			expand(pnode, FOREFF,
				temp1 == 't' ?
					"\ttestl\t$NR,A1\n"
				: temp2 == TLLONG ?
					"\txorl\tU1,U1\n" /*CAT*/
					"\tshll\t$32-SR,A1\n" /*CAT*/
					"\tsarl\t$32-SR,A1\n"
				: "\txorl\tU1,U1\n\tandl\t$NR,A1\n", q);
		} else {
			/* Big field, must span both words */
			expand(pnode, FOREFF,
				"\tmovl\tAL,A1\n" /*CAT*/
				"\tmovl\tUL,U1\n" /*CAT*/
				"H?R\tshrdl\t$HR,U1,A1\n", q);
			expand(pnode, FOREFF,
				temp1 == 't' ?
					"\ttestl\t$M>R<<HR,U1\n" /*CAT*/
					"ZL.1\tjne\tZL1\n" /*CAT*/
					"\ttestl\tA1,A1\n" /*CAT*/
					"ZL1:\n"
				: temp2 == TLLONG ?
					"H?R\tsarl\t$HR,U1\n"
				: "H?R\tshll\t$HR,U1\n", q);
		}
		break;
	}
	pl = getlr(pnode, NEXTZZZCHAR); /* the FLD node */
	fsz = UPKFSZ(pl->in.c.off);
	foff = UPKFOFF(pl->in.c.off);
	fty = pl->in.type;
	temp2 = pl->in.left->tn.type;
	pr = getadr(pnode, ppc); /* the rhs of the assignment */
	if (temp1 == '\0') {
		/* Clobber the line if the field size and the ICON are 1 */
		if (fsz == 1 && num_ucompare(&pr->tn.c.ival, &num_1) == 0) {
			while (NEXTZZZCHAR != '\n')
				;
		}
		break;
	}
	/* Must be an assignment to a long long FLD */
	if (pr->tn.op == ICON && pr->tn.name == (char *)0) {
		/* Do the masking part of ZH now to catch all zero cases */
		(void)num_unarrow(&pr->tn.c.ival, fsz);
		if (num_ucompare(&pr->tn.c.ival, &num_0) == 0) {
			temp1 = (temp1 == 'P') ? 'Z' : 'z';
		}
	}
	if (foff >= SZLONG || fsz + foff <= SZLONG) {
		/* Entire field fits either in the first or second word */
		if (foff >= SZLONG)
			pl->in.c.off = PKFIELD(fsz, foff - SZLONG);
		pl->in.type = (fty == TLLONG) ? TLONG : TULONG;
		expand(pnode, FOREFF, foff >= SZLONG
			? "\tandl\t$M~L,U(LL)\n"
			: "\tandl\t$M~L,A(LL)\n", q);
		if (pr->in.op == ICON) {
			if (temp1 == 'Z') {
				expand(pnode, FOREFF,
					"\txorl\tA1,A1\n" /*CAT*/
					"\txorl\tU1,U1\n", q);
			} else if (temp1 == 'z') {
				/*EMPTY*/
			} else {
				expand(pnode, FOREFF, foff >= SZLONG
					? "\torl\t$ZHLR,U(LL)\n"
					: "\torl\t$ZHLR,A(LL)\n", q);
				if (temp1 == 'P') {
					if (temp2 == TLLONG)
						num_snarrow(&pr->tn.c.ival, fsz);
					expand(pnode, FOREFF,
						"\tmovl\tAR,A1\n" /*CAT*/
						"\tmovl\tUR,U1\n", q);
				}
			}
		} else {
			expand(pnode, FOREFF,
				"\tmovl\tAR,A1\n" /*CAT*/
				"\tandl\t$NL,A1\n" /*CAT*/
				"H?L\tshll\t$HL,A1\n", q);
			expand(pnode, FOREFF, foff >= SZLONG
				? "\torl\tA1,U(LL)\n"
				: "\torl\tA1,A(LL)\n", q);
			if (temp1 == 'p' || temp1 == 'r' || temp1 == 'm') {
				/*EMPTY*/
			} else if (temp2 == TLLONG) {
				if (fsz + UPKFOFF(pl->tn.c.off) < SZLONG) {
					expand(pnode, FOREFF,
						"\tshll\t$32-HL-SL,A1\n", q);
				}
				expand(pnode, FOREFF,
					"\tmovl\tA1,U1\n" /*CAT*/
					"\tsarl\t$31,U1\n" /*CAT*/
					"\tsarl\t$32-SL,A1\n", q);
			} else {
				expand(pnode, FOREFF,
					"H?L\tshrl\t$HL,A1\n" /*CAT*/
					"\txorl\tU1,U1\n", q);
			}
		}
		if (foff >= SZLONG)
			pl->in.c.off = PKFIELD(fsz, foff);
		pl->in.type = fty;
	} else {
		/* Field spans the two words */
		expand(pnode, FOREFF,
			"\tandl\t$M&~L,A(LL)\n" /*CAT*/
			"\tandl\t$M>~L,U(LL)\n", q);
		if (pr->tn.op == ICON) {
			if (temp1 == 'Z') {
				expand(pnode, FOREFF,
					"\txorl\tA1,A1\n" /*CAT*/
					"\txorl\tU1,U1\n", q);
			} else if (temp1 == 'z') {
				/*EMPTY*/
			} else {
				INTCON tmp;

				expand(pnode, FOREFF,
					"\torl\t$ZH&LR,A(LL)\n", q);
				tmp = pr->tn.c.ival;
				(void)num_lrshift(&pr->tn.c.ival, 32 - foff);
				if (num_ucompare(&pr->tn.c.ival, &num_0) != 0) {
					expand(pnode, FOREFF,
						"\torl\tAR,U(LL)\n", q);
				}
				pr->tn.c.ival = tmp;
				if (temp1 == 'P') {
					if (temp2 == TLLONG)
						num_snarrow(&pr->tn.c.ival, fsz);
					expand(pnode, FOREFF,
						"\tmovl\tAR,A1\n" /*CAT*/
						"\tmovl\tUR,U1\n", q);
					
				}
			}
		} else {
			expand(pnode, FOREFF, "\tmovl\tAR,A1\n", q);
			expand(pnode, FOREFF,
				fsz > SZLONG && (temp1 == 'p' || temp1 == 'P') ?
					"\tmovl\tUR,U1\n" /*CAT*/
					"\tandl\t$N>L,U1\n"
				: fsz > SZLONG && pr->in.type & (TINT|TLONG) ?
					"\tmovl\tA1,U1\n" /*CAT*/
					"\tsarl\t$31,U1\n"
				: "\txorl\tU1,U1\n", q);
			expand(pnode, FOREFF,
				"\tandl\t$N&L,A1\n" /*CAT*/
				"H?L\tshldl\t$HL,A1,U1\n" /*CAT*/
				"H?L\tshll\t$HL,A1\n" /*CAT*/
				"\torl\tU1,U(LL)\n" /*CAT*/
				"\torl\tA1,A(LL)\n", q);
			if (temp1 == 'p' || temp1 == 'r' || temp1 == 'm') {
				/*EMPTY*/
			} else if (temp2 == TLLONG) {
				expand(pnode, FOREFF,
					"\tshll\t$64-HL-SL,U1\n" /*CAT*/
					"\tsarl\t$64-HL-SL,U1\n" /*CAT*/
					"H?L\tshrdl\t$HL,U1,A1\n" /*CAT*/
					"H?L\tsarl\t$HL,U1\n", q);
			} else {
				expand(pnode, FOREFF,
					"H?L\tshrdl\t$HL,U1,A1\n", q);
				expand(pnode, FOREFF, fsz <= SZLONG
					? "\txorl\tU1,U1\n"
					: "H?L\tshll\t$HL,U1\n", q);
			}
		}
	}
	break;
    }
    case 'H':
	/* Put out a shifted constant that has been properly
	 * modified for FLD ops.  This constant is anded (and
	 * overwritten) so it will fit into the field.  Then
	 * it is shifted for the field location and written out.
	 */
	{
	extern int fldsz, fldshf;
	INTCON tmp;
	int mask;

	mask = 0;
	if (PEEKZZZCHAR == '&') {
		mask = 1;
		(void)NEXTZZZCHAR;
	}
	pl = getlr( pnode, NEXTZZZCHAR );		/* FLD operator */
	pr = getlr( pnode, NEXTZZZCHAR );		/* ICON operator */
	fldsz = UPKFSZ(pl->tn.c.off);
	fldshf = UPKFOFF(pl->tn.c.off);
	(void)num_unarrow(&pr->tn.c.ival, fldsz); /* modifies the ICON */
	tmp = pr->tn.c.ival;
	(void)num_llshift(&tmp, fldshf);
	if (mask)
		(void)num_unarrow(&tmp, SZINT);
	fprintf(outfile, "%s", num_tohex(&tmp));
	}
	break;

    case 'O':
	{
	int shft;

	pl = getadr(pnode, ppc);
	(void)num_tosint(&pl->tn.c.ival, &shft);
	fprintf(outfile,  "%d", 1 << shft );
	}
	break;

    case 'C':
	if ((temp1 = specialcmp) == 0)
		temp1 = save_specialcmp;
	if (temp1 < 0) {
		extern int target_flag;
		int op = pnode->bn.lop - EQ;

		/* Generate a floating point conditional branch */
		if (!ieee_fp())
			op = noieeerel[op] - EQ;
		fprintf(outfile,  "%s\t.L%d",
		    target_flag == 6 ? fp6ccbranches[op]
			: fpccbranches[op], pnode->bn.c.label);
	} else if (temp1 != 0) {
		/* Multiword integer compare special case code.
		 * ZDL did the first part of the compare, we need
		 * to generate an appropriate conditional branch
		 * and then define the label given by temp1.
		 */
		int op = mwrelop[pnode->bn.lop - EQ];

		fprintf(outfile, "%s\t.L%d\n.L%d:",
			ccbranches[op - EQ], pnode->bn.c.label, temp1);
	} else {
		/* Generate a conditional branch */
		fprintf(outfile,  "%s\t.L%d",
		   ccbranches[pnode->bn.lop - EQ], pnode->bn.c.label);
	}
	if( zflag ) {
	    emit_str( "\t\t/ ZC expansion");
	}
	putc('\n',outfile );
	save_specialcmp = specialcmp = 0;
	break;

    case 'D':
	/* The next zzzcode better be a ZC or Zl or this is useless */
	temp1 = NEXTZZZCHAR;
	if (temp1 == 'F')
		specialcmp = -1; /* emit floating compare sequence in ZC */
	else if (temp1 != 'L')
		cerror(gettxt(":0", "botched ZD macro"));
	else {
		specialcmp = getlab(); /* almost always used */
		temp2 = pnode->in.c.label; /* early positive */
		/*
		 * A null pr means comparing against ICON zero.
		 * A null pl means comparing against ICON w/high order zero.
		 */
		if (pnode->in.op == GENBR) {
			pr = 0; /* implicit compare against zero */
			pl = 0;
		} else {
			pr = pnode->in.right;
			pl = pr;
			if (pr->in.op == ICON && pr->in.name == (char *)0) {
				if (num_ucompare(&pr->in.c.ival, &num_0) == 0) {
					pr = 0;
					pl = 0;
				} else {
					INTCON tmp = pr->in.c.ival;

					(void)num_lrshift(&tmp, 32);
					if (num_ucompare(&tmp, &num_0) == 0)
						pl = 0;
				}
			}
		}
		switch (pnode->in.lop) {
		default:
			cerror(gettxt(":0", "unknown relop in ZDL"));
			/*NOTREACHED*/
		case NE:
		case LG:
		case ULG:
			specialcmp = 0; /* nevermind */
			temp1 = NE;
			break;
		case EQ:
		case NLG:
		case UNLG:
			temp1 = NE;
			temp2 = 0; /* no early positive */
			break;
		case LGE:
		case ULGE:
		case NLGE:
		case UNLGE:
			specialcmp = 0; /* nevermind */
			temp1 = 0; /* REALLY nevermind */
			break;
		case LT:
		case NGE:
		case GE:
		case NLT:
			if (pr == 0) { /* don't care about low half's value */
				temp1 = -1; /* don't generate 2nd compare */
				specialcmp = 0;
				temp2 = 0;
			}
			else if (pnode->in.lop == LT || pnode->in.lop == NGE) {
		case LE:
		case NGT:
				temp1 = LT;
			} else {
		case GT:
		case NLE:
				temp1 = GT;
			}
			break;
		case ULT:
		case UNGE:
		case ULE:
		case UNGT:
			temp1 = ULT;
			if (pl == 0)
				temp2 = 0; /* no early positive possible */
			break;
		case UGT:
		case UNLE:
		case UGE:
		case UNLT:
			temp1 = UGT;
			break;
		}
		if (temp1 != 0) {
			expand(pnode, FOREFF, pr != 0 ? "\tcmpl\tUR,UL\n"
				: "\tcmpl\t$0,UL\n", q);
			if (temp2 != 0) {
				fprintf(outfile, "\t%s\t.L%d\n",
					ccbranches[temp1 - EQ], temp2);
			}
			if (specialcmp != 0)
				fprintf(outfile, "\tjnz\t.L%d\n", specialcmp);
			if (temp1 > 0) {
				expand(pnode, FOREFF, pr != 0 ?
					"\tcmpl\tAR,AL\n" : "\tcmpl\t$0,AL\n",
					q);
			}
		}
		if (zflag)
			emit_str("\t/ ZD expansion\n");
	}
	return;

    case 'l':
	/* Save specialcmp until ZC is seen. */
	if (specialcmp) 
	    save_specialcmp = specialcmp;
	break;

    case 'S':
	/* STASG - structure assignment */
	fprintf(outfile,"/STASG**************:\n");
        /* If the root strat field contains a VOLATILE flag,
               move the strat field to the left and right nodes */

        if ( pnode->in.strat & VOLATILE )
        {
                pnode->in.left->in.strat |= VOLATILE;
                pnode->in.right->in.strat |= VOLATILE;
        }
        /* now call stasg for the actual structure assignemnt */
	stasg( pnode->in.left, pnode->in.right, pnode->stn.stsize, q );
	CLEAN();
	fprintf(outfile,"/End STASG^^^^^^^^^^^^^^:\n");
	break;

    case 's':
	/* STARG - structure argument */
	{   /* build a lhs to stasg to, on the stack */
	NODE px, pxl, pxr;

	px.in.op = PLUS;
	pxl.tn.type = px.in.type = pnode->in.left->in.type;
	px.in.left = &pxl;
	px.in.right = &pxr;
	px.in.strat = 0;
	pxl.tn.op = REG;
	pxl.tn.sid = REG_ESP;
	pxl.tn.strat = 0;
	pxr.tn.op = ICON;
	pxr.tn.type = TINT;
	pxr.tn.name = 0;
#ifdef FIXED_FRAME
	if(fixed_frame())
		num_fromslong(&pxr.tn.c.ival, cg_arg_offset);
	else
#endif
		pxr.tn.c.ival = num_0;
	pxr.tn.strat = 0;

	fprintf(outfile,"/STARG**************:\n");
#ifdef FIXED_FRAME
	if(fixed_frame()) {
		cg_arg_offset += ((pnode->stn.stsize+(SZINT-SZCHAR))/SZINT)*(SZINT/SZCHAR);
		update_max_arg_byte_count();
		
	}
	else
#endif
	fprintf(outfile,  "\tsubl\t$%d,%%esp",
		((pnode->stn.stsize+(SZINT-SZCHAR))/SZINT)*(SZINT/SZCHAR));
	if( zflag ) {
	    emit_str( "\t\t/ STARG setup\n");
	}
	putc('\n',outfile );
	if (pnode->in.strat & VOLATILE) {
	    px.tn.strat = VOLATILE;
	    pnode->in.left->in.strat |= VOLATILE;
	}
	stasg( &px, pnode->in.left, (BITOFF)pnode->stn.stsize, q );
	CLEAN();
	fprintf(outfile,"/End STARG^^^^^^^^^^^^^^:\n");
	break;
	}

	/*
	 * This macro generates the optimal multiply by a constant
	 * sequence.  The calling sequence is:
	 *        ZM'SIZE','CONSTANT','OP','RESULT'.
	 * For instance ZML,R,L,1[,2] might generate the proper shift
	 * sequence or the sequence imulZTL $CR,AL,A1.  The break off
	 * point for generating shifts/adds/subs/moves over imulx is
	 * nine cycles.  pr is the constant's node, pl is the left
	 * side, and pt is the temp location node.  [,2] is an optional
	 * second temporary which may be useful.
	 */
    case 'M':
	{
		register NODE *pt;
		char multype;

		pr = getadr(pnode, ppc);
		switch (pr->in.type) {
			case TCHAR: case TUCHAR:
				multype = 'b';
				break;
			case TSHORT: case TUSHORT:
				multype = 'w';
				break;
			default:
				multype = 'l';
				break;
		}
		if (NEXTZZZCHAR != ',')
			cerror(gettxt(":706", "botched ZM macro"));
		pr = getadr(pnode, ppc);
		if (NEXTZZZCHAR != ',')
			cerror(gettxt(":706", "botched ZM macro"));
		pl = getadr(pnode, ppc);
		if (NEXTZZZCHAR != ',')
			cerror(gettxt(":706", "botched ZM macro"));
		pt = getadr(pnode, ppc);
		if ( **ppc == ',' ) {
			++(*ppc);
			(void) getadr(pnode, ppc);
		}
		fprintf(outfile, "\timul%c\t", multype);
		adrput(pr);
		putc(',',outfile);
		vol_opnd_end();
		adrput(pl);
		putc(',',outfile);
		vol_opnd_end();
		adrput(pt);
		putc('\n',outfile);
		special_instr_end();
	}
	break;			/* OUT of 'M' */

    case 'n':
	/* Generate code for floating point constant moves 
	**
	**	Zn.	Generate code if right was an FCON.  Eat the
	**		rest of the line if so, otherwise do nothing.
	**	Zn+	Continue to eat lines if we have generated code.
	**	Zn-	Stop eating lines.
	*/
	{
		static int eat = 0;
		int rep[3];
		char buf[80];
		int i;
		switch (NEXTZZZCHAR) {
		case '+':	
			if (eat)
				while( NEXTZZZCHAR != '\n' ) /* EMPTY */ ;
			break;
		case '-':
			eat = 0;
			break;
		case '.':
			if (pnode->in.right->in.op != NAME ||
			    !(pnode->in.right->in.strat & WAS_FCON))
				break;
			eat = 1;
			while( NEXTZZZCHAR != '\n' ) /* EMPTY */ ;
			switch(fcon_to_array(pnode->in.right, rep)) {
			case 3:
				sprintf(buf, "\tmovl\t$%d,UUL\n",rep[2]);
				expand(pnode, FOREFF, buf, q);
				/* FALLTHRU */
			case 2:
				sprintf(buf, "\tmovl\t$%d,UL\n",rep[1]);
				expand(pnode, FOREFF, buf, q);
				/* FALLTHRU */
			case 1:
				sprintf(buf, "\tmovl\t$%d,AL\n",rep[0]);
				expand(pnode, FOREFF, buf, q);
				break;
			default:
				cerror("fcon_to_array returned illegal value");
			}
			break;
		default:
			cerror("Illegal argument to Zn macro");
		}
	break;
	}

    case 'v':
        pl = getadr(pnode, ppc);
        if (pl->in.strat & VOLATILE)  vol_opnd |= cur_opnd;
	break;

    case 'W':
		fprintf(outfile,"/BLOCK MOVE*************:\n");
		blockmove(pnode, q);
                CLEAN();
		fprintf(outfile,"/END BMOVE^^^^^^^^^^^^^^:\n");
		break;

    /* Move long long from memory into register pair without using
    ** any additional registers.  (Sharing is required!)
    ** The only trick to decide whether to move the low or high
    ** half first depending on clashing register use.  We default
    ** to high first, only doing low first when only the low
    ** register in the destination is available.
    */
    case '=':
    {
	static const char hilo[] = "\tmovl\tUR,UL\n\tmovl\tAR,AL\n";
	static const char lohi[] = "\tmovl\tAR,AL\n\tmovl\tUR,UL\n";
	RST used, dst;
	NODE asg, star;
	extern void cfix_regsused();

	pl = getadr(pnode, ppc);
	pr = getadr(pnode, ppc);
	asg.in.op = ASSIGN;
	asg.in.left = pl;
	asg.in.right = pr;
	if (pl->in.op == REG) {
		used = RS_NONE;
		cfix_regsused(pr, &used);
		dst = RS_BIT(pl->tn.sid);
		if (used & RS_PAIR(dst)) {
			if ((used & dst) == 0) {
				expand(&asg, FOREFF, lohi, q);
				break;
			}
			if (pr->in.op != STAR)
				cerror(gettxt(":0","Both regs buzy in Z="));
			expand(&asg, FOREFF, "\tleal\tAR,AL\n", q);
			asg.in.right = &star;
			star = *pr;
			star.in.left = pl;
		}
	}
	expand(&asg, FOREFF, hilo, q);
	break;
    }

    /* Put out section directive for either .data or .rodata.
    ** Next character is 'd' for data, 't' for text.
    */
    case 'o':
		(void)locctr(NEXTZZZCHAR == 'd' ? FORCE_LC(CDATA) : PROG);
		break;

    /* Put out uninitialized data to "zero" */
    case 'z':
		zecode(pnode->tn.c.size);
		break;
    /* Treat operand as address mode under * */
    case '*':
	pl = getadr( pnode, ppc );		/* operand */
	starput( pl, (OFFSET) 0 );
	break;
    
    /* Operand is ++/-- node.  Produce appropriate increment/decrement
    ** code for the left operand of the ++/-- (actually, *(R++ )).
    */
    case '+':
    {
	int isincr;

	pl = getadr(pnode, ppc)->in.left;
	switch (pl->in.op) {
	case INCR:	isincr = 1; break;
	case DECR:	isincr = 0; break;
	default:
	    cerror(gettxt(":707","Bad Z+ operator %s"), opst[pl->in.op]);
	}

	if (num_ucompare(&pl->in.right->tn.c.ival, &num_1) == 0)
	    fprintf(outfile, "	%s	", isincr ? "incl" : "decl");
	else
	    fprintf(outfile, "	%s	$%s,", isincr ? "addl" : "subl",
				num_tosdec(&pl->in.right->tn.c.ival));
	adrput(pl->in.left);
	putc('\n', outfile);
	break;
    }
    case 'Z':
	/* Eat line if tree given is not unsigned (ZZ+) or signed (ZZ-) */
	temp1 = NEXTZZZCHAR;
	pl = getadr(pnode, ppc);
	switch (pl->in.type) {
	case TCHAR: case TSHORT: case TINT: case TLONG: case TLLONG:
		if (temp1 == '+')
			while( NEXTZZZCHAR != '\n' ) 
				/* EMPTY */ ;
		break;
	case TUCHAR: case TUSHORT: case TUNSIGNED: case TULONG: case TULLONG:
		if (temp1 == '-')
			while( NEXTZZZCHAR != '\n' ) 
				/* EMPTY */ ;
		break;
	default:
		cerror(gettxt(":1577","Bad type passed to ZZ macro"));
	}
	break;

    default:
	cerror(gettxt(":708", "botched Z macro %c"), **ppc );
	/*NOTREACHED*/
    }
    specialcmp = 0;		/* zero out if the last op wasn't a ZC */
}


/*
** fcon_to_array : dump the integer equivalents of float constants at location
**		   and return number of int's written.
*/
int 
fcon_to_array(node, location)
NODE *node;
int *location;	/* dump the integer equivalents of float constants at location */
{
	int		index;
	unsigned char	*efp;
	int		size;
	FP_LDOUBLE	fval = *(FP_LDOUBLE *)node->in.right; /*ugh*/
	FP_DOUBLE  	dbl;
	FP_FLOAT   	flt;

#define FormInt(ptr)	((ptr[3] << 24)|(ptr[2] << 16)|(ptr[1] << 8) | ptr[0])

	switch (node->tn.type) {
		case TLDOUBLE:  size  = 3;
				efp   = fval.ary;
				location[0] = FormInt(efp);
				efp+=4;
				location[1] = FormInt(efp);
				efp+=4;
				location[2] = (efp[1] << 8) | (efp[0] << 0);
				break;
		case TDOUBLE:   size  = 2;
				dbl   = FP_XTOD(fval);
				efp   = dbl.ary;
				location[0] = FormInt(efp);
				efp+=4;
				location[1] = FormInt(efp);
				break;
		case TFLOAT:    size  = 1;  
				flt   = FP_XTOF(fval);
				efp   = flt.ary;
				location[0] = FormInt(efp);
				break;
		default:	cerror("illegal node in fcon_to_array");
	}
	return size;
}

static int
cseput(NODE *p, int n) /* adrput for CSE node */
{
	struct cse *csp = getcse(p->csn.id);
	const char *rn;

	if (csp == 0) {
#ifndef NODBG
		e2print(p);
#endif
		cerror(gettxt(":709", "Unknown CSE id"));
	}
	if (n < 0)
		return csp->reg;
	n += csp->reg;
	switch (adrput_regtype != 0 ? adrput_regtype : p->tn.type) {
	default:
		rn = rnames[n];
		break;
	case TSHORT:
	case TUSHORT:
		rn = rsnames[n];
		break;
	case TCHAR:
	case TUCHAR:
		rn = rcnames[n];
		break;
	}
	emit_str(rn);
	return n;
}

void
conput( p ) 			/* name for a known */
register NODE *p; 
{
    INTCON isav;

    if (p->in.strat & VOLATILE) vol_opnd |= cur_opnd;
    switch( p->in.op ) 
    {
    case ICON:
	/* choose style for readability and for OPTIM's sake */
	isav = p->tn.c.ival;
	if (num_snarrow(&p->tn.c.ival, SZSHORT)) {
	    p->tn.c.ival = isav;
	    (void)num_snarrow(&p->tn.c.ival, SZLONG);
	    if (num_snarrow(&p->tn.c.ival, SZSHORT)) {
		p->tn.c.ival = isav;
		(void)num_unarrow(&p->tn.c.ival, SZLONG);
	    }
	}
	acon( p );
	p->tn.c.ival = isav;
	break;
    case REG:   /* get the right name for a register */
	switch (adrput_regtype != 0 ? adrput_regtype : p->tn.type)
	{
	default:        /* extended register, e.g., %eax */
	    emit_str( rnames[p->tn.sid]);
	    break;
	case TSHORT:
	case TUSHORT:   /* short register, %ax */
	    emit_str( rsnames[p->tn.sid]);
	    break;
	case TCHAR:
	case TUCHAR:    /* char register, %al */
	    emit_str( rcnames[p->tn.sid]);
	    break;
	}
	break;
    case UNINIT:
        acon(p);
        break;
    case CSE:
	(void)cseput(p, 0);
	break;
    case COPY:
    case COPYASM:
        if ( p->tn.name)
             emit_str(p->tn.name);
        break;

    default:
	cerror(gettxt(":710", "confused node in conput" ));
	/*NOTREACHED*/
    }
}
/*ARGSUSED*/
void
insput( p ) NODE *p; { cerror( "insput" ); /*NOTREACHED*/ }

/* Generate an addressing mode that has an implied STAR on top. */

static int baseregno;			/* base register number */
static int indexregno;			/* index register number */
static int scale;			/* scale factor */
static OFFSET offset;			/* current offset */
static char *name;			/* for named constant */
static OFFSET findstar();

static void
starput( p, inoff )
NODE *p; 
OFFSET inoff;				/* additional offset (bytes) */
{
    int flag = 0;
    /* enter with node below STAR */
#ifdef FIXED_FRAME
	/* Flag controls whether or not we must adjust offsets
	** by the stack size in base plus index addressing.
	** It's a hack.
	*/
    int fixed_frame_offset_flag = 0;
#endif
    baseregno = -1;
    indexregno = -1;
    scale = 1;
    offset = inoff;
    name = (char *) 0;

    /* Find the pieces, then generate output. */
#ifdef FIXED_FRAME
    offset += findstar(p,0,&flag,&fixed_frame_offset_flag);
#else
    offset += findstar(p,0,&flag);
#endif

    /* Prefer base register to index register. */
    if (indexregno >= 0 && baseregno < 0 && scale == 1) {
	baseregno = indexregno;
	indexregno = -1;
    }

    if (name) {
	emit_str(name);
	if (picflag) {
	    char *outstring = "";
	    if (flag & PIC_GOT) {
		gotflag |= GOTREF;
		if (flag & (NI_FLSTAT|NI_BKSTAT))
		    outstring = "@GOTOFF"; 
		else if (offset)
		    cerror(gettxt(":712","starput: illegal offset in ICON node: %s"), name);
		else 
		    outstring = "@GOT";
	    } 
	    else if (flag & PIC_PLT) {
		gotflag |= GOTREF;
		outstring = "@PLT";
	    }
	    emit_str(outstring);
	}
	if (offset > 0)
	    putc('+',outfile);
    }
#ifdef FIXED_FRAME
    if(fixed_frame() && fixed_frame_offset_flag && baseregno == REG_ESP) {
	if(offset)
		fprintf(outfile, "%ld+%s", offset, frame_offset_string(0));
	else
		emit_str(frame_offset_string(0));
    }
    else
#endif
    {
    if (offset)
	fprintf(outfile, "%ld", offset);
    else if (baseregno == REG_ESP)
	putc('0',outfile );
    }
    

    putc('(',outfile);
    if (baseregno >= 0)
	fprintf(outfile, "%s", rnames[baseregno]);
    if (indexregno >= 0) {
	fprintf(outfile, ",%s", rnames[indexregno]);
	if (scale > 1)
	    fprintf(outfile, ",%d", scale);
    }
    putc(')',outfile );
    sideff = 0;
    return;
}

void
printout_stack_addr(offset)
OFFSET offset;
{
#ifdef FIXED_FRAME
	if(fixed_frame()) {
		offset += ff_offset_adjust;
		if(offset)
			fprintf(outfile, "%ld%s(%s)",
				offset, PLUS_FRAME_OFFSET_STRING, rnames[REG_ESP]);
		else
			fprintf(outfile, "%s(%s)", FRAME_OFFSET_STRING, rnames[REG_ESP]);
	}
	else
#endif
	fprintf(outfile,"%ld(%s)", offset, rnames[REG_EBP]);
}

/* Find the pieces for address modes under *.  Return
** the numeric part of any constant found.  If "scaled"
** set, any register must be an index register.
*/
static OFFSET
#ifdef FIXED_FRAME
findstar(p,scaled,flag,fixed_frame_offset_flag_ptr)
int *fixed_frame_offset_flag_ptr;
#else
findstar(p,scaled,flag)
#endif
NODE * p;
int scaled;
int *flag;
{
    int rn;

    switch (p->in.op) {
    case CSE:
	rn = cseput(p, -1); /* just get the register */
	goto doreg;
    case REG:
	rn = p->tn.sid;
    doreg:
	/* Base register has pointer type, index is other.
	** With double indexing and an address constant, there
	** may be two index registers.  Be careful not to overwrite
	** the index register, and make sure the index register is
	** the one that gets scaled, if necessary.
	*/
	if (scaled) { /* Double-indexed with address constant case. */
	    if (indexregno >= 0)
		baseregno = indexregno;
	    indexregno = rn;
	} else if (baseregno < 0)
	    baseregno = rn;
	else
	    indexregno = rn;
	return 0;
    case ICON:
    {
	OFFSET temp;

	name = p->tn.name;
	*flag = p->tn.strat | (p->tn.sid & (NI_BKSTAT|NI_FLSTAT));
	(void)num_toslong(&p->tn.c.ival, &temp);
	return( temp );
    /* Force specific ordering of tree walk. */
    case PLUS:
#ifdef FIXED_FRAME
	temp = findstar(p->in.left,0,flag,fixed_frame_offset_flag_ptr);
	return(temp + findstar(p->in.right,0,flag,fixed_frame_offset_flag_ptr));
#else
	temp = findstar(p->in.left,0,flag);
	return(temp + findstar(p->in.right,0,flag));
#endif
    case MINUS:
#ifdef FIXED_FRAME
	temp = findstar(p->in.left,0,flag,fixed_frame_offset_flag_ptr);
	return(temp - findstar(p->in.right,0,flag,fixed_frame_offset_flag_ptr));
#else
	temp = findstar(p->in.left,0,flag);
	return(temp - findstar(p->in.right,0,flag));
#endif
    case LS:
	/* Assume *small* ICON on right. */
	(void)num_toslong(&p->in.right->tn.c.ival, &temp);
	scale = 1 << temp;
#ifdef FIXED_FRAME
	return(findstar(p->in.left,1,flag,fixed_frame_offset_flag_ptr));
#else
	return(findstar(p->in.left,1,flag));
#endif
    }
    case UNARY AND:
	p = p->in.left;
	if (p->in.op != VAUTO)
	    break;
	if (baseregno >= 0)
	    indexregno = baseregno;
#ifdef FIXED_FRAME
	if(fixed_frame()) {
	    baseregno = REG_ESP;
	    *fixed_frame_offset_flag_ptr = 1;
	}
	else
#endif
	baseregno = REG_EBP;
	if(p->tn.c.off >= 0)
		special_opnd |= cur_opnd;
	return( p->tn.c.off );
    }
    cerror(gettxt(":713","confused findstar, op %s"), opst[p->in.op]);
    /* NOTREACHED */
}

void
adrput( p ) 			/* output address from p */
NODE *p; 
{      
    INTCON isav;

    sideff = 0;

again:
    while( p->in.op == FLD || p->in.op == CONV )
	p = p->in.left;
    if (p->in.strat & VOLATILE)  vol_opnd |= cur_opnd;

    switch( p->in.op ) 
    {
    case ICON:               /* value of the constant */
	putc('$', outfile);
	isav = p->tn.c.ival;
	if (p->tn.type & (TLLONG|TULLONG)) { /* present "first" portion */

#ifndef RTOLBYTES
	    (void)num_lrshift(&p->tn.c.ival, SZLLONG - SZINT);
#endif
	    (void)num_unarrow(&p->tn.c.ival, SZINT);
	} else {
	    /* choose style for readability and for OPTIM's sake */
	    if (num_snarrow(&p->tn.c.ival, SZSHORT)) {
		p->tn.c.ival = isav;
		(void)num_unarrow(&p->tn.c.ival, SZLONG);
	    }
	}
	acon(p);
	p->tn.c.ival = isav;
	break;
    case NAME:
	acon( p );
	break;
    case CSE:
    case REG:
	conput( p );
	break;
    case STAR:
	if( p->in.left->in.op == UNARY AND ) {
	    p = p->in.left->in.left;
	    goto again;
	}
	starput( p->in.left, (OFFSET)0 );
	break;
    case UNARY AND:
	p = p->in.left;
	switch (p->in.op) {
	case STAR:
	    p = p->in.left;
	    goto again;
	case NAME:
	    putc('$', outfile);
	    /*FALLTHRU*/
	case TEMP:
	case VAUTO:     /* these appear as part of STARG and STASG */
	case VPARAM:
	    goto again;
	default:
	    cerror(gettxt(":715", "adrput:  U& over nonaddressable node" ));
	    /*NOTREACHED*/
	}
	break;
    case TEMP:
	printout_stack_addr(p->tn.c.off);
	break;
    case VAUTO:
	if(p->tn.c.off >= 0)
		special_opnd |= cur_opnd;
			/*
			** A positive offset for an auto
			** occurs for example with
			** p = a + ARRAY_SIZE + 1, a and p autos
			** ( int a[ARRAY_SIZE], *p; )
			** This confuses the optimizer's attempts
			** to identify the parameters, so put
			** out a special comment.
			*/
	printout_stack_addr(p->tn.c.off);
	break;
    case VPARAM:
		printout_stack_addr(PARM_OFFSET(p->tn.c.off));
	break;
    default:
	cerror( "adrput: illegal address" );
	/* NOTREACHED */
    }
}

    /* Output address of n words after p.  Used for double moves. */
void
upput( p, n ) 
register NODE *p; 
int n;
{
    register NODE *psav = p;

    while (p->in.op == FLD || p->in.op == CONV) {
	p = p->in.left;
    }
    if ((p->in.strat & VOLATILE) || (psav->in.strat & VOLATILE))
	vol_opnd |= cur_opnd;

recurse:
    switch (p->in.op) {
    case NAME:
    case VAUTO:
    case VPARAM:
    case TEMP:
	p->tn.c.off += n * SZINT/SZCHAR;
	adrput(psav);
	p->tn.c.off -= n * SZINT/SZCHAR;
	break;
#if 0 /*dfp--can't be right!*/
    case CSE:
    case REG:
	fprintf(outfile,  "%d", n * SZINT/SZCHAR);
	adrput(psav);
	break;
#else /*!0*/
    case REG:
	p->tn.sid += n;
	adrput(p);
	p->tn.sid -= n;
	break;
    case CSE:
	(void)cseput(p, n);
	break;
    case ICON:
	/*if (p->tn.type & (TLLONG|TULLONG))*/ { /* present "n-th" portion */
	    INTCON isav = p->tn.c.ival;

#ifdef RTOLBYTES
	    (void)num_lrshift(&p->tn.c.ival, n * SZINT);
#else
	    (void)num_lrshift(&p->tn.c.ival, SZLLONG - (n * SZINT));
#endif
	    (void)num_unarrow(&p->tn.c.ival, SZINT);
	    putc('$', outfile);
	    acon(p);
	    p->tn.c.ival = isav;
	    break;
	}
	/*FALLTHROUGH*/
#endif /*0*/
    default:
	cerror(gettxt(":716", "upput:  confused addressing mode"));
	/*NOTREACHED*/
    case STAR:
	starput(p->in.left, (OFFSET)n * SZINT/SZCHAR);
	break;
    case UNARY AND:
	p = p->in.left;
	goto recurse;
    }
}

void
acon( p ) 			/* print out a constant */
register NODE *p; 
{       
    OFFSET off;
    INTCON tmp;
    char * name;

    if( p->in.name == (char *) 0 ) {	/* constant only */
	/* Heuristically choose style for readability */
	tmp = p->tn.c.ival;
	emit_str(num_snarrow(&tmp, SZSHORT) != 0
	    ? num_tohex(&p->tn.c.ival) : num_tosdec(&p->tn.c.ival));
    } else {
	char *outstring = "";

	if (p->in.op == ICON)
	    (void)num_toslong(&p->tn.c.ival, &off);
	else
	    off = p->tn.c.off;
	if (picflag) {
	    register flag = p->tn.strat;

	    if (flag & PIC_GOT) {
		gotflag |= GOTREF;
		if (p->tn.sid & (NI_FLSTAT|NI_BKSTAT))
		    outstring = "@GOTOFF";
		else if (off != 0)
		    cerror(gettxt(":717","acon: illegal offset in ICON node: %s"), p->in.name);
		else
		    outstring = "@GOT";
	    } 
	    else if (flag & PIC_PLT) {
		gotflag |= GOTREF;
		outstring = "@PLT";
	    }
	}
	name = exname(cg_escape_leading_dollar(p->in.name));
	if (off == 0)     /* name only */
	    fprintf(outfile,  "%s%s", name, outstring );
	else if( off > 0 )                       /* name + offset */
	    fprintf(outfile,  "%s%s+%ld", name, outstring, off );
	else                                     /* name - offset */
	    fprintf(outfile,  "%s%s%ld", name, outstring, off );
    }
}

void
p2cleanup()
{
	/* must cleanup floating point stack after every statement.  This
	** could be done after every basic block, but I believe the cost
	** is the same.  This keeps the stack cleaner and allows for
	** potentially more temporaries in use to be kept on the stack.
	*/
	fp_cleanup();
}

/*ARGSUSED*/
special( sc, p )
NODE *p;
{
    cerror(gettxt(":718", "special shape used" ));
    /* NOTREACHED */
}

/* Move an intermediate result in a register to a different register. */
void 
rs_move( pnode, newregno ) 
NODE * pnode; int newregno; 
{
    register int type = pnode->tn.type;
    int r = regno(pnode);

    if( type & (TINT|TUNSIGNED|TPOINT) ) 
	fprintf(outfile,  "\tmovl\t%s,%s", rnames[r], rnames[newregno] );
    else if( type & (TSHORT|TUSHORT) )
	fprintf(outfile, "\tmovw\t%s,%s", rsnames[r], rsnames[newregno] );
    else if( type & (TCHAR|TUCHAR) ) 
	fprintf(outfile, "\tmovb\t%s,%s", rcnames[r], rcnames[newregno] );
    else 
	cerror(gettxt(":719", "bad rs_move" ));
    
    if( zflag ) {           /* if commenting on source of lines */
	emit_str( "\t\t/ RS_MOVE\n");
    }
    putc('\n',outfile );
}

static int
is_address_mode(p)
NODE *p;
/* Return non-zero if p can be evaluated by an address mode */
{

	NODE *l, *r;
	NODE *ll, *lr, *rl, *rr;

	/* Strip off top level STAR.  If no STAR, we can use leal */

	if (p->tn.op == STAR)
		p = p->tn.left;

	l = p->tn.left;
	r = p->tn.right;
	if (p->tn.op == MINUS)
		goto minus;	/* special case */
	if (p->tn.op != PLUS)
		return 0;

	/* Pick up indirect without base 
	**
	**	(Rsreg[iuip]+C[iuip])
	**	((Rsreg[iui]<<C)+C[iuip])	first C is 1,2,3
	*/
#	define SHIFT_CON(p) (p->tn.op == ICON && \
			num_ucompare(&p->tn.c.ival, &num_3) <= 0 && \
			num_ucompare(&p->tn.c.ival, &num_1) >= 0)

	if (r->tn.op == ICON && (r->tn.type & (TINT|TUNSIGNED|TPOINT))) {
		
		/* right side ok, check left side */

		if (l->tn.op == REG && (l->tn.type & (TINT|TUNSIGNED|TPOINT)))
			return 1;

		if (l->tn.op == LS) {
			ll = l->tn.left;
			lr = l->tn.right;

			if (	ll->tn.op == REG && 
			    	(ll->tn.type & (TINT|TUNSIGNED)) &&
				SHIFT_CON(lr)
			   )
				return 1;
		}
	}
	/* Double indexing with REG and implied stack pointer 
	**
	** 1.   &A + Rsreg[iui]
	** 2.	&A + (Rsreg[iui]<<C)
	** 3.	(&A+Rsreg[iui]) + C
	** 4.	(&A+(Rsreg[iui]<<C)) + C
	*/
#	define AAUTO(p)	(p->tn.op == UNARY AND && p->tn.left->tn.op == VAUTO)

	if (AAUTO(l)) {
		/* Case 1, 2 */
		if (r->tn.op == REG && (r->tn.type & (TINT|TUNSIGNED)))
			return 1;	/* case 1 */
		rl = r->tn.left;
		rr = r->tn.right;
		if (	rl->tn.op == REG && 
			(rl->tn.type & (TINT|TUNSIGNED)) &&
			SHIFT_CON(rr)
		   )
			return 1;	/* Case 2 */
	}
	ll = l->tn.left;
	if (l->tn.op == PLUS && AAUTO(ll) && r->tn.op == ICON) {
		/* Case 3, 4 */
		lr = l->tn.right;
		if (lr->tn.op == REG && (lr->tn.type & (TINT|TUNSIGNED)))
			return 1;	/* Case 3 */
		if (	lr->tn.op == LS &&
			lr->tn.left->tn.op == REG &&
			(lr->tn.left->tn.type & (TINT|TUNSIGNED)) &&
			SHIFT_CON(lr->tn.right)
		   )
			return 1;	/* Case 4 */
	}
	/* Double Indexing with two explicit registers 
	**
	** 1.	Rsreg[iuip] + Rsreg[iuip]
	** 2.	Rsreg[iuip] + (Rsreg[iui]<<C)
	** 3.	(Rsreg[iuip]+Rsreg[iuip]) + C
	** 4.	(Rsreg[iuip]+(Rsreg[iui]<<C1)) + C 
	*/

	if (l->tn.op == REG && (l->tn.type & (TINT|TUNSIGNED|TPOINT))) {
		/* Case 1, 2 */
		if (r->tn.op == REG && (r->tn.type & (TINT|TUNSIGNED|TPOINT))) 
			return 1;	/* case 1 */
		rl = r->tn.left;
		rr = r->tn.right;

		if (	r->tn.op == LS &&
			rl->tn.op == REG &&
			(rl->tn.type & (TINT|TUNSIGNED)) &&
			SHIFT_CON(rr)
		   )
			return 1;	/* case 2 */
	}
	if (r->tn.op == ICON && l->tn.op == PLUS) {
		/* Case 3, 4 */
		ll = l->tn.left;
		lr = l->tn.right;
		if (	ll->tn.op == REG  &&	
			(ll->tn.type & (TINT|TUNSIGNED|TPOINT)) &&
			lr->tn.op == REG  &&
			(lr->tn.type & (TINT|TUNSIGNED|TPOINT))
		   )
			return 1;	/* case 3 */
		if (	ll->tn.op == REG  &&    
                        (ll->tn.type & (TINT|TUNSIGNED|TPOINT)) &&
			lr->tn.op == LS &&
			lr->tn.left->tn.op == REG &&
			(lr->tn.left->tn.type & (TINT|TUNSIGNED|TPOINT)) &&
			SHIFT_CON(lr->tn.right)
		   )
			return 1;	/* case 4 */
	}
	
	return 0;

	/* Double Indexing with two explicit registers, special case
	** with - as root of tree.
	**
	** 5.   (Rsreg[iuip]+Rsreg[iuip]) - C[!p]
	** 6.	(Rsreg[iuip]+(Rsreg[iui]<<C)) - C[!p]
	*/
minus:
	if (l->tn.op == PLUS && r->tn.op == ICON && (!(r->tn.type & TPOINT))) {
		ll = l->tn.left;
		lr = l->tn.right;
		if (	ll->tn.op == REG  &&	
			(ll->tn.type & (TINT|TUNSIGNED|TPOINT)) &&
			lr->tn.op == REG  &&
			(lr->tn.type & (TINT|TUNSIGNED|TPOINT))
		   )
			return 1;	/* case 5 */

		if (	ll->tn.op == REG  &&    
                        (ll->tn.type & (TINT|TUNSIGNED|TPOINT)) &&
			lr->tn.op == LS &&
			lr->tn.left->tn.op == REG &&
			(lr->tn.left->tn.type & (TINT|TUNSIGNED|TPOINT)) &&
			SHIFT_CON(lr->tn.right)
		   )
			return 1;	/* case 6 */
	}
	return 0;
}

NODE *
suggest_spill(p)
NODE *p;
/* For now, this routine will only be concerened with finding
** the proper side of a floating point operation to spill.
*/
{
	int op = p->tn.op;
	TWORD type = p->tn.type;
	TWORD ltype, rtype;
	NODE *l = p->tn.left;
	NODE *r = p->tn.right;

	if (optype(op) != BITYPE)
		return NULL;		/* no need to make suggestion */

	if (asgop(op))
		return NULL;		/* not prepared to handle this yet */

	ltype = l->tn.type;
	rtype = r->tn.type;
	
	if ((ltype|rtype|type) & (~(TLDOUBLE|TDOUBLE|TFLOAT)))
		return NULL;

	/* Spill one side if other side is a leaf */

	if (optype(l->tn.op) == LTYPE)
		return r;	
	if (optype(r->tn.op) == LTYPE)
		return l;	

	if (is_address_mode(l))
		return r;
	if (is_address_mode(r))
		return l;
	return NULL;
}

static void
blockmove(pnode, q)
NODE *pnode;
OPTAB *q;
{
	/* Do a block move.  from and to are scratch
	** registers, count is either a scratch register
	** or a constant.
	*/
	int bk_vol_opnd = (pnode->in.strat & VOLATILE) ? VOL_OPND2 : 0;
	NODE *pcount, *pfrom, *pto;	/*node pointers */
	BITOFF size;

	/* This can be either a block move or VLRETURN node.
	** For block moves, the count may or may not be a 
	** scratch register; if not, a1 is available for a copy.
	** For VLRETURN, the count must be in a scratch reg;
	** a1 contains the "to" adress (%fp)
	*/

	switch ( pnode->in.op)
	{
	case BMOVE:
		/* Do a stasg() so that the code need only be maintained
		** in one place.  If the move count is not a constant, though,
		** we cannot do a stasg().
		*/
		pcount = pnode->in.left;
		pfrom = pnode->in.right->in.right;
		pto = pnode->in.right->in.left;
		if (pcount->tn.op != ICON || pcount->tn.name != 0)
			break;
		/* If the root strat field contains a VOLATILE flag,
		** move the strat field to the left and right nodes 
		*/

		if ( bk_vol_opnd ) {
			pfrom->in.strat |= VOLATILE;
			pto->in.strat |= VOLATILE;
		}
		(void)num_toulong(&pcount->tn.c.ival, &size);
		stasg(pto, pfrom, ORTOBI(size), q);
                CLEAN();
		return;

	case BMOVEO:
		cerror(gettxt(":720","blockmove: BMOVEO unsupported"));
		/*NOTREACHED*/
	default:
		cerror(gettxt(":721","Bad node passed to ZM macro"));
		/*NOTREACHED*/
	}
	/* Variable length block moves are not used by C.
	** Furthermore, we did them incorrectly in the past.  
	** For this reason, we will exit with a cerror at this point.
	*/
	cerror(gettxt(":722","blockmove: Variable length moves unsupported"));
	/*NOTREACHED*/
}

void
end_icommon()
{
	(void) locctr(locctr(UNK));
}

costex()
{
	cerror(gettxt(":723","costex(): not ported for i386 CG\n"));
	/*NOTREACHED*/
}

	/* output volatile/pos_offset operand information at the 
	** end of an instruction
	*/
void
special_instr_end()
{
        int opnd;
        int first = 1;

        for (opnd=0; vol_opnd; ++opnd)
        {
            if (vol_opnd & (1<<opnd))
            {
                   /* first time output the information for the instruction */
                   if ( first )
                   {
                        PUTS("/VOL_OPND ");
                        first = 0;
                   }
                   else
                        PUTCHAR(',');
                   vol_opnd &= ~(1<<opnd);      /* clean up the checked operand bit */
                   fprintf(outfile, "%d", opnd+1);
            }
        }

        if ( !first ) PUTCHAR('\n');

	first = 1;

        for (opnd=0; special_opnd; ++opnd)
        {
            if (special_opnd & (1<<opnd))
            {
                   /* first time output the information for the instruction */
                   if ( first )
                   {
                        PUTS("/POS_OFFSET ");
                        first = 0;
                   }
                   else
                        PUTCHAR(',');
                   special_opnd &= ~(1<<opnd);      /* clean up the checked operand bit */
                   fprintf(outfile, "%d", opnd+1);
            }
        }

        if ( !first ) PUTCHAR('\n');

	CLEAN();    /* reset the initial values for bookkeeping variables */
}

/* Routines to support HALO optimizer. */

#ifdef	OPTIM_SUPPORT

#ifndef	INI_OIBSIZE
#define	INI_OIBSIZE 100
#endif

static char oi_buf_init[INI_OIBSIZE];	/* initial buffer */
static
TD_INIT(td_oibuf, INI_OIBSIZE, sizeof(char), 0, oi_buf_init, "optim support buf");

#define	OI_NEED(n) if (td_oibuf.td_used + (n) > td_oibuf.td_allo) \
			td_enlarge(&td_oibuf, td_oibuf.td_used+(n)) ;
#define	OI_BUF ((char *)(td_oibuf.td_start))
#define	OI_USED (td_oibuf.td_used)

/* Produce #REGAL information for a local temp. */
static int temp_set = 0;
#define MAX_TEMPS  64
 /* optim will not use more then 32 temps */
struct temp_loc {
      OFFSET off;
      OFFSET last_off;
};
static struct temp_loc temp_array[MAX_TEMPS];

/*  oi_temp() will save temps created by freetemp(). If the temp size is more
** then one word this temp can't be in register.
**
**  oi_temp() will return true if temp that can't be in register overlap
** with /REGAL or if /REGAL overlap with long temp. In this case freetemp
** will try another offset.
*/

int
oi_temp(offset,size)
OFFSET offset;
int size;
{
	int i,last_offset;

	last_offset = offset + (size - 1) * 4;
	if (temp_set == MAX_TEMPS )
		return 0; /* too many temps. */
	for (i = 0; i < temp_set; i++) {
		if (    temp_array[i].off == offset
		     && temp_array[i].last_off == last_offset
		)
			return 0; /*  REGAL overlap with same size REGAL */
		if (  temp_array[i].off >= offset
		    && temp_array[i].off <= last_offset )
		    return 1; /* /REGAL partly overlap. No Good */
		if ( offset >= temp_array[i].off
		    && offset <= temp_array[i].last_off )
		    return 1; /* /REGAL partly overlap. No Good */
	}
	temp_array[temp_set].off = offset;
	temp_array[temp_set++].last_off = last_offset;
	return 0; /* New REGAL */
}

void
oi_temp_end(size)
OFFSET size;            /* size is byte size */
{
	int i;
	if (temp_set == MAX_TEMPS)
		temp_set = 0;	/* too many temps */
	else if (temp_set) {
		for (i = 0; i < temp_set; i++) {
			if (temp_array[i].off >= size)
				continue;
			if (temp_array[i].off == temp_array[i].last_off) {
#ifdef FIXED_FRAME
			if (fixed_frame())
				fprintf(outfile, "/REGAL\t0\tAUTO\t%ld%s(%s)\t4\n",
					temp_array[i].off,
					PLUS_FRAME_OFFSET_STRING, rnames[REG_ESP]);
			else
#endif
				fprintf(outfile, "/REGAL\t0\tAUTO\t%ld(%s)\t4\n",
					temp_array[i].off, rnames[REG_EBP]);
			}
			else {
#ifdef FIXED_FRAME
			if (fixed_frame())
				fprintf(outfile, "/REGAL\t0\tAUTO\t%ld%s(%s)\t%d\tFP\n",
					temp_array[i].off,
					PLUS_FRAME_OFFSET_STRING,
					rnames[REG_ESP],
					4 + temp_array[i].last_off - temp_array[i].off);
			else
#endif
				fprintf(outfile, "/REGAL\t0\tAUTO\t%ld(%s)\t%d\tFP\n",
					temp_array[i].off, rnames[REG_EBP],
					4 + temp_array[i].last_off - temp_array[i].off);
			}
		}
		temp_set = 0;
	}
}

/* Produce comment for loop code. */

char *
oi_loop(code)
int code;
{
    char * s;

    switch( code ) {
    case OI_LSTART:	s = "/LOOP	BEG\n"; break;
    case OI_LBODY:	s = "/LOOP	HDR\n"; break;
    case OI_LCOND:	s = "/LOOP	COND\n"; break;
    case OI_LEND:	s = "/LOOP	END\n"; break;
    default:
	cerror(gettxt(":724","bad op_loop code %d"), code);
    }
    return( s );
}


/* Analog of adrput, but this one takes limited address modes (leaves
** only) and writes to a buffer.  It returns a pointer to just past the
** end of the buffer.
*/
static void
sadrput(p)
NODE * p;				/* node to produce output for */
{
    int n;

    /* Assume need space for auto/param at a minimum. */
    /*      % n ( % ebp) NUL */

    OI_NEED(1+8+1+1+ 3+1+1);

    switch( p->tn.op ){
    case REG:
			OI_USED += sprintf(OI_BUF+OI_USED, "%s",rnames[p->tn.sid]);	
			break;
    case VAUTO:
    case VPARAM:
#ifdef FIXED_FRAME
			if(fixed_frame()) {
			    OI_NEED(FRAME_OFFSET_STRING_LEN);
			    OI_USED += sprintf(OI_BUF+OI_USED, "%ld%s(%s)",
				(p->tn.op == VAUTO ? p->tn.c.off : p->tn.c.off-4),
				PLUS_FRAME_OFFSET_STRING, rnames[REG_ESP]);
			}
			else
#endif
			OI_USED += sprintf(OI_BUF+OI_USED, "%ld(%s)",
					p->tn.c.off, rnames[REG_EBP]);
			break;
    case NAME:		n = strlen(p->tn.name);
			if (*p->tn.name == '$') {
				OI_NEED(2);
				(void) strcpy(OI_BUF+OI_USED,"\\");
				OI_USED += 1;
			}
			OI_NEED(n+1);
			(void) strcpy(OI_BUF+OI_USED, p->tn.name);
			OI_USED += n;
			if (p->tn.c.off != 0) {
			    OI_NEED(1+8+1);
			    OI_USED += sprintf(OI_BUF+OI_USED, "%s%ld",
					(p->tn.c.off > 0 ? "+" : ""),
					p->tn.c.off);
			}
			if (   picflag
			    && (p->tn.sid & (NI_GLOBAL))) {
			    OI_NEED(4+1);
			    (void) strcpy(OI_BUF+OI_USED, "@GOT");
			    OI_USED += 4;
			}
			else if (   picflag
			    && (p->tn.sid & (NI_FLSTAT|NI_BKSTAT))) {
			    OI_NEED(7+1);
			    (void) strcpy(OI_BUF+OI_USED, "@GOTOFF");
			    OI_USED += 7;
			}
			break;
    default:
	cerror(gettxt(":725","bad op %d in sadrput()"), p->in.op);
    }
    return;
}

/* Note that the address of an object was taken. */

char *
oi_alias(p)
NODE * p;
{
    BITOFF size = (p->tn.type & (TVOID|TSTRUCT)) ? 0 : gtsize(p->tn.type);

    OI_USED = 0;			/* start buffer */
    /*	    /ALIAS\t	*/
    OI_NEED(1+   5+1+1);
    OI_USED += sprintf(OI_BUF, "/ALIAS	");
    sadrput(p);
    /*	    \t% n\tFP\n */
    OI_NEED(1+1+8+1+2+1+1);
    (void) sprintf(OI_BUF+OI_USED, "	%ld%s\n", size/SZCHAR,
		(long)(p->tn.type & (TFLOAT|TDOUBLE|TLDOUBLE)) ? "	FP" : "");
    return( OI_BUF );
}

/* Produce #REGAL information for a symbol. */

char *
oi_symbol(p, class, offset)
NODE * p;
int class, offset;
{
    char * s_class;
    int reg;

    switch( class ) {
    case OI_AUTO:	s_class = "AUTO"; break;
    case OI_PARAM:	s_class = "PARAM"; break;
    case OI_RPARAM:	s_class = "RPARAM"; break;
    case OI_EXTERN:	s_class = "EXTERN"; break;
    case OI_EXTDEF:	s_class = "EXTDEF"; break;
    case OI_STATEXT:	s_class = "STATEXT"; break;
    case OI_STATLOC:	s_class = "STATLOC"; break;
    default:
	cerror(gettxt(":726","bad class %d in op_symbol"), class);
    }

    OI_USED = 0;			/* initialize */
    /*		/REGAL\t 0\tSTATLOC\t	*/
    OI_NEED(	1+   5+1+1+1+     7+1+1 );
    OI_USED += sprintf(OI_BUF, "/REGAL	0	%s	", s_class);
    if(class == OI_RPARAM) {
	reg = p->tn.sid; /* Save for later */
	p->tn.op = VPARAM;
	p->tn.c.off = offset;
	p->tn.sid = 0;
    }
    sadrput(p);
    /*	    \t% n\tFP\n */
    OI_NEED(1+1+8+1+2+1);
    OI_USED += sprintf(OI_BUF+OI_USED, "	%d%s", gtsize(p->tn.type)/SZCHAR,
		(p->tn.type & (TFLOAT|TDOUBLE|TLDOUBLE)) ? "	FP" : "");
    if(class == OI_RPARAM) {
	OI_NEED(1); 
        OI_USED += sprintf(OI_BUF+OI_USED, "\t");
	p->tn.op = REG;
	p->tn.sid = reg;
	p->tn.c.off = 0;
	sadrput(p);
    }
    OI_NEED(1); 
    OI_USED += sprintf(OI_BUF+OI_USED, "\n");
    return( OI_BUF );
}


put_br_hint(int comment)
{
	if (comment == ABOVE || comment == BELOW) return;
	fprintf(outfile,"/BRANCH: ");
	print_comment_string(comment);
}

print_comment_string(int comment)
{
		if (comment & GUARD_RET) fprintf(outfile,"GUARD_RET\t"); 
		if (comment & CMP_DEREF) fprintf(outfile,"CMP_DEREF\t"); 
		if (comment & GUARD_CALL) fprintf(outfile,"GUARD_CALL\t"); 
		if (comment & GUARD_LOOP) fprintf(outfile,"GUARD_LOOP\t"); 
		if (comment & GUARD_SHORT_BLOCK) fprintf(outfile,"GUARD_SHORT_BLOCK\t"); 
		if (comment & CMP_GLOB_CONST) fprintf(outfile,"CMP_GLOB_CONST\t"); 
		if (comment & CMP_PARAM_CONST) fprintf(outfile,"CMP_PARAM_CONST\t"); 
		if (comment & CMP_PTR_ERR) fprintf(outfile,"CMP_PTR_ERR\t"); 
		if (comment & CMP_GLOB_PARAM) fprintf(outfile,"CMP_GLOB_PARAM\t"); 
		if (comment & CMP_2_AUTOS) fprintf(outfile,"CMP_2_AUTOS\t"); 
		if (comment & CMP_LOGICAL) fprintf(outfile,"CMP_LOGICAL\t"); 
		if (comment & CMP_USE_SHIFT) fprintf(outfile,"CMP_USE_SHIFT\t"); 
		if (comment & SHORT_IF_THEN_ELSE) fprintf(outfile,"SHORT_IF_THEN_ELSE\t"); 
		if (comment & CMP_FUNC_CONST) fprintf(outfile,"CMP_FUNC_CONST\t"); 
		if (comment & COMPOUND) fprintf(outfile,"COMPOUND\t"); 
		if (comment) fprintf(outfile,"%x\n",comment);
}
#endif	/* def OPTIM_SUPPORT */
