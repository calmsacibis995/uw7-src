#ident	"@(#)cg:i386/local.c	1.32.1.47"
/*	local.c - machine dependent stuff for front end
 *	i386 CG
 *		Intel iAPX386
 */

#ifdef NVLT
#ifndef _KMEMUSER
#define _KMEMUSER
#endif
#endif

#include <signal.h>
#include "mfile1.h"
#include "mfile2.h"
#include <string.h>
#include <memory.h>
#include <unistd.h>

#ifdef NVLT
#include <sys/nwctrace.h>
int nvltflag = 0;
	 /* default mask, override with -2N0x... */
unsigned int nvltMask = NVLTM_prof;
#endif NVLT

/* register numbers in the compiler and their external numbers:
**
**	comp	name
**	0-2	eax,edx,ecx
**	3	fp0
**	5-7	ebx,esi,edi
*/

#ifndef TMPDIR
#define TMPDIR	"/tmp"		/* to get TMPDIR, directory for temp files */
#endif

/* bit vectors for register variables */

#define REGREGS		(RS_BIT(REG_EBX)|RS_BIT(REG_ESI)|RS_BIT(REG_EDI))
#define CREGREGS	(RS_BIT(REG_EBX))

/* *** i386 *** */
RST regstused = RS_NONE;
RST regvar = RS_NONE;
/* *** i386 *** */

int tmp_start;			/* start of temp locations on stack */
char request[TOTREGS];	/* list of reg vars used by the fct.
				   Given on either BEGF node or ENDF node */
int r_caller[]={-1};		/* no caller save register */
static int biggest_return;
#ifdef FIXED_FRAME
static int filefixedframeflag = 0;  /* If not set, do old style frame.
				** set with -2F[01], or #pragma fixed_frame [01] */
#endif
static int inlineintrinsicsflag = 1; 
static int ieeeflag = 1;
static int hostedflag = 1;
static int inlineallocaflag = 0;
int target_flag = 5;

static char *tmpfn;
static FILE *tmpfp;

FILE *fopen();
int proflag = 0;
int picflag = 0;
int gotflag;

static int callflag = 0;

#ifdef FIXED_FRAME

static functionfixedframeflag = 1;
int MAX_USER_REG;
#ifndef NODBG
static int use_ebp = 0; /* So debugging == nondebugging behavior */
#endif

void
set_max_user_reg()
{
#ifndef NODBG
	if(use_ebp && fixed_frame())
		MAX_USER_REG = REG_EBP;
	else
#endif
		MAX_USER_REG = REG_EDI;
}

int
get_file_fixed_frame_flag()
{
	return filefixedframeflag;
}

int
get_function_fixed_frame_flag()
{
	return functionfixedframeflag;
}

	/* functionfixedframeflag can't override corresponding file flag */
void
set_function_fixed_frame_flag(flag)
int flag;
{
	if(flag) {
		functionfixedframeflag = filefixedframeflag;
	}
	else {
		functionfixedframeflag = 0;
	}
}

int 
fixed_frame()
{
extern int calls_in_blocks;
	return calls_in_blocks > 1 && get_function_fixed_frame_flag();
#if 0
extern al_saw_safe_asm, al_saw_asm, al_saw_setjmp, al_saw_longjmp, al_saw_alloca;
	return (filefixedframeflag && functionfixedframeflag &&
		!al_saw_safe_asm && !al_saw_asm && !al_saw_setjmp
		&& !al_saw_longjmp && !al_saw_alloca);
#endif
}
#endif

int
ieee_fp()
{
	return ieeeflag;
}

int
inline_alloca()
{
	return inlineallocaflag;
}

int 
inline_intrinsics()
{
	return inlineintrinsicsflag;
}

int 
hosted()
{
	return hostedflag;
}

void
p2abort()
{
	extern int unlink();
	if (tmpfp)
		(void) unlink(tmpfn);
	return;
}

extern thrash_thresh;
char *intrinsic_dir;

void
myflags(cpp)
char **cpp;
{
	/* process flag pointed to by *cpp.  Leave *cpp pointing
	** to last character processed.  All these flags set
	** using -2...
	*/
	switch (**cpp)
	{
		case 'A':	++*cpp; inlineallocaflag = **cpp - '0'; break;
		case 'C':	callflag = 1; break;
		case 'c': {     /* -2c<number> set max_call_count */
			extern max_call_count;
			++*cpp; /* Point at first digit */
			max_call_count = strtol(*cpp,cpp,10);
			--*cpp; /* Point at last digit or c if none */
			break;
		}
#ifdef FIXED_FRAME
		case 'F':       ++*cpp;
				filefixedframeflag = **cpp - '0';
				functionfixedframeflag = filefixedframeflag;
				break;
#ifndef NODBG
		case 'n':	++*cpp; use_ebp = **cpp - '0'; break;
#endif
#endif
		case 'h':	++*cpp; hostedflag = **cpp - '0'; break;
		case 'I':	++*cpp; inlineintrinsicsflag = **cpp - '0'; break;
		case 'i':	++*cpp; ieeeflag = **cpp - '0'; break;
		case 'k':	picflag = 1; break;
#ifdef NVLT
		case 'N': {
			char *remember;	
			nvltflag = 3;
			remember = ++*cpp; /* Point at first digit */
			nvltMask = strtol(*cpp,cpp,0);
			if(*cpp == remember) nvltMask = NVLTM_prof;
			--*cpp; /* Point at last digit or N if none */
		}
#endif NVLT
			/* Processor: 3, 4, 5, 6: 386, 486, pent, p6 */
		case 'T':	++*cpp; target_flag = **cpp - '0'; break;
		case 't': {     /* -2t<number> set thrashing limit */
			++*cpp; /* Point at first digit */
			thrash_thresh = strtol(*cpp,cpp,10);
			--*cpp; /* Point at last digit or t if none */
			break;
		case 'Y':	/* User specifies location of intrinsics
				** with -2Y<path>
				** we assume no further -2 options
				** here, so need to advance *cpp to the end.
				*/
			intrinsic_dir = ++*cpp;
			while(**cpp)
				(*cpp)++;
			--*cpp; /* Point at last char or Y if none */
		}
		default:	break;
	}
}

#define LOC_ATTRIBUTE	01	/* attribute required */
#define LOC_SEEN	02	/* We have entered this section before */
#define LOC_SECTION	04	/* .section required */

static int location_map[] = {
	/* PROG */	0,
	/* ADATA */	1,
	/* DATA */	1,
	/* ISTRNG */	2,
	/* STRNG */	2,
	/* CDATA */	3,
	/* CSTRNG */	4,
	/* EH_RANGES */	5,
	/* EH_RELOC */	6,
	/* EH_OTHER */	7,
	/* CTOR */	8
};
struct location {
	char *name;
	char *attributes;
	char *type;	/* only specified if necessary */
	unsigned int flags;
} location[] = {
	/* PROG */ 		{ ".text", "ax", NULL, 0 },
	/* ADATA, DATA*/	{ ".data", "aw", NULL, 0 },
	/* ISTRNG, STRNG */	{ ".data1", "aw", NULL, LOC_SECTION|LOC_ATTRIBUTE },
	/* CDATA */		{ ".rodata", "a", NULL, LOC_SECTION|LOC_ATTRIBUTE },
	/* CSTRNG */		{ ".rodata1", "a", NULL, LOC_SECTION|LOC_ATTRIBUTE },
	/* EH_RANGES */		{ ".eh_ranges", "a", NULL, LOC_SECTION|LOC_ATTRIBUTE },
	/* EH_RELOC */		{ ".eh_reloc", "aw", "delayrel", LOC_SECTION|LOC_ATTRIBUTE },
	/* EH_OTHER */		{ ".eh_other", "a", NULL, LOC_SECTION|LOC_ATTRIBUTE },
	/* CTOR */		{ ".ctor", "aw", NULL, LOC_SECTION|LOC_ATTRIBUTE }
};

static void 
print_location(s)
int s;
{
	struct location *loc = &location[location_map[s]];
	if (loc->flags & LOC_SECTION)
		emit_str("\t.section");
	fprintf(outfile, "\t%s", loc->name);
	if ((loc->flags & (LOC_ATTRIBUTE|LOC_SEEN)) == LOC_ATTRIBUTE) {
		fprintf(outfile, ",\"%s\"", loc->attributes);
		loc->flags |= LOC_SEEN;
		if (loc->type != NULL)
			fprintf(outfile, ",\"%s\"", loc->type);
		fprintf(outfile, "\n");
	} else
		emit_str("\n");
}

void
cg_section_map(section, name)
int section;
char *name;
{
	struct location *loc = &location[location_map[section]];
	loc->name = name;
	loc->flags = LOC_SECTION|LOC_ATTRIBUTE;
}

extern int lastalign; 

/* location counters for PROG, ADATA, DATA, ISTRNG, STRNG, CDATA, and CSTRNG */
locctr(l)		/* output the location counter */
{
	static int lastloc = UNK;
	int retval = lastloc;		/* value to return at end */

	curloc = l;
	if (curloc != lastloc) lastalign = -1;

	switch (l)
	{
	case CURRENT:
		return ( retval );

	case PROG:
		lastalign = -1;
		/* FALLTHRU */
	case ADATA:
	case CDATA:
	case DATA:
	case STRNG:
	case ISTRNG:
	case CSTRNG:
	case EH_RANGES:
	case EH_RELOC:
	case EH_OTHER:
	case CTOR:
		if (lastloc == l)
			break;
		outfile = textfile;
		print_location(l);
		break;

	case FORCE_LC(CDATA):
		if (lastloc == l)
			break;
		outfile = textfile;
		print_location(CDATA);
		break; 

	case UNK:
		break;

	default:
		cerror(gettxt(":692", "illegal location counter" ));
	}

	lastloc = l;
	return( retval );		/* declare previous loc. ctr. */
}

/* Can object of type t go in a register?  rbusy is array
** of registers in use.  Return -1 if not in register,
** else return register number.  Also fill in rbusy[].
*/
int
cisreg(t, rbusy)
TWORD t;
char rbusy[TOTREGS];
{
	int i;

	if (picflag)
		rbusy[BASEREG] = 1;

	if ( t & ( TSHORT | TUSHORT
		 | TINT   | TUNSIGNED
		 | TLONG  | TULONG
		 | TPOINT | TPOINT2) )
	{
		/* Have a type we can put in any register. */
		for (i = MAX_USER_REG; i >= MIN_USER_REG; --i)
			if (!rbusy[i]) break;
	
		/* If candidate is suitable number grab it, adjust rbusy[]. */
		if (i >= MIN_USER_REG) {
			rbusy[i] = 1;
			regvar |= RS_BIT(i);
			return( i );
		}
	}
	else if ( t & ( TCHAR | TUCHAR ) )
	{	/* Only possible reg for these types is %ebx */
		if (! rbusy[REG_EBX] ) {
			rbusy[REG_EBX] = 1;
			return( REG_EBX );
		}
	}
	else if (t & (TLLONG|TULLONG)) /* ugh.  need a pair */
	{
		for (i = MAX_USER_REG; i > MIN_USER_REG; --i)
			if (!rbusy[i] && !rbusy[i-1])
			{
				rbusy[i] = 1;
				regvar |= RS_BIT(i);
				i--;
				rbusy[i] = 1;
				regvar |= RS_BIT(i);
				return i;
			}
	}

	/* Bad type or no register to allocate. */
	return( -1 );
}

NODE *
clocal(p)			/* manipulate the parse tree for local xforms */
register NODE *p;
{
	register NODE *l, *r;

#if 0 /* No SC_MOEs live into CG */
	/* make enum constants into hard ints */

	if (p->in.op == ICON && p->tn.type == ENUMTY) {
	    p->tn.type = INT;
	    return( p );
	}
#endif

	/* The code to do long long UNARY MINUS turns out to be so ugly
	   that it is better overall to change the tree into a regular
	   MINUS from zero. */
	if (p->in.op == UNARY MINUS && p->in.type & (TLLONG|TULLONG)) {
		p->in.op = MINUS;
		p->in.right = p->in.left;
		p->in.left = l = talloc();
		l->tn.op = ICON;
		l->tn.type = p->in.type;
		l->tn.c.ival = num_0;
		l->tn.name = (char *)0;
		return p;
	}

	if (!asgbinop(p->in.op) && p->in.op != ASSIGN)
		return (p);
	r = p->in.right;
	if (optype(r->in.op) == LTYPE)
		return (p);
	l = r->in.left;
	if (r->in.op == QUEST ||
		(r->in.op == CONV && l->in.op == QUEST) ||
		(r->in.op == CONV && l->in.op == CONV &&
		l->in.left->in.op == QUEST))
				/* distribute assigns over colons */
	{
		register NODE *pwork;
		extern NODE * tcopy();
		NODE *pcpy = tcopy(p), *pnew;
		int astype = p->in.type;	/* remember result type of asgop */
#ifndef NODBG
		extern int xdebug;
		if (xdebug)
		{
			emit_str("Entering [op]=?: distribution\n");
			e2print(p);
		}
#endif
		pnew = pcpy->in.right;
		while (pnew->in.op != QUEST)
			pnew = pnew->in.left;
		/*
		* pnew is top of new tree
		*/

		/* type of resulting ?: will be same as original type of asgop.
		** type of : must be changed, too
		*/
		pnew->in.type = astype;
		pnew->in.right->in.type = astype;

		if ((pwork = p)->in.right->in.op == QUEST)
		{
			tfree(pwork->in.right);
			pwork->in.right = pnew->in.right->in.left;
			pnew->in.right->in.left = pwork;
			/* at this point, 1/2 distributed. Tree looks like:
			*		ASSIGN|ASGOP
			*	LVAL			QUEST
			*		EXPR1		COLON
			*			ASSIGN|ASGOP	EXPR3
			*		LVAL		EXPR2
			* pnew "holds" new tree from QUEST node
			*/
		}
		else
		{
			NODE *pholdtop = pwork;

			pwork = pwork->in.right;
			while (pwork->in.left->in.op != QUEST)
				pwork = pwork->in.left;
			tfree(pwork->in.left);
			pwork->in.left = pnew->in.right->in.left;
			pnew->in.right->in.left = pholdtop;
			/* at this point, 1/2 distributed. Tree looks like:
			*		ASSIGN|ASGOP
			*	LVAL			ANY # OF CONVs
			*			QUEST
			*		EXPR1		COLON
			*			ASSIGN|ASGOP	EXPR3
			*		LVAL		ANY # OF CONVs
			*			EXPR2
			* pnew "holds" new tree from QUEST node
			*/
		}
		if ((pwork = pcpy)->in.right->in.op == QUEST)
		{
			pwork->in.right = pnew->in.right->in.right;
			pnew->in.right->in.right = pwork;
			/*
			* done with the easy case
			*/
		}
		else
		{
			NODE *pholdtop = pwork;

			pwork = pwork->in.right;
			while (pwork->in.left->in.op != QUEST)
				pwork = pwork->in.left;
			pwork->in.left = pnew->in.right->in.right;
			pnew->in.right->in.right = pholdtop;
			/*
			* done with the CONVs case
			*/
		}
		p = pnew;
#ifndef NODBG
		if (xdebug)
		{
			emit_str("Leaving [op]=?: distribution\n");
			e2print(p);
		}
#endif
	}
	return(p);
}

save_return_value()
{
	extern TWORD get_ret_type();
	if (!picflag) return;
	switch(get_ret_type())
	{
	case TVOID: case TFLOAT: case TDOUBLE: case TLDOUBLE:
		return;
	}
	emit_str("	movl	%eax,%ecx\n");
}

void
restore_return_value()
{
	extern TWORD get_ret_type();
	if (!picflag) return;
	switch(get_ret_type())
	{
	case TVOID: case TFLOAT: case TDOUBLE: case TLDOUBLE:
		return;
	}
	emit_str("	movl	%ecx,%eax\n");
}

#ifdef FIXED_FRAME
static register_saves;
#endif

static void
save_callee_save_regs(stack_size)
OFFSET stack_size;
{
	if(register_saves) {
		int i;
#ifdef FIXED_FRAME
		if(fixed_frame()){
			OFFSET offset;
			offset = get_max_arg_byte_count() - stack_size;
			for(i = MAX_USER_REG; i >= MIN_USER_REG; i--)
				if((register_saves & RS_BIT(i)) && i != REG_EBP )
					fprintf(outfile,
						"\tmovl\t%s,%ld%s(%s)\n",
						rnames[i],
						offset -= 4,
						PLUS_FRAME_OFFSET_STRING,
						rnames[REG_ESP]);
				/* Hack to get saves in an order
				** optim recognizes.
				*/
			if(register_saves & RS_BIT(REG_EBP))
					fprintf(outfile,
						"\tmovl\t%s,%ld%s(%s)\n",
						rnames[REG_EBP],
						offset -= 4,
						PLUS_FRAME_OFFSET_STRING,
						rnames[REG_ESP]);
		}
		else
#endif
			for(i = MAX_USER_REG; i >= MIN_USER_REG; i--)
				if(register_saves & RS_BIT(i))
					fprintf(outfile,"\tpushl\t%s\n",rnames[i]);
	}
}

	/*
	** Note that this function is called before the corresponding
	** save_callee_save_regs().  Consequently, this is
	** where we grow the stack to get the space required
	** for the saves.
	*/
static void
restore_callee_save_regs(stack_size)
OFFSET stack_size; /* Size of stack LESS the arg passing area */
{
	OFFSET offset = 0;
	register_saves = 0;
			/* Reverse the register saves */
	if (request[REG_EBX] && !(picflag && gotflag == NOGOTREF)) {
		register_saves |= RS_BIT(REG_EBX);
		offset += 4;
	}
	if(request[REG_ESI] || regstused & RS_BIT(REG_ESI)) {
		register_saves |= RS_BIT(REG_ESI);
		offset += 4;
	}
	if(request[REG_EDI] || regstused & RS_BIT(REG_EDI)) {
		register_saves |= RS_BIT(REG_EDI);
		offset += 4;
	}
	if(request[REG_EBP]) {
		register_saves |= RS_BIT(REG_EBP);
		offset += 4;
	}

	if(register_saves) {
		int i;
#ifdef FIXED_FRAME
		if(fixed_frame()){
			set_max_arg_byte_count(offset + get_max_arg_byte_count());
			offset = -(offset+stack_size);
			if(register_saves & RS_BIT(REG_EBP)) {
				fprintf(outfile, "\tmovl\t%ld%s(%s),%s\n",
				offset,
				PLUS_FRAME_OFFSET_STRING,
				rnames[REG_ESP],
				rnames[REG_EBP]);
				offset += 4;
			}
			for(i = MIN_USER_REG; i <= MAX_USER_REG; i++)
			if((register_saves & RS_BIT(i)) && i != REG_EBP) {
				fprintf(outfile, "\tmovl\t%ld%s(%s),%s\n",
				offset,
				PLUS_FRAME_OFFSET_STRING,
				rnames[REG_ESP],
				rnames[i]);
				offset += 4;
			}
		}
		else
#endif
			for(i = MIN_USER_REG; i <= MAX_USER_REG; i++)
				if(register_saves & RS_BIT(i))
					fprintf(outfile,"\tpopl\t%s\n",rnames[i]);

		}
}


static int	toplab;
static int	botlab;
static int	piclab, piclab2;
static int	picpc;		/* stack temp for pc */
static int	efoff[TOTREGS];

static int	alt_botlab;
static int	parms_in_regs; /* bit vector */
static char	*alt_entry_label;

void
efcode(p)			/* wind up a function */
NODE *p;
{
	extern int strftn;	/* non-zero if function is structure function,
				** contains label number of local static value
				*/
	register int i;
	OFFSET stack, alloc_stack;
	int scratch;

	if (p->in.name)
		memcpy(request, p->in.name, sizeof(request));
	if (picflag)
                request[BASEREG] = 1;

	stack = -(p->tn.c.off) * SZCHAR;

	deflab(retlab);
	if (callflag)
	{
		int temp;

		emit_str("/ASM\n");
		temp = getlab();
		print_location(PROG);
		emit_str("      pushl   %eax\n");
		fprintf(outfile, "      pushl   $.L%d\n", temp);
		deflab(temp);
		if (picflag) {
			fprintf(outfile,"call	 *_epilogue@GOT(%s)\n", rnames[BASEREG]);
			gotflag |= GOTREF;
		}
		else {
			emit_str("  call    _epilogue\n");
		}
		emit_str("      addl    $4, %esp\n");
		emit_str("      popl    %eax\n");
		emit_str("/ASMEND\n");
        }

#ifdef NVLT
	if (nvltflag == 3)
	{
		int temp;

		emit_str("/ASM\n");
		temp = getlab();
		emit_str("\t.text\n");
		emit_str("\tpushl\t%eax\n");		/* return  
value	*/

		fprintf(outfile, "\tpushl\t$0x%x\n",
			nvltMask | NVLTT_Leave);

		deflab(temp);
		if (picflag) {

			fprintf(outfile,"\tcall\t*NVLTleave@GOT(%s)\n",  
rnames[BASEREG]);
			gotflag |= GOTREF;
		}
		else {
			emit_str("\tcall\tNVLTleave\n");
		}
		emit_str("\taddl\t$4, %esp\n");
		emit_str("\tpopl\t%eax\n");
		emit_str("/ASMEND\n");
        }
#endif NVLT

	restore_globals();

	for (i = REG_EDI; i >= REG_EBX; i--)
		if (request[i]) {
			stack += SZINT;
			efoff[i] = -(stack/SZCHAR);
		}

	stack = -(p->tn.c.off) * SZCHAR;
	SETOFF(stack, ALSTACK);

	alloc_stack = stack/SZCHAR;

		/* Next line may grow the stack */
	restore_callee_save_regs(alloc_stack);

#ifdef FIXED_FRAME
	if(fixed_frame()) {
		fprintf(outfile,
			"\taddl\t$%s,%%esp\n",
			FRAME_OFFSET_STRING);
	}
	else
#endif
	fprintf(outfile,"\tleave\n");
	fprintf(outfile,"\tret/%d\n",biggest_return);

	/*
	 * The entry sequence according to the 387 spec sheet is always
	 * faster (by 4 cycles!) to do a push/movl/sub rather than an
	 * enter and occasionally a sub also.  Fastest enter is 10 cycles
	 * versus 2/2/2...
	 */
	deflab(botlab);
#ifndef	OLDTMPSRET
	if (strftn)
		emit_str("\tpopl\t%eax\n\txchgl\t%eax,0(%esp)\n");
#endif
#ifdef FIXED_FRAME
	if(fixed_frame())
		alloc_stack += get_max_arg_byte_count();
	else
#endif
	emit_str("\tpushl\t%ebp\n\tmovl\t%esp,%ebp\n");

#ifdef FIXED_FRAME
	if(fixed_frame())
		fprintf(outfile, "\tsubl\t$%s,%%esp\n", FRAME_OFFSET_STRING);
	else
#endif
		if(alloc_stack)
			fprintf(outfile, "\tsubl\t$%ld,%%esp\n", alloc_stack);

	save_callee_save_regs(alloc_stack);

#ifdef FIXED_FRAME
	if(fixed_frame())
		fprintf(outfile, "\t.set\t%s,%ld\n", FRAME_OFFSET_STRING, alloc_stack);
#endif

	regstused = RS_NONE;
	regvar = RS_NONE;
	if (gotflag != NOGOTREF) {
	    if(!parms_in_regs) {
	        fprintf(outfile,"\tcall	.L%d\n", piclab);
		deflab(piclab);
	        fprintf(outfile,"\tpopl	%s\n", rnames[BASEREG]);
	        if (gotflag & GOTSWREF) { /* Save %ebx in temp */
		    fprintf(outfile,"\tmovl\t%s,", rnames[BASEREG]);
		    printout_stack_addr(picpc);
		    fputc('\n',outfile);
	        }
	        fprintf(outfile,"\taddl	$_GLOBAL_OFFSET_TABLE_+[.-.L%d],%s\n",
				    piclab, rnames[BASEREG]);
		jmplab(toplab);
	    }
	    else { /* Both gotflag and parmsinregs */
		piclab2 = getlab();
		jmplab(piclab2);
	    }
	}
	else
		jmplab(toplab);
	if(parms_in_regs) {
	int i;
/* now do it all again for alt_entry */
	deflab(alt_botlab);
#ifndef	OLDTMPSRET
	if (strftn) {
		emit_str("\tpopl\t%eax\n\txchgl\t%eax,0(%esp)\n");
		cerror(gettxt(":0","can't do this bozo"));
	}
#endif

#ifdef FIXED_FRAME
	if(fixed_frame())
		fprintf(outfile, "\tsubl\t$%s,%%esp\n", FRAME_OFFSET_STRING);
	else {
#endif
		emit_str("\tpushl\t%ebp\n\tmovl\t%esp,%ebp\n");
		if(alloc_stack) {
			fprintf(outfile, "\tsubl\t$%ld,%%esp\n", alloc_stack);
		}
	}

	save_callee_save_regs(alloc_stack);


	i = 1; /* parm count */
	scratch = 1;
	while(parms_in_regs) {
		const char *reg;
		if(parms_in_regs % 2) {
			switch(scratch) {
			case 1:
				reg = rnames[REG_EDX];
				break;
			case 2:
				reg = rnames[REG_ECX];
				break;
			case 3:
				reg = rnames[REG_EAX];
				break;
			}
			scratch++;
			if(fixed_frame())
				fprintf(outfile, "\tmovl\t%s,%d+%s(%s)\n",
				reg, 4*i, FRAME_OFFSET_STRING, rnames[REG_ESP]);
			else
				fprintf(outfile, "\tmovl\t%s,%d(%s)\n",
				reg, 4*i + 4, rnames[REG_EBP]);
		}
		i++;
		parms_in_regs >>= 1;

	}

	regstused = RS_NONE;
	regvar = RS_NONE;
	if (gotflag != NOGOTREF)
	{
	    deflab(piclab2);
	    fprintf(outfile,"\tcall	.L%d\n", piclab);
	    deflab(piclab);
	    fprintf(outfile,"\tpopl	%s\n", rnames[BASEREG]);
	    if (gotflag & GOTSWREF) { /* Save %ebx in temp */
		fprintf(outfile,"\tmovl\t%s,", rnames[BASEREG]);
		printout_stack_addr(picpc);
		fputc('\n',outfile);
/*
		fprintf(outfile,"\tmovl\t%s,%d(%%ebp)\n", rnames[BASEREG],picpc);
*/
	    }
	    fprintf(outfile,"\taddl	$_GLOBAL_OFFSET_TABLE_+[.-.L%d],%s\n",
				piclab, rnames[BASEREG]);
	}
	jmplab(toplab);
	} /* if(parms_in_regs) */
}

void
set_alt_entry(char *function, int argset)
{
	parms_in_regs = argset;
	alt_entry_label = function;
}

void
bfcode(p)			/* begin function code. a is array of n stab */
NODE * p;
{
	extern void fp_init();
	retlab = getlab();	/* common return point */
	toplab = getlab();
	botlab = getlab();
	gotflag = NOGOTREF;
	if (picflag) {
		piclab = getlab();
		picpc  = freetemp(1) / SZCHAR;	/* stack temp for pc */
	}
	jmplab(botlab);
	if(parms_in_regs) {
		fprintf(outfile, "%sTWO_ENTRY_POINTS\n", COMMENTSTR);
		fprintf(outfile,"%s%s:\n",ALTENTRYSTR,alt_entry_label);
		alt_botlab = getlab();
		jmplab(alt_botlab);
	}
	deflab(toplab);
	load_globals();
	if (p->in.type == TSTRUCT)
	{
		strftn = 1;
		    /* 
		    ** Save struct return value address to temp.
		    ** Typically this is movl %eax,<str_spot>(%ebp)
		    ** or movl %eax,<str_spot>+FIXED_FRAME_OFFSET(%esp)
		    ** depending on the frame model.  See i386 ABI.
		    */
		fprintf(outfile,"	movl	%s,",rnames[AUXREG]);
		printout_stack_addr(str_spot);
		fprintf(outfile,"\n");
	}
	if (proflag)
	{
	        int temp;

		emit_str("/ASM\n");
		print_location(DATA);
		temp = getlab();
		emit_str("	.align	4\n");
		deflab(temp);
		emit_str("	.long	0\n");
		print_location(PROG);
		if (picflag) {
		    fprintf(outfile,"	leal	.L%d@GOTOFF(%s),%%edx\n",temp, rnames[BASEREG]);
		    fprintf(outfile,"	call	*_mcount@GOT(%s)\n", rnames[BASEREG]);
		    gotflag |= GOTREF;
		}
		else {
		    fprintf(outfile,"	movl	$.L%d,%%edx\n",temp);
		    emit_str("	call	_mcount\n");
		}
		emit_str("/ASMEND\n");
	}
        if (callflag)
        {
		int temp;

		emit_str("/ASM\n");
		temp = getlab();
		print_location(PROG);
		fprintf(outfile, "      pushl   $.L%d\n", temp);
		deflab(temp);
		if (picflag) {
			fprintf(outfile,"	call	*_prologue@GOT(%s)\n", rnames[BASEREG]);
			gotflag |= GOTREF;
		}
		else {
			emit_str("  call    _prologue\n");
		}
		emit_str("      addl    $4, %esp\n");
		emit_str("/ASMEND\n");
	}

#ifdef NVLT
        if (nvltflag == 3)
        {
		int temp;
		extern get_nargs();

		emit_str("/ASM\n");
		temp = getlab();
		emit_str("\t.text\n");

		/*
		 *	NVLTenter( (nvltMask | NVLTT_Enter), argno)
		 */
		fprintf(outfile, "\tpushl\t$%d\n", get_nargs());
		fprintf(outfile, "\tpushl\t$0x%x\n",
			nvltMask | NVLTT_Enter);
		deflab(temp);
		if (picflag) {
			fprintf(outfile,"\tcall *NVLTenter@GOT(%s)\n",
				rnames[BASEREG]);
			gotflag |= GOTREF;
		}
		else {
			emit_str("\tcall\tNVLTenter\n");
		}
		emit_str("\taddl\t$8,%esp\n"); /*  pop 2 arguments to NVLTenter	*/
		emit_str("/ASMEND\n");
	}
#endif NVLT

	fp_init();			/* Init the floating point stack */

}

static char f_l[FRAME_OFFSET_STRING_LEN];

#ifdef FIXED_FRAME
char *
frame_offset_string(init_flag)
int init_flag;
{
	static int count;
	if(init_flag)
		sprintf(f_l,"+.FSZ%d",count++);
	return &f_l[1];
}
#endif

void
begf(p) /*called for BEGF nodes*/
NODE *p;
{
                        /*save the used registers*/
	if (p->in.name)
        	memcpy(request, p->in.name, sizeof(request));
	else
		memset(request, 0, sizeof(request));
	if (picflag)
		request[BASEREG] = 1;

        (void) locctr(PROG);
        strftn = 0;
	biggest_return = 0;
#ifdef FIXED_FRAME
	if(fixed_frame()) {
		set_max_arg_byte_count(0L);
		(void)frame_offset_string(1); /* Init the name of this frame */
	}
#endif
}

void
myret_type(t)
TWORD t;
{
	if (t & (TLLONG | TULLONG))
	{
		biggest_return = 2;
	}
	else if (! (t & (TVOID | TFLOAT | TDOUBLE | TLDOUBLE | TFPTR)) )
	{
		if (!biggest_return)
			biggest_return = 1;
	}
}

void
sw_reduce(const INTCON *vp, int dflt) /* switch %edx:%eax -> %eax */
{
	INTCON lo, hi;

	lo = *vp;
	hi = *vp;
	(void)num_unarrow(&lo, 32);
	(void)num_lrshift(&hi, 32);
	if (num_ucompare(&lo, &num_0) != 0) {
		fprintf(outfile, "\tsubl\t$%s,%%eax\n", num_tohex(&lo));
		fprintf(outfile, "\tsbbl\t$%s,%%edx\n", num_tohex(&hi));
	} else if (num_ucompare(&hi, &num_0) != 0) {
		fprintf(outfile, "\tcmpl\t$%s,%%edx\n", num_tohex(&hi));
	} else {
		emit_str("\ttestl\t%edx,%edx\n");
	}
	fprintf(outfile, "\tjnz\t%s%d\n", LABSTR, dflt);
}

void		
sw_direct(NODE *lp, NODE *hp, TWORD type, unsigned long range, int dflt)
{
	static const char nonpic[] = "\t.long\t%s%d\n";
	static const char picfmt[] = "\t.long\t%s%d-%s%d\n";
	unsigned long val, cnt;
	const char *fmt;
	INTCON low, tmp;
	int tbl, lab;

	low = lp->in.c.ival;
	if (type & (TLLONG|TULLONG)) {
		sw_reduce(&low, dflt);
	} else if (num_ucompare(&low, &num_0) != 0) {
		tmp = low;
		(void)num_unarrow(&tmp, 32);
		fprintf(outfile, "\tsubl\t$%s,%%eax\n", num_tohex(&tmp));
	}
	fprintf(outfile, "\tcmpl\t$%l#x,%%eax\n\tja\t%s%d\n",
		range, LABSTR, dflt);
	tbl = getlab();
	if (picflag) {
		fmt = picfmt;
		fprintf(outfile, "\tleal\t%s%d@GOTOFF(%s)",
			LABSTR, tbl, rnames[BASEREG]);
		emit_str(",%edx\n\tmovl\t(%edx,%eax,4),%eax\n\taddl\t");
		printout_stack_addr(picpc);
		emit_str(",%eax\n\tjmp\t*%eax\n");
		gotflag |= GOTSWREF;
	} else {
		fmt = nonpic;
		fprintf(outfile, "\tjmp\t*%s%d(,%%eax,4)\n", LABSTR, tbl);
	}
	(void)locctr(FORCE_LC(CDATA));
	defalign(ALPOINT);
	emit_str("/SWBEG\n");
	deflab(tbl);
	/*
	* The list of SWCASE nodes that go into the table are between
	* lp and hp, inclusive.  Note: this loop modifies the constant
	* values as it goes.
	*/
	for (val = cnt = 0;; cnt++) {
		if (cnt == val) {
			fprintf(outfile, fmt, LABSTR, lp->in.sid, LABSTR, piclab);
			if (lp == hp)
				break;
			lp = lp->in.left;
			(void)num_ssubtract(&lp->in.c.ival, &low);
			(void)num_toulong(&lp->in.c.ival, &val);
			continue;
		}
		fprintf(outfile, fmt, LABSTR, dflt, LABSTR, piclab);
	}
	if (cnt != range)
		cerror(gettxt(":0", "wrong switch table size"));
	emit_str("/SWEND\n");
	(void)locctr(PROG);
}

p2done()
{
        char buf[BUFSIZ];
        int m;
	if (tmpfp)
	{
        	fclose(tmpfp);
        	if (!(tmpfp = fopen(tmpfn, "r")))
                	cerror(gettxt(":693","string file disappeared???"));
#ifndef RODATA
		(void) locctr(DATA);
#endif
        	while (m = fread(buf, 1, BUFSIZ, tmpfp))
                	fwrite(buf, 1, m, outfile);
        	(void) unlink(tmpfn);
	}
	return( ferror(outfile) );
}
