#ident	"@(#)optim:i386/local.c	1.2.14.55"

#include "optim.h" 	/* includes "defs.h" */
#include "optutil.h"
#include "sgs.h"
#include "paths.h"
#include "regal.h"
#include "storclass.h"
#include "debug.h"
#include <malloc.h>
void lazy_register_save();
static void init_globals();
static void replace_asz();
#if EH_SUP
static void force_frame();
#endif
static void unmark_intrinsics();
extern void rmrdmac();
extern void init_regals();
extern void init_assigned_params();
extern void save_regal();
extern int double_params();
extern void estimate_double_params();
extern void loop_descale();
extern void fpeep();
extern void fp_loop();
extern void loop_decmpl();
extern char *next_double_param();
extern void set_param2auto();
extern int is_double_param_regal();
extern NODE *prepend();
extern void init_ebboptim();
extern void init_loops();
extern void init_optutil();
extern void init_regal();
extern void init_reg_bits();
extern void optim_rmbrs();
static void match_ind_calls();
static void restore_ind_calls();

typedef struct asz_s {
	int name;
	int size;
	struct asz_s *next;
} asz_element;

#define ASDATE ""

static asz_element asz_list = { 0, 0, NULL};
static char * linptr, * line;	/* pointers to current statement. linptr
				   moves, line doesn't */
static int opn;			/* current operand, starting at 1 */ 
boolean found_safe_asm = false;
int set_FSZ = true;
int in_intrinsic = false;
int in_safe_asm = false;
static int set_nopsets = false;
static int lineno = IDVAL;
static int first_src_line = 0;	/* Value is set each time we see a
				   ..FS.. label */
int auto_elim=0;		/* true if function has no autos
					   or all autos have been placed
					   into registers by raoptim.*/
int fp_removed =0;		/* true if remove_enter_leave was done */
boolean no_safe_asms = false;    /* for treating safe asms as none safe */
int double_aligned;
unsigned int args_for_regs;
#ifdef DEBUG
static int d=0;
int min=0, max=0, not=0; /* first and last functions to be optimized */
int opt_idx = 0;  /* number of current optimization */
int second_idx = 0; /* number of current opportunity within optimization.*/
int first_opt=0,last_opt=0; /* first and last optimizations to execute */
int start =0,finish =0; /* first and last loops to be loop-optimized */
int do_getenv = 0; /* flag whether or not to read the above from environment */
int min5=0, max5=0; /*first and last function to be pentium otimized*/
int r5,rblend; /* globals to enable per function change of target cpu */
int skip_fp_elim = 0; /* skip over fp elimination; enable printing when cant*/
static void get_env(); /* read limits from environment */
extern int sflag;
extern int bflag;
void calc_frame_offsets();
#endif

#if EH_SUP
int try_block_nesting = 0;
int try_block_index = 0;
#endif

int first_run =1;

/*These global variables handle branch knowledge. We mark predictable branches
**by having their op2 point to the comment "/predictable", flacky one point
**to "/flacky" and unk ones point to "/unk". Initial knoweldge comes in 
**acomp comments, as analysed by amigo.
*/

int last_branch = 0;

unsigned int regs_for_out_going_args = 0;
unsigned int regs_for_incoming_args = 0;

int numauto;		/* number of bytes of automatic vars. */
static int numnreg;		/* number of registers */
static int autos_pres_el_done = false; /*enter_leave remove in presence of
										 automatic variables.           */

enum CC_Mode ccmode = Ansi;/* compilation mode */      

extern void	ldanal();

#define	BIGREGS	16	/* Any number that signifies many registers */
#define	BIGAUTO	(1<<30)	/* Any number that signifies many auto vars */

static char *optbl[] = {
	"aaa", "aad", "aam", "aas", "adc",
	"adcb", "adcl", "adcw", "addb", "addl",
	"addw", "andb", "andl", "andw", "arpl",
	"bound", "boundl", "boundw", "bsf", "bsfl",
	"bsfw", "bsr", "bsrl", "bsrw", "bswap",
	"bt", "btc", "btcl", "btcw", "btl",
	"btr", "btrl", "btrw", "bts", "btsl",
	"btsw", "btw", "call", "call0", "call1", "call2","cbtw", "clc",
	"cld", "cli", "clr", "clrb", "clrl",
	"clrw", "cltd", "clts", "cmc", 
    "cmova", "cmovae", "cmovael","cmovaew", "cmoval", "cmovaw",
	"cmovb", "cmovbe", "cmovbel", "cmovbew", "cmovbl", "cmovbw",
	"cmovc", "cmovcl", "cmovcw", "cmove", "cmovel", "cmovew",
	"cmovg", "cmovge", "cmovgel", "cmovgew", "cmovgl", "cmovgw",
	"cmovl", "cmovle", "cmovlel", "cmovlew", "cmovll", "cmovlw",
	"cmovna", "cmovnae", "cmovnael", "cmovnaew", "cmovnal", "cmovnaw",
	"cmovnb", "cmovnbe", "cmovnbel", "cmovnbew", "cmovnbl", "cmovnbw",
	"cmovnc", "cmovncl", "cmovncw", "cmovne", "cmovnel", "cmovnew",
	"cmovng", "cmovnge", "cmovngel", "cmovngew", "cmovngl", "cmovngw",
	"cmovnl", "cmovnle", "cmovnlel", "cmovnlew", "cmovnll", "cmovnlw",
	"cmovno", "cmovnol", "cmovnow", "cmovnp", "cmovnpl", "cmovnpw",
	"cmovns", "cmovnsl", "cmovnsw", "cmovnz", "cmovnzl", "cmovnzw",
	"cmovo", "cmovol", "cmovow", "cmovp", "cmovpe", "cmovpel",
	"cmovpew", "cmovpl", "cmovpo", "cmovpol", "cmovpow", "cmovpw",
	"cmovs", "cmovsl", "cmovsw", "cmovz", "cmovzl", "cmovzw",
	"cmp", "cmpb", "cmpl", "cmps", "cmpsb", "cmpsl",
	"cmpsw", "cmpw", "cmpxchg", "cmpxchgb", "cmpxchgl",
	"cmpxchgw", "cts", "cwtd", "cwtl", "daa",
	"das", "dec", "decb", "decl", "decw",
	"div", "divb", "divl", "divw", "enter",
	"esc", "f2xm1", "fabs", "fadd", "faddl",
	"faddp", "fadds", "fbld", "fbstp", "fchs",
	"fclex",
	"fcmovb", "fcmovbe", "fcmove", "fcmovnb", "fcmovnbe", "fcmovne",
	"fcmovnu", "fcmovu",
	"fcom", "fcoml", "fcomp", "fcompl",
	"fcompp", "fcomps", "fcoms", "fcos", "fdecstp",
	"fdiv", "fdivl", "fdivp", "fdivr", "fdivrl",
	"fdivrp", "fdivrs", "fdivs", "ffree", "fiadd",
	"fiaddl", "ficom", "ficoml", "ficomp", "ficompl",
	"fidiv", "fidivl", "fidivr", "fidivrl", "fild",
	"fildl", "fildll", "fimul", "fimull", "fincstp",
	"finit", "fist", "fistl", "fistp", "fistpl",
	"fistpll", "fisub", "fisubl", "fisubr", "fisubrl",
	"fld", "fld1", "fldcw", "fldenv", "fldl",
	"fldl2e", "fldl2t", "fldlg2", "fldln2", "fldpi",
	"flds", "fldt", "fldz", "fmul", "fmull",
	"fmulp", "fmuls", "fnclex", "fninit", "fnop",
	"fnsave", "fnstcw", "fnstenv", "fnstsw", "fpatan",
	"fprem", "fprem1", "fptan", "frndint", "frstor",
	"fsave", "fscale", "fsetpm", "fsin", "fsincos",
	"fsqrt", "fst", "fstcw", "fstenv", "fstl",
	"fstp", "fstpl", "fstps", "fstpt", "fsts",
	"fstsw", "fsub", "fsubl", "fsubp", "fsubr",
	"fsubrl", "fsubrp", "fsubrs", "fsubs", "ftst",
	"fucom", "fucomp", "fucompp", "fwait", "fxam",
	"fxch", "fxtract", "fyl2x", "fyl2xp1", "hlt",
	"idiv", "idivb", "idivl", "idivw", "imul",
	"imulb", "imull", "imulw", "in", "inb",
	"inc", "incb", "incl", "incw", "inl",
	"ins", "insb", "insl", "insw", "int",
	"into", "invd", "invlpg", "inw", "iret",
	"ja", "jae", "jb", "jbe", "jc",
	"jcxz", "je", "jg", "jge", "jl",
	"jle", "jmp", "jna", "jnae", "jnb",
	"jnbe", "jnc", "jne", "jng", "jnge",
	"jnl", "jnle", "jno", "jnp", "jns",
	"jnz", "jo", "jp", "jpe", "jpo",
	"js", "jz", "lahf", "lar", "larl",
	"larw", "lcall", "lcall0", "lcall1", "lcall2",
	"lds", "ldsl", "ldsw",
	"lea", "leal", "leave", "leaw", "les",
	"lesl", "lesw", "lfs", "lfsl", "lfsw",
	"lgdt", "lgs", "lgsl", "lgsw", "lidt",
	"ljmp", "lldt", "lmsw", "lock", "lods",
	"lodsb", "lodsl", "lodsw", "loop", "loope",
	"loopne", "loopnz", "loopz", "lret", "lsl",
	"lsll", "lslw", "lss", "lssl", "lssw",
	"ltr", "mov", "movb", "movl", "movs",
	"movsb", "movsbl", "movsbw", "movsl", "movsw",
	"movswl", "movw", "movzbl", "movzbw", "movzwl",
	"mul", "mulb", "mull", "mulw", "neg",
	"negb", "negl", "negw", "nop", "not",
	"notb", "notl", "notw", "or", "orb",
	"orl", "orw", "out", "outb", "outl",
	"outs", "outsb", "outsl", "outsw", "outw",
	"pop", "popa", "popal", "popaw", "popf",
	"popfl", "popfw", "popl", "popw", "push",
	"pusha", "pushal", "pushaw", "pushf", "pushfl",
	"pushfw", "pushl", "pushw", "rcl", "rclb",
	"rcll", "rclw", "rcr", "rcrb", "rcrl",
	"rcrw", "rep", "repe", "repne", "repnz",
	"repz", "ret", "rol", "rolb", "roll",
	"rolw", "ror", "rorb", "rorl", "rorw",
	"sahf", "sal", "salb", "sall", "salw",
	"sar", "sarb", "sarl", "sarw", "sbb",
	"sbbb", "sbbl", "sbbw", "scab", "scal",
	"scas", "scasb", "scasl", "scasw", "scaw",
	"scmp", "scmpb", "scmpl", "scmpw", "seta",
	"setae", "setb", "setbe", "setc", "sete",
	"setg", "setge", "setl", "setle", "setna",
	"setnae", "setnb", "setnbe", "setnc", "setne",
	"setng", "setnl", "setnle", "setno", "setnp",
	"setns", "setnz", "seto", "setp", "setpe",
	"setpo", "sets", "setz", "sgdt", "shl",
	"shlb", "shld", "shldl", "shldw", "shll",
	"shlw", "shr", "shrb", "shrd", "shrdl",
	"shrdw", "shrl", "shrw", "sidt", "sldt",
	"slod", "slodb", "slodl", "slodw", "smov",
	"smovb", "smovl", "smovw", "smsw", "ssca",
	"sscab", "sscal", "sscaw", "ssto", "sstob",
	"sstol", "sstow", "stc", "std", "sti",
	"stos", "stosb", "stosl", "stosw", "str",
	"sub", "subb", "subl", "subw", "test",
	"testb", "testl", "testw", "verr", "verw",
	"wait", "wbinvd", "xadd", "xaddb", "xaddl",
	"xaddw", "xchg", "xchgb", "xchgl", "xchgw",
	"xlat", "xor", "xorb", "xorl", "xorw"
};

#ifdef FLIRT

static char *optim_tbl[] = {
	"o_bbopt1", "o_bbopt2", "o_clean_zero", "o_comtail", "o_const_index", 
	"o_detestl", "o_fp_loop",
	"o_fpeep", "o_fpeep01", "o_fpeep02", "o_fpeep03", "o_fpeep04",
	"o_fpeep05", "o_fpeep06", "o_fpeep07", "o_fpeep08", "o_fpeep09",
	"o_fpeep10", "o_fpeep11", "o_fpeep12", "o_imm_2_reg", "o_imull",
	"o_insert_moves", "o_loop_decmpl",
	"o_loop_descale", "o_mrgbrs", "o_optim", "o_peep",
	"o_postpeep", "o_raoptim", "o_regals_2_index_ops",
	"o_remove_overlapping_regals", "o_reord", "o_reordtop",
	"o_replace_consts", "o_replace_regals_w_regs", "o_rm_all_tvrs1",
	"o_rm_all_tvrs2", "o_rm_all_tvrs3",
	"o_rm_all_tvrs4", "o_rm_dead_insts", 
	"o_rm_mvmem", "o_rm_mvxy", "o_rm_rd_const_load", 
	"o_rm_tmpvars1", "o_rm_tmpvars2", "o_rmbrs", "o_rmlbls", "o_rmrdld",
	"o_rmrdmv", "o_rmrdpp", "o_rmunrch", "o_schedule",
	"o_setauto_reg", "o_setcond", "o_stack_cleanup",
	"o_sw_pipeline", "o_try_again_w_rt_disamb", "o_try_backward",
	"o_try_forward", "o_xchg_save_locals", "o_zvtr"
};

#define optim_num (sizeof(optim_tbl) / sizeof(char *)) /* num of optim's */

#endif

# define numops (sizeof(optbl) / sizeof(char *)) /* number of mnemonics */

extern char *getenv();
#ifdef DEBUG
char *get_label();
#endif
extern char *tempnam();
static FILE *tmpopen();
static FILE *stmpfile;	/* Temporary storage for switch tables that are in the text
			 * section. The tables are collected and printed at
			 * the end of a function. */
static char tmpname[50];	/* name of file for storing switch */
static FILE *atmpfile; /* Temporary storage for input while scanning for presence of
			 * #ASM in function if aflag on */
static char atmpname[50];

int Aflag = false;
int asmflag = false;	/* indicates whether an 'asm' has been encountered */
static long asmotell;	/* location in the output file where the last function ended */
int aflag = false;	/* indicates whether -a flag was entered, if true
			   no atempt will be made to optimize functions
			   containing ASMs */

			/* Next three flags apply globally -- set on
			   command line by  -_r, -_e, -_s */

static int never_register_allocation = false;
static int never_enter_leave = false; 
static int never_stack_cleanup = false;
#if EH_SUP
static int must_use_frame = false;
#endif

			/*
			** Next three flags apply on a per function basis:
			** suppress ( only under -Xt ) register allocation
			** if fn contains a setjmp.  Similarly, suppress
			** enter/leave removal if a function contains
			** a longjmp (for -Xa and -Xt as well)
			** Lastly,  stop the cleanup of the stack 
			** for a given function if it calls alloca().
			** These flags reinitialized for each function
			** to the value of the global flags declared above.
			*/

static int suppress_register_allocation = false;
/* static */ int suppress_enter_leave = false;
static int suppress_stack_cleanup = false;

int tflag = false; 	/* suppress removal of redundant loads (rmrdld())? */
int Tflag = false; 	/* trace removal of redundand loads routine? */
static boolean identflag = false;/* output .ident string? */

static boolean new_csline,get_first_src_line = false, get_lineno = false;
static boolean inswitch = false;/* currently in switch table */

enum Section section=CStext, prev_section;      /* Control sections. */

boolean swflag = false;		/* switch table appears in function */
int rvregs = 1;			/* # registers used to hold return value */
/* function declarations */
extern int unlink();

extern struct regal *lookup_regal();
extern int raoptim();
extern void rmrdld();
extern void wrapup();	/* print unprocessed text and update statistics file */
extern void dstats();
extern void loop_regal();
extern void const_index();

extern char *getstmnt(); /* also called from debug.c */
/* Input function (getline()), lifted from HALO */
static char *ExtendCopyBuf(); /* Used only by getstmnt */
static void eliminate_ebp();
void set_refs_to_blocks(); /* connect switch tables names to blocks */
static void hide_safe_asm();
static void recover_safe_asm();
static void reset_crs();
#ifdef DEBUG
static int verify_safe_asm();
#endif

int plookup();			/* look up pseudo op code */
#ifdef FLIRT
void flirt();
#endif

static void parse_com();
static void parse_branch();
static void parse_regal();		/* Parses a #REGAL line. */
static void parse_alias();		/* Parses a #ALIAS line. */
static void parse_op();
static void parse_pseudo();
static void fixed_frame_data();
static int remove_enter_leave();
static void xchg_save_locals();
static enum Section parse_section();	/* Parse the argument to .section. */
static int lookup();			/* look up op code ordinal */
#ifdef FLIRT
static int search();            /* search for optimization ordinal */
#endif
static void asmopen();	/* opens the temp file for storing input while looking for 'asm' */
int putasm();	/* writes to temp file for 'asm' processing */
static void asmchk();	/* checks for 'asm' in files and undoes code movement */
static void setautoreg();	/* set indicator for number of autos/regs */
static void reordtop();
static void reord_double_top();
static void reordbot();
static void reordbot_fs();
static void eliminate_duplicate_part();
static void copyafter();
static void putstr();
static void prstr();
static void filter();
void sets_and_uses();
static void stack_cleanup();
extern void peep();
extern void imull();
static void forget_bi();
extern int check_double(), replace_consts();
extern SWITCH_TBL * get_base_label();
extern void bldgr();
extern void schedule(), backalign(), sw_pipeline(), rm_tmpvars(), rmrdpp(),
			postpeep(), ebboptim(), setcond(), rm_all_tvrs(), bbopt(),
			clean_zero(), minimize_fsz() ;
extern char *strstr();
static char *switch_label;
const char frame_string[] = ".FSZ";
const char regs_string[] = ".RSZ";


typedef struct list_s {
	struct list_s *next;
	unsigned long val;
	char *label;
	} list;

static char * seen_ro = NULL;
list l0;
static list *last_list = &l0;

FUNCTION_DATA func_data;
unsigned int frame_reg;

FUNCTION_DATA init_func_data = { 0,0,0,0,0,0};

typedef struct suppress_list_s {
	struct suppress_list_s* next;
	char* optimization_name;
	} suppress_list_type;

static suppress_list_type* suppress_list;

/* calling all updating functions to update variables depending on fixed_stack */
void 
init_all()
{
	init_ebboptim();
	init_loops();
	init_optutil();
	init_regal();
}


static void
addlist(s,val) char *s; unsigned long val;
{
	last_list->next = (list *) malloc(sizeof(list));
	last_list = last_list->next;
	last_list->label = strdup(s);
	last_list->val = val;
	last_list->next = NULL;
}/*end addlist*/

boolean
is_pos_ro(s,val) char *s ;unsigned long *val;
{
list *l;
	for (l = l0.next; l; l = l->next) {
		if (!strcmp(s,l->label)) {
			*val = l->val;
			return true;
			
		}
	}
	return false;
}/*end is_pos_ro*/

#ifdef FLIRT

void flirt(code,occ)
int code;       /* code of optimization which called FLiRT */
int occ ;       /* occurence of optimization */
{
	static times=0;

	if (code==CHECK)
	{
		fprintf(stderr,"\n");
		if (occ==0)
			 fprintf(stderr,"doing %s only once\n",optim_tbl[LOC]+2);
   		else {
			  ++times;
			  if (times==1)
				   fprintf(stderr,"doing %s: \n",optim_tbl[LOC]+2);
			  fprintf (stderr,"Invocation no.: %d\nchange no.: %d\n",occ,times);
			 }
	 }
	 return;
}
#endif

#ifdef EMON
static void pcg_begin() , pcg_end(), pcg_routine_end(), add_emons();
static int function_index;
static char *input_file_name = NULL;
#endif

	void
yylex()
{
	register char * s;			/* temporary string ptr */
	register char * start;		/* another temporary string ptr */
	int first_label = 1;
	extern int pic_flag;
	char *s_alloca = pic_flag ? "alloca@PLT" : "alloca";
	char *s_setjmp = pic_flag ? "setjmp@PLT" : "setjmp";
	char *s_sigsetjmp = pic_flag ? "sigsetjmp@PLT" : "sigsetjmp";
	char *s_longjmp = pic_flag ? "longjmp@PLT" : "longjmp";

#ifdef DEBUG
	r5 = target_cpu == P5; /* remember them to enable a per function change from env*/
	rblend = target_cpu == blend;
#endif
#ifdef EMON
	if (dflag) pcg_begin();
#endif

	set_FSZ = true;
	init_regals(); /* clear regals data structure before reading .s file */
	init_assigned_params();
	init_globals();

	if( aflag ) asmopen();		/* open temp file for asm treatment */
	line = linptr = getstmnt();
	while ( linptr 
		&& ( !aflag || putasm(linptr) ))
	{
	if (section == CSdebug) {	/* hook to handle an entire debug
					   section.  We do this, because
					   parse_debug wants to do its own
					   I/O and parsing */
		parse_debug(linptr);
	}		
	else {
		switch (*linptr)		/* dispatch on first char in line */
		{
		case '\n':			/* ignore blank lines */
		case '\0':
			break;
			
		case CC:			/* check for special comments */
			parse_com(linptr);
			break;
	
		default:			/* label */
			s = strchr(linptr,':');	/* find : */
			if (s == NULL)		/* error if not found */
			  fatal(__FILE__,__LINE__,"Bad input format\n");
			*s++ = '\0';		/* change : to null, skip past */
			if (section == CStext) {
				if(strncmp(linptr,"..LN",4) == 0)  /* linenumber label */
					get_lineno = true;
				else if(strncmp(linptr,"..FS",4) == 0)  
					/* 1st source line for function */
					get_first_src_line = true;
				else {
					if(first_label) {
						save_text_begin_label(linptr);
						first_label = 0;
					}
					applbl(linptr,s-linptr);  /* append label node */
					lastnode->uniqid = IDVAL;
#if EH_SUP
					if (linptr[1] == '.' && linptr[2] == 'E') {
						if (linptr[3] == 'C')
				   			addref(linptr,(unsigned int)(s-linptr+1),NULL);
						if (linptr[3] == 'T' || linptr[3] == 'R') {
							lastnode->op = EHLABEL;
				   			addref(linptr,(unsigned int)(s-linptr+1),NULL);
						}
					}
#endif
				}
			}
			else if(section != CSline) {
				printf("%s:\n",linptr);
				if (is_read_only(linptr)) {
					seen_ro = strdup(line);
				}
				addref(linptr,(unsigned int)(s-linptr),NULL);
				if (inswitch) { /*the jump table label */
					switch_label = getspace(LABELSIZE);
					(void) strcpy(switch_label,linptr);
				}
			}
		linptr = s;
			continue;			/* next iteration */
	/* handle more usual case of line beginning with space/tab */
		case ' ':
		case '\t':
			s = linptr;
				/* linptr now points to beginning of line after label */
			SkipWhite(s);
			
			switch(*s)			/* dispatch on next character */
			{
			case '\0':			/* line of white space */
			case CC:			/* comment */
			case '\n':			/* new line */
			break;			/* ignore */
			
			case '.':			/* start of pseudo-op */
			parse_pseudo(s);	/* do pseudo-ops */
			break;
	
			default:			/* normal instruction not */
			if (section != CStext) { /* in .text section this is weird case */
				if (section != CSline) printf("\t%s\n",s); /* just write line */
			}
			else			/* normal instruction in text */
			{
				char lastchar;
				int opc, m;
				NODE *p;

				for (start = s; isalnum(*s); s++)
				;		/* skip over instruction mnemonic */
						/* start points to mnemonic */
	
				lastchar = *s;
				*s = '\0';	/* demarcate with null */
	
				if ((opc = lookup(start,&m)) == OTHER)
					/* save funny instruction */
					p = Saveop(0,strdup(start),0,opc);
				else
					/* save normal inst */
					p = Saveop(0,optbl[m],0,opc);
				*s = lastchar;
	
				SkipWhite(s);

				/* Check for special versions of CALL and LCALL
				   opcodes; set number og return value registers
				   and convert opcode back to generic opcode. */
				switch (opc) {
					case CALL_2:
						p->rv_regs++;
						/* FALLTHRU */
					case CALL_1: case CALL:
						p->rv_regs++;
						/* FALLTHRU */
					case CALL_0:
						chgop(p, CALL, "call");
						opc = CALL;
						break;

					case LCALL_2: case LCALL:
						p->rv_regs++;
						/* FALLTHRU */
					case LCALL_1:
						p->rv_regs++;
						/* FALLTHRU */
					case LCALL_0:
						chgop(p, LCALL, "lcall");
						opc = LCALL;
						break;
				}  /* switch */
				   
				/* Check if this is a call to a local label,
				   if so we need to prevent it from being
				   removed.  This only arises when the compiler
				   is generating PIC.
			   
				   Also check for call to setjmp if compiling
				   in transition mode (-Xt).  Such calls must
				   disable register allocation. */

				if (opc == JMP && (s[0] == '*')
				 &&  (s[1] == '%')) {
					suppress_enter_leave = true;
				}
				if( opc == CALL || opc == LCALL ) {
					if(*s == '.' ) {
					   char *p = s+1; /* point at 'L' */
					   while ( isalnum(*p) ) p++;
					   *p = '\0';
					   addref(s,(unsigned int)(p-s+1),NULL);
					}
					else if(*s == 's' &&
						((strcmp(s,s_sigsetjmp) == 0) || (strcmp(s,s_setjmp)==0))) {
						suppress_enter_leave = true;
						if (ccmode == Transition )
						suppress_register_allocation = true;
					}
					else if(*s == 'l' && (strcmp(s,s_longjmp) == 0)) {
						suppress_enter_leave = true;
					}
					else if(*s == 'a' && (strcmp(s,s_alloca) == 0)) {
						suppress_stack_cleanup = true,
						suppress_enter_leave = true;
					}
					/* hack to avoid confusing this function,
					   which makes assumptions about the frame
					   pointer of its caller */
				}
				opn = 1;		/* now save operands */
				parse_op(s);
			}
			break;
			}   /* end space/tab case */
		break;
		}	/* end first character switch */
	}	/* end if (section ...) ... else */
	line = linptr = getstmnt();
	}   /* end while */
	return;				/* just exit */
}

	static void
parse_com(s)
register char *s;
{

	switch(*++s){
	case 'A':	/* #ALIAS, #ASM, or #ASMEND ? */
		if(strncmp(s,"ALIAS",5) == 0){
			parse_alias(s+5);
			break;
		}
		else if (strncmp(s,"ALLOCA",6) == 0) {
			suppress_stack_cleanup = true;
			suppress_enter_leave = true;
			break;
		} else if(strncmp(s,"ASM",3) != 0)
			break;
			/* since #ASM processing chews up everything to #ASMEND ... */
		if(strncmp(s+3,"END",3) == 0)
			if (!in_safe_asm)
				fatal(__FILE__,__LINE__,"parse_com: unbalanced asm.\n");
			else {
				in_safe_asm = false;
				saveop(0,strdup(s-1),0,SAFE_ASM);
				break;
			}
		asmflag = true;		/* #ASM appears in function */
		/* here if #ASM<stuff> */
		s = linptr;
		do {
			while(*linptr != '\0') linptr++;
			/* *linptr++ = '\0'; take this out (psp) */
			saveop(0,strdup(s),0,ASMS);
			if(strncmp(s+1,"ASMEND",6) == 0) {
				break;
			}
		} while ( (line=linptr=getstmnt()) != NULL
			&& ( !aflag || putasm(s) ));
		if(linptr == NULL)
			fatal(__FILE__,__LINE__,"parse_com: unbalanced asm.\n");
		break;
		 case 'I':  /* inlined INTRINSIC function ? */
			 if (strncmp(s,"INTRINSICEND",12) == 0) {
				 in_intrinsic = false;
			 } else if (strncmp(s,"INTRINSIC",9) == 0) {
				 in_intrinsic = true;
			 }
			 break;
	 case 'B':  /* BRANCH ? */
		if (strncmp(s,"BRANCH:",7)) break;
		parse_branch(s+7);
		break;
#if EH_SUP
	 case 'C':
		if (!strncmp(s,"CATCH_HANDLERS",14)) {
			try_block_index++;
		}
		break;
#endif
	 case 'E':
		if (fixed_stack && !strncmp(s,"End STARG",9)) {
			saveop(0,s,9,TSRET);
		}
		break;
     case 'F':  /* FIXED_FRAME */
		if (strncmp(s,"FIXED_FRAME",11) != 0) break ;
		fixed_stack = 1 ;
		frame_reg = ESP;
		break;
	 case 'L':	/* #LOOP ? */
		if(strncmp(s,"LOOP",4) == 0) {
			s += 4;		/* enter it into the list. */
			SkipWhite(s);	/* Skip white space to retrieve arg.	*/
			saveop(0,s,3,LCMT);	/* append loop-cmt node with its arg */
			break;
		} else if (strncmp(s,"LIVE:",5) == 0) {
			s+=5;
			SkipWhite(s);
			regs_for_out_going_args |= setreg(s);
			break;
		} else break;
#if EH_SUP
	 case 'N':  /* NO_EXACT_FRAME ? */
		if (!strncmp(s,"NO_EXACT_FRAME",14))  {
			suppress_enter_leave = true;
			must_use_frame = true;
		}
		break;
#endif
	 case 'P':	/* POS_OFFSET ? */
		if (strncmp(s,"POS_OFFSET",10) != 0) break;
		if (lastnode == NULL) break;
		s += 10;
		SkipWhite(s);
		lastnode->ebp_offset = (int)strtoul(s,(char **)NULL,0);
		break;
	 case 'R':	/* #REGAL ? */
		if(strncmp(s,"REGAL",5) != 0) break;
		parse_regal(s+5);	/*Read NAQ info in #REGAL stmt*/
		break;
	 case 'S':	/* #SWBEG or #SWEND ? */
		if(strncmp(s,"SWBEG",5) == 0)
		  {
			start_switch_tbl();
			inswitch = true; 	/* Now in a switch table. */
			swflag = true;
			  }
		else if(strncmp(s,"SWEND",5) == 0) {
			end_switch_tbl(switch_label);
			inswitch = false;	/*Out of switch table.*/
		} else if (!strncmp(s,"SAFE_ASM",8)) {
			if (no_safe_asms) { /*treat safe asms as asms */
				asmflag = true;
				s = linptr;
				do {
					while(*linptr != '\0') linptr++;
						saveop(0,strdup(s),0,ASMS);
						if(strncmp(s+1,"ASMEND",6) == 0) {
							break;
					}
				} while ( (line=linptr=getstmnt()) != NULL
					&& ( !aflag || putasm(s) ));
				if(linptr == NULL)
					fatal(__FILE__,__LINE__,"parse_com: unbalanced asm.\n");
				break;
			} else {
				saveop(0,strdup(s-1),0,SAFE_ASM);
				in_safe_asm = true;
				found_safe_asm = true;
			}
		} else if (fixed_stack && !strncmp(s,"STARG",5)) {
			saveop(0,s,5,TSRET);
		} else if (strncmp(s,"SUPPRESS_",9) == 0) {
			/* optimization to be suppressed for this function */
			suppress_list_type* sl = (suppress_list_type*) getspace(sizeof(suppress_list_type));
			sl->next = NULL;
			sl->optimization_name = COPY(s+9, strlen(s)+1-9);
			if (suppress_list == NULL)
				suppress_list = sl;
			else {
				suppress_list_type* p = suppress_list;
				while (p->next != NULL)
					p = p->next;
				p->next = sl;
			}
		}
		break;
	 case 'T':
		if (!strncmp(s,"TWO_ENTRY_POINTS",16)) {
			func_data.two_entries = true;
			break;
		}
#if EH_SUP
		if (!strncmp(s,"TRY_BLOCK_START",15)) {
			try_block_nesting++;
			try_block_index++;
		}
		if (!strncmp(s,"TRY_BLOCK_END",13)) {
			try_block_nesting--;
		}
#endif
		if (lastnode == NULL)	/* #TMPSRET ? */
			break;
		if (strncmp(s,"TMPSRET",7) == 0)
			if (lastnode->op == PUSHL)	/* identify push of addr */
				saveop(2,"TMPSRET\0",8,TSRET);	/* for struct funct ret. */
		break;
	 case 'V':	/* #VOL_OPND ? */
		if(strncmp(s,"VOL_OPND", 8) != 0) break;
		s += 8;
		SkipWhite(s);
		if (lastnode == NULL)
			break;
		while( *s != '\0' && *s != '\n' ){
			/* set volatile bit in operand */
			mark_vol_opnd(lastnode,*s - '0');
			s++;
			SkipWhite(s);
			if( *s == ',' ){ 
				s++;
				SkipWhite(s);
			}
		}
		break;
	} /* end of switch(*++s) */
}

static void
parse_branch(char *t)
{
	SkipWhite(t);
	switch (*t) {
		case 'C': if (!strncmp(t,"CMP_PTR_ERR",11)) 
					last_branch = CMP_PTR_ERR;
				  else if (!strncmp(t,"CMP_FUNC_CONST",14))
					last_branch = CMP_FUNC_CONST;
				  else
					last_branch = 0;
				  break;
		case 'G': if (!strncmp(t,"GUARD_RET",9))
					last_branch = GUARD_RET;
				  else if (!strncmp(t,"GUARD_CALL",10))
					last_branch = GUARD_CALL;
#if 0
				  else if (!strncmp(t,"GUARD_LOOP",10))
					last_branch = GUARD_LOOP;
#endif
				  else
					last_branch = 0;
				  break;
		default: last_branch = 0;
	}
}

	static void
parse_regal( p ) 		/* read #REGAL comments */
register char *p;

{
 register struct regal *r;
 char *q;
 char *name;
 int len,rt;

	/* the formats recognized are:
	 * 1) #REGAL <wt> NODBL
	 * 2) #REGAL <wt> {AUTO,PARAM,RPARAM,EXTERN,EXTDEF,STATEXT,STATLOC} <name> <size> [FP]
	 *
	 * However only the following subset is used by the 386 optimizer:
	 * #REGAL <wt> {AUTO,PARAM,RPARAM} <name> <size>
	 * and the other formats are ignored.
	 */

			/* scan to estimator and read it */
	SkipWhite(p);
	strtoul( p, &q, 0 );
	if(p == q)
		fatal(__FILE__,__LINE__,"parse_regal: missing weight field\n");
	p = q;


			/* scan to storage class and read it */
	SkipWhite(p);
	rt = SCLNULL;
	switch(*p){
	case 'A':
		if(strncmp(p,"AUTO",4) == 0)
			rt = AUTO;
		break;
	case 'E':
		if(strncmp(p,"EXTDEF",6) == 0)
			return;			/* ignore */
		else if(strncmp(p,"EXTERN",6) == 0)
			return;			/* ignore */
		break;
	case 'N':
		if(strncmp(p,"NODBL",5) == 0)
			return;			/* ignore */
		break;
	case 'P':
		if(strncmp(p,"PARAM",5) == 0)
			rt = PARAM;
		break;
	case 'R':
		if(strncmp(p,"RPARAM",6) == 0)
			rt = RPARAM;
		break;
	case 'S':
		if(strncmp(p,"STATEXT",7) == 0)
			return;			/* ignore */
		else if(strncmp(p,"STATLOC",7) == 0)
			return;			/* ignore */
		break;
	}
	if( rt == SCLNULL )
	 fatal(__FILE__,__LINE__, "parse_regal:  illegal #REGAL type:\n\t%s\n",p);

			/* scan to name and delimit it */
	FindWhite(p);
	SkipWhite(p);
	name = p;
	FindWhite(p);
	*p++ = '\0';
	if (   ((q = (strchr(name,'('))) == NULL)
		|| (   (strncmp(q,"(%ebp)",6) != 0)
			&& (   !fixed_stack
				|| (strstr(name,".FSZ(%esp)") != 0)) ) )
		fatal(__FILE__,__LINE__,"parse_regal:  illegal name in #REGAL\n");
			/* only "nnn(%ebp) currently allowed */

			/* scan to length in bytes */
	SkipWhite(p);
	len = (int)strtoul( p, &q, 0 );
	if(p == q)
		fatal(__FILE__,__LINE__,"parse_regal: missing length\n");
	p = q;

			/* read floating point indicator */
	SkipWhite(p);
	if((p[0] != '\0' && p[0] == 'F') && (p[1] != '\0' && p[1] == 'P')) {
		/* keep the knowledge that this operand could be a regal, but do
		** not send it to raoptim() to assign it to a register.
		*/
		save_regal(name,len,true,false);
		return;			/* ignore */
	}

	if (rt == RPARAM) {
		save_assign(name,p);
		return; /*dont save_regal*/
	}

			/* install regal node */
	if ((r = lookup_regal(name,true)) == NULL)
		fatal(__FILE__,__LINE__,"parse_regal:  trouble installing a regal\n");
	r->rglscl = rt;
	r->rgllen = len;
 return;
}

	static void
parse_alias(s)
register char *s;

{
 extern struct regal *new_alias();
 char *name,*t;
 int len;
 register struct regal *a;

			/* scan to name and delimit it */
 SkipWhite(s);
 name = s;
 FindWhite(s);
 *s++ = '\0';
 if (strchr(name,'(') == NULL)
   return;		/* catch only AUTOs and PARAMs - "n(%ebp)" */


			/* scan to length in bytes */
 SkipWhite(s);
 len = (int)strtoul( s, &t, 0 );
 if(s == t)
   fatal(__FILE__,__LINE__,"parse_alias: missing length\n");
 s = t;

			/* read floating point indicator */
 SkipWhite(s);
 if( s[0] == 'F' && s[1] == 'P' ) {
   save_regal(name,len,false,false);
   return;
 }

			/* append node to list of aliases */
 a = new_alias();
 a->rglname = strdup(name);
 a->rgllen = len;

 return;
}

/* Calculate $n1[+|-]n2[+|-]n3... for example: $32+2-4 should be $30 */
/*
** Also handle $n1&n2 which is now introduced for shifts of 64 bit values.
**  - Note : This is a quick and dirty implementation for this special
**           case.  No attempt is being made for generic expressions and
**           the operator precedence issues that follow.  
*/ 
static char *
calc_v(t) char *t;
{
	char ch;
	char *s1 = ++t;
	long sum = 0;

	sum += (long) strtoul(s1,&s1,0);
	while ((ch = *s1) != '\0') {
		if ((ch == '+') || (ch == '-')) {
			sum += (long) strtoul(s1,&s1,0);
		} else if (ch == '&') {
			sum &= (long) strtoul(s1+1,&s1,0);
		} else if (ch == '<'  && s1[1] == '<') {
			sum <<= (long) strtoul(s1+2,&s1,0);
		} else if (ch == '>'  && s1[1] == '>') {
			sum >>= (long) strtoul(s1+2,&s1,0);
		}
	}
    s1 = getspace(20);
    sprintf(s1,"$%d",sum);
	return(s1);      
}

    static void
fixed_frame_data(com,data)
char *com;
FUNCTION_DATA *data;
{
    NODE *p;
    char *s;
    int curr=0,max=0;

    while (!isdigit(*com)) /* search for function number */
        com++;
    data->number=(int)strtoul(com,(char **)NULL,0);
    while (isdigit(*com))
        com++;
    com++; /* comma */
    data->frame=(int)strtoul(com,(char **)NULL,0); /* initial value of frame_size */
    for (ALLN(p)) {
        if ((scanreg(p->op2,true) == ESP) && strstr(p->op2,frame_string)) { 
            s=p->op2;
            if (*s=='-')
                s++;
			else
				continue;
            if ((curr=(int)strtoul(s,(char **)NULL,0)) > max)
                max=curr;
        }
		else if (/* isfp(p) && */ p->op1 && strstr(p->op1,frame_string)) {
			s=p->op1;
			if (*s=='-')
				s++;
			else
				continue;
			if ((curr=(int)strtoul(s,(char **)NULL,0)) > max)
				max=curr;
		}
    }
    /* when "pushing", the address will be : -int_off+fr_sz(%esp). Before
    "pushing", increment int_off and fr_sz by 4 */
    data->int_off=max;
    data->pars=0;
	data->regs_spc = 0; /* only until we can determine how many are already saved. 
						 * This will be done in raparam */
}

static asz_element *asz_walk = NULL;
static void
add_asz(char *line)
{
	if (asz_walk == NULL) {
		asz_list.next = (asz_element *) getspace(sizeof(asz_element));
		asz_walk = asz_list.next;
	} else {
		asz_walk->next = (asz_element *) getspace(sizeof(asz_element));
		asz_walk = asz_walk->next;
	}
	line = strstr(line,".ASZ");
	line += 4; /*skip .,A,S,Z ,get to the number*/
	asz_walk->name = (int)strtoul(line,(char **)NULL,0);
	line = strchr(line,',');
	line++;
	asz_walk->size = (int)strtoul(line,(char **)NULL,0);
	asz_walk->next = NULL;
}

	static void
parse_op(s)
	register char *s;
{
	register char *t;
	register more, prflg;

	more=true;
	prflg=false;

	while (more) { /* scan to end of operand and save */	
	if (! *s )
		return;
	SkipWhite(s);
	t = s;		/* remember effective operand start */

	while(*s && (*s != ',') ) { /* read to end of this operand */
		switch ( *s++ ) {
		case CC: /* process comment */
			if( isret( lastnode ) ) rvregs = *s - '0';
			*s = *(s-1) = '\0'; /* that's what the old code did ??? */
			if (s == t+1) return; /* a comment is not an operand, dont saveop */
		break;
		case '(':
			prflg = true;
		break;
		case ')':
			prflg = false;
		break;
		}
		if(prflg && (*s==',')) s++;
	}

	if(*s ) *s = '\0';	/* change ',' to '\0' */
	else more = false;	/* found the end of instruction */
	/* now s points to null at end of operand */
    if ( isimmed(t) ) {
		t = calc_v(t);
	}
	saveop(opn++, t, (++s)-t, 0);
	if (is_label_text(t) && inswitch)
		addref(t,(unsigned int)(s-t),NULL);
	} /* while(more) */
	lastnode->uniqid = lineno;
	lineno = IDVAL;
}



	static void
parse_pseudo (s)
	register char *s;		/* points at pseudo-op */
{
#define NEWSECTION(x) prev_section=section; section=(x);
	register int pop;		/* pseudo-op code */
	char savechar;			/* saved char that follows pseudo */
	char * word = s;		/* pointer to beginning of pseudo */
	int m;
	enum Section save;		/* for swaps */

	init_all();
	FindWhite(s);			/* scan to white space char */
	savechar = *s;			/* mark end of pseudo-op */
	*s = '\0';
	pop = plookup(word);		/* identify pseudo-op */
	*s = savechar;			/* restore saved character */

	if(section == CSline) {
		if ( new_csline && pop == (int) FOURBYTE) {
			new_csline = false;
			if (get_first_src_line)	{
				get_first_src_line = false;
				first_src_line = (int)strtoul(s,(char **)NULL,0);
			} else if (get_lineno) {	
				lineno = (int)strtoul(s,(char **)NULL,0);
				get_lineno = false; 
			} 
		}
		if (pop != (int) PREVIOUS)  {
			return;
		} else if (! init_line_flag) {
			init_line_section();
		}
		/* assume .line entries are simple mindedly bracketted
		   by .section .line ... .previous 
		   The .previous will get printed by the code that
		   comes next. */
	}
	if (section != CStext) {
		switch(pop) { /* check for section changes and .long,
				 otherwise just print it. */
		case TEXT:
			NEWSECTION(CStext);
			printf("%s\n", line);
		/* don't print: is this right??? (psp) */
			break;

		case PREVIOUS:
			save=section;
			section=prev_section;
			prev_section=save;
			printf("%s\n", linptr); /* flush to output */
			break;

		case SECTION:
			printf("%s\n", line);
			prev_section=section;
			section=parse_section(s);
			break;

		case LONG:
			break;
		case BYTE:
			{int x1,x2,x3,x4;
				if (seen_ro) {
					sscanf(s,"%x,%x,%x,%x",&x1,&x2,&x3,&x4);
					if ((x4 & 0x80) == 0) { /* positive */
						addlist(seen_ro,(unsigned long)
							(x1 + (x2<<8) + (x3 << 16) + (x4 <<24)));
					}
					seen_ro = NULL;
				}
			}
			/* FALLTHRU */
		default:
			printf("%s\n",line);
			break;
		} /* switch */
		if(pop != (int) LONG) {
			return;
		}
	} /* non text sections done ( except for .long ) */

	switch (pop) {			/* dispatch on pseudo-op in text */
					/* and on .long in any section */
		case TEXT:
			NEWSECTION(CStext);
			break;

		case BYTE:
			printf("%s\n",line);
			break;

		case LOCAL:
		case GLOBL:
			appfl(line, strlength(line)); /* filter node - why */
			break;

		case FIL:
#ifdef EMON
			if (dflag) {
				int l;
				char *s = strchr(line,'"');

				input_file_name = xalloc(l=strlen(s)-1);
				sprintf(input_file_name,"%s",s+1);
				input_file_name[l-1] = (char) 0;
			}
			/* FALLTHRU */
#endif
		case BSS: /* assume .bss has an argument,
				 this will get .bss <no arg> wrong */
			printf("%s\n", line);
			break;

		case ALIGN:
			m=strlength(line);
			if (inswitch) {
				saveop(0, line+1, m-1, OTHER);
				opn = 1;
				lastnode->uniqid = lineno;
				lineno = IDVAL;
			} else {
				appfl(line, m);
			}
			break;

		case SET:   /* changed for fixed stack. Still need to postpone
		               .set printout until ..fr_sz is steady by value */
			m=strlength(line);
			if (strstr(line,".ASZ")) {
				add_asz(line);
				/* print value only at end of program */
				appfl(line, m);
			} else if (fixed_stack && strstr(line,".FSZ")) {
				fixed_frame_data(line,&func_data);
			} else {
				/* print value only at end of program */
				 appfl(line, m);
			}
			break;

		case DATA:
			printf("%s\n", line);
			NEWSECTION(CSdata);
			break;

		case SECTION:
			printf("%s\n", line);
			prev_section=section;
			section=parse_section(s);
			break;

		case PREVIOUS:
			save=section;
			section=prev_section;
			prev_section=save;
			printf("%s\n", linptr); /* flush to output */
			break;

		case LONG:
			SkipWhite(s);

/* we have to deal with whether .long is within a switch (SWBEG/SWEND) or
** not, and whether or not it appears in a data section.
*/

			if (inswitch) {		/* always add reference */
				char *t;
				if ((t = strchr(s,'-')) != 0) {
					/* Assume we are looking at .long .Lxx-.Lyy,
				   	where .Lyy is the target of a call ( PIC
				   	code ).  So we make the first label
				   	hard.  The second label is handled
				   	when the call is parsed. */
					*t = '\0';
					addref(s,(unsigned int)(t-s+1),NULL);
					*t = '-';
				} else {
					addref(s,(unsigned int)(strlen(s)+1),switch_label);
					/* we assume only one arg per .long */
				}
			}

			if (section != CStext) {
				/* not in .text, flush to output */
				printf("%s\n",linptr); /* print line */
			} else if (inswitch) {
				/* text and switch */
				putstr(linptr);	/* flush to special file */
			} else {
				/* text, not in switch */
				saveop(0,".long",6,OTHER);
				opn = 1;	/* doing first operand */
				parse_op(s);
			}
			break;

		/*
		* ELF new pseudo_op
		*
		* do optimizations, then spit out the 
		* input .size line which is assumed to be
		* of the form .size foo,.-foo ( no intervening white space )
		*/

		case SIZE:
			/* For clarity, this code should be placed in a separate function. */
		{
			char * ptr;
			SkipWhite(s);
			ptr=strchr(s,',');
			if(ptr == NULL) { /* not the right format */
				printf("%s\n",line);
				break;
			}
			if(*(ptr+1) != '.' || *(ptr+2) != '-') {
				printf("%s\n",line);
				break;
			}
			*ptr = '\0'; /* s points to function name, now */
			if(strcmp(s,ptr+3) != 0) { 
				/* Some kind of .size in text unknown to us. */
				*ptr = ',';
				printf("%s\n",line);
				fatal(__FILE__,__LINE__,
				"parse_pseudo(): unrecognized .size directive\n");
				break;

			}
			if (first_src_line != 0) {
				print_FS_line_info(first_src_line,s);
				first_src_line = 0;
			}
			*ptr = ',';
		}

			printf("	.text\n");
#ifdef DEBUG
			{
				void ratable();
				++d;
				if (do_getenv) {
					get_env();
				}
				if (min && (d> max || d < min)) {
					ratable();
					filter();
					goto noopt;
				} else if (min) {
					fprintf(stderr,"%d %s ", d,get_label());
				}
				opt_idx = 0;
				if (min5) {
					if (d > max5 || d < min5) {
						target_cpu = P4;
						fprintf(stderr,"%d-4 %s ",d,get_label());
					} else {
						if (r5) {
							target_cpu = P5;
						} else if (rblend) target_cpu = blend; {
							fprintf(stderr,"%d-%d %s ",d,
							target_cpu & (blend|P5) ? 5 : 4 ,get_label());
						}
					}
				}
			}
#endif
#ifdef EMON
			function_index++;
#endif
			if( !asmflag || !aflag  ) {
				if (asmflag) {
					suppress_enter_leave = true;
				}
				sets_and_uses();
				/* what registers point to the stack?
				** on fixed frame model it's only esp.
				** otherwise, it's both ebp and esp, as long as we do not
				** do frame_pointer elimination. If we're not going to do,
				** ebp points to the stack all throughout the optimization
				** process. If we do, then ebp points to the stack
				** until we do, and then it's only esp.
				*/
				if (fixed_stack) {
					frame_reg = ESP;
				} else {
					frame_reg = EBP | ESP;
				}
				replace_asz();
				set_save_restore();
				if (func_data.two_entries) {
					reord_double_top();
					eliminate_duplicate_part();
				} else {
					reordtop();
				}
				match_ind_calls();
				if (!ieee_flag) {
					/*make less fp inst before estimation*/
					fpeep();
				}
				estimate_double_params();
				if (   !suppress_enter_leave
				    && !func_data.two_entries) { /*IKWID*/
					if ((double_aligned = (check_double()
					          || double_params())) != 0) {
						suppress_enter_leave = true;
					}
				} else {
					double_aligned = false; 
				}
				hide_safe_asm();
				setautoreg();	/* set numnreg and numauto */
				recover_safe_asm();
				auto_elim= !numauto;
				if(!suppress_register_allocation) {
					hide_safe_asm();
					numnreg = raoptim(numnreg, numauto, &auto_elim);
					if (fixed_stack) {
						forget_bi();
					}
					recover_safe_asm();
					unmark_intrinsics();
				}
				if (func_data.two_entries) {
					args_2_regs();
				}
				remove_overlapping_regals();
#ifdef DEBUG
				(void ) verify_safe_asm();
#endif
				if (!fixed_stack) {
					if (suppress_stack_cleanup) {
						suppress_stack_cleanup = never_stack_cleanup;
					} else {
						hide_safe_asm();
						stack_cleanup();
						recover_safe_asm();
					}
				}
				loop_regal(DO_loop_regal_only);
				rm_all_tvrs();
				rm_tmpvars();
				ebboptim(COPY_PROP);
				loop_descale();
				if (!eiger) {
					loop_detestl();
				}
				if (! ieee_flag) {
					fp_loop();
					fpeep();
					rm_all_tvrs();
					rm_tmpvars(); 
					fpeep();
				}
				filter();
				if(suppress_register_allocation) {
					void ratable();
					ratable();		
					suppress_register_allocation = never_register_allocation;
				}
				first_run = 1;
				rmrdld();	/* remove redundant loads   */
				const_index();
				optim(true);	/* do standard improvements */
				if (!eiger) {
					reverse_branches();
					optim(true);
				}
				ebboptim(ZERO_PROP); /*extended basic block zero value*/
				peep();		/* do peephole improvements */
				bbopt();
				if (eiger) {
					loop_regal(DO_loop_regal_only);
				} else {
					loop_regal(DO_deindexing);
				}
				peep();		/* do peephole improvements */
				if (!ieee_flag) {
					fp_loop();
					fpeep();
				}
				if (target_cpu != P6) {
					imull();
				}
				hide_safe_asm();
				rm_all_tvrs();
				rm_tmpvars(); 
				recover_safe_asm();
				ebboptim(ZERO_PROP);/* extended basic block zero tracing */
				(void) replace_consts(true);
				rmrdld();/* remove redundant loads   */

			/* make one more pass	     */

				const_index();
				setcond();
				if (!eiger) {
					trace_optim();
				}
				optim(true);	/* do standard improvements */
				ebboptim(COPY_PROP); /*extended basic block copy elimination*/
				clean_zero();
				peep();		/* do peephole improvements */
				if (replace_consts(false)) {
					ldanal();
				}
				ebboptim(COPY_PROP);
				rmrdpp();	/*remove redundant push /pop*/
				hide_safe_asm();
#ifdef DEBUG
				if (last_func() && skip_fp_elim && first_opt) {
					if (++opt_idx > last_opt) {
						goto noopt;
					} else  {
						fprintf(stderr,"%d remove_enter_leave",opt_idx);
					}
				}
#endif
				if (!fixed_stack) {
					if (suppress_enter_leave) {
						if (double_aligned) {
							fp_removed = remove_enter_leave(numauto);
							if (!fp_removed) {
								optim(false);
								fp_removed = remove_enter_leave(numauto);
								if (!fp_removed) {
									fatal(__FILE__,__LINE__,
									"cant eliminate frame pointer.\n");
								}
							}
						}
						auto_elim = 0;
					} else {
						fp_removed = remove_enter_leave(numauto);
						if (!fp_removed) {
							optim(false);
							fp_removed = remove_enter_leave(numauto);
							if (!fp_removed) {
								fatal(__FILE__,__LINE__,
								"cant eliminate frame pointer.\n");
							}
						}
					}
					if (fp_removed) {
						if (double_aligned) {
							frame_reg = EBP | ESP;
						} else {
							frame_reg = ESP;
						}
					} else {
						frame_reg = EBP | ESP;
					}
				} /* if ! fixed_stack */
				recover_safe_asm();
				forget_bi();
				restore_ind_calls();
				
				loop_decmpl();
				if (fixed_stack) {
					reordbot_fs();
				} else {
					reordbot(fp_removed);
				}
#if 0
				/*this seems to be a loser*/
				if (!eiger) xchg_save_locals();
#endif
				ldanal();
				rm_dead_insts();
				optim_rmbrs();
				if (   fixed_stack
				    && (target_cpu & (blend|P5))) {
					convert_mov_to_push();
				}
				/*implemented only for fixed_stack style!! */
				if (   !eiger
				    && fixed_stack
				    && !func_data.two_entries) {
					lazy_register_save();
				}
				if (   target_cpu == P6
				    || target_cpu == blend) {
					spy_body();
				}
#if EH_SUP
				/* check for NO_EXACT_FRAME and call force_frame()
				   before scheduling is done.  scheduling may
				   move another instruction between the "pushl %ebp"
				   and the "movl %esp,%ebp". */
				if (must_use_frame) {
					force_frame();
				}
#endif
				if (target_cpu != P3) {
					schedule();  /*instruction scheduler*/
					if (target_cpu != P6) {
						postpeep();  /*post schedule peepholes*/
						sw_pipeline();  /*post schedule sw pileline for FSTP */
						if (target_cpu == P4) {
							/* .backalign support */
							backalign();
						}
					}
				}
				/* This is a function for fixed stack mode, that will 
				 * minimize the size of the FSZ parameter. 
				 * After this optimization, no optimizations that deal
				 * with regals should be run.
				 */
				if (fixed_stack) {
					minimize_fsz();
					rmrdmac();
				}
			} /* if( !asmflag || !aflag ) ) */

#ifdef DEBUG
			noopt: 
#endif
		/* enable inserting "nops" that change condition codes for .align
		** disable if ASMS exist in the function.
		*/
			if (! set_nopsets) {
				if (!( asmflag || aflag) ) {
					printf("\n	.nopsets	\"cc\"");
					set_nopsets = true;
				}
			} else if (asmflag || aflag ) {
				printf("\n	.nopsets");
				set_nopsets = false;
			}
			if (!Aflag) {
				printf("\n	.align	16\n");
			} else {
				printf("\n");
			}
			reset_crs();
			if (fixed_stack && set_FSZ) {
				if (func_data.frame % 8) {
					func_data.frame += 4; 
				}
				printf("    .set    %s%d,%d\n",
				        frame_string,func_data.number,func_data.frame);
#ifndef DEBUG /*when debugging, print it anyway*/
				if (func_data.regs_spc != 0) {
#endif
					printf("    .set    %s%d,%d\n",
					       regs_string,func_data.number,func_data.regs_spc);
#ifndef DEBUG
				}
#endif
			}
#ifdef DEBUG
			if (sflag) {
				calc_frame_offsets();
			}
#endif
#ifdef EMON
			if (dflag) {
				add_emons();
			}
#endif
			prtext();
#ifdef EMON
			if (dflag) {
				pcg_routine_end();
			}
#endif
			prstr();
			printf("%s\n", line);
			if( aflag ) {
				asmchk();
			}
			init_globals();
			init_regals();
			init_reg_bits();
			init_assigned_params();
			init();
			(void) get_free_reg(0,0,NULL); /* Init get_free_reg() */
			break;

		default:		/* all unrecognized text
		        		** pseudo-ops
		        		*/
			if (! inswitch) {
				printf("%s\n", linptr); /* flush to output */
			} else {
				/* in switch:  to special file */
				putstr(linptr);
			}
			break;
	}
}

	static void
filter() /* print FILTER nodes and remove from list, also
		clean up loop comment nodes, normally done in
		raoptim. */
{
	register NODE *p;
	if (fixed_stack) {
	NODE *q,*r;
	int size;
	int smov_size;
		for(ALLN(p)) {
			if (p->op == TSRET) {
				size = -2;
				r = NULL;
				smov_size = 0;
				for (q = p->forw; q ; q = q->forw) {
					if (q->op == TSRET) {
						DELNODE(p);
						p = q;
						DELNODE(q);
						if (smov_size && r) {
							r->save_restore = smov_size * size;
						}
						break;
					}
					if (q->op == SMOVB) smov_size = 1;
					else if (q->op == SMOVW) smov_size = 2;
					else if (q->op == SMOVL) smov_size = 4;
					if (size == -2 && sets(q) & ECX) {
						if (isimmed(q->op1))
							size = (int)strtoul(q->op1+1,(char **)NULL,0);
						else size = -1;
					}
					if (q->op == LEAL && setreg(q->op2) == EDI
						&& scanreg(q->op1,0) & ESP) {
						r = q;
					}
				}
			}
		}
	} else {
		for(ALLN(p)) {
			if (p->op == TSRET) DELNODE(p);
		}
	}
	for(ALLN(p)) {
	if (p->op == FILTER) {
		(void) puts(p->ops[0]);
		DELNODE(p);
	}
	else if(suppress_register_allocation && p->op == LCMT)
		DELNODE(p);
	}
}
	static enum Section
parse_section(s)
register char * s; /* string argument of .section */
{
	char savechar, *sname;

	SkipWhite(s);
	if(*s == NULL)
		fatal(__FILE__,__LINE__,"parse_section(): no section name\n");
	for(sname = s; *s != '\0'; ++s)
			if(*s == ',' || isspace(*s)) break;
	savechar = *s;
	*s = '\0';
	/* If this is too slow, look at first and last char
	   to identify string. */
	if(strcmp(sname, ".debug") == 0)
		section=CSdebug;
	else if(strcmp(sname, ".line") == 0) {
		section=CSline;
		new_csline = true;
	}
	else if(strcmp(sname, ".text") == 0)
			section = CStext;
	else if(strcmp(sname, ".data") == 0)
			section = CSdata;
	else if(strcmp(sname, ".rodata") == 0)
			section = CSrodata;
	else if(strcmp(sname, ".data1") == 0)
			section = CSdata1;
	else if(strcmp(sname, ".bss") == 0)
		section=CSbss;
	else	/* unknown section */
			section = CSother;
	*s = savechar;
	return section;
}
	int
plookup(s)	/* look up pseudo op code */
	char *s;

{
	static char *pops[POTHER] = {
			"2byte",
			"4byte",
			"8byte",
			"align",
			"ascii",
			"bcd",
			"bss",
			"byte",
			"comm",
			"data",
			"double",
			"even",
			"ext",
			"file",
			"float",
			"globl",
			"ident",
			"lcomm",
			"local",
			"long",
			"previous",
			"section",
			"set",
			"size",
			"string",
			"text",
			"type",
			"value",
			"version",
			"weak",
			"word", 
			"zero",
	};

	register int l,r,m,x; /* temps for binary search */

	l = 0; r = (int) POTHER-1;
	while (l <= r) {
		m = (l+r)/2;
		x = strcmp(s+1, pops[m]); /* s points at . */
		if (x<0) r = m-1;
		else if (x>0) l = m+1;
		else return(m);
	}
	fatal(__FILE__,__LINE__,"plookup(): illegal pseudo_op: %s\n",s);	
/* NOTREACHED */
}

	void
yyinit(flags) char * flags; {

	section = CStext;	/* Assembler assumes the current section 
				   is .text at the top of the file. */

	for (; *flags != '\0'; flags++) {
		switch( *flags ) {
		case 'V':			/* Want version ID.	*/
			fprintf(stderr,"UX: optim: %s%s\n",SGU_PKG,SGU_REL);
			break;
		case 'a':
			aflag = true;
			break;
		case 'A':
			Aflag = true;
			break;
		case 't':	/* suppress rmrdld() */
			tflag = true;
			break;
		case 'T':	/* trace rmrdld */
			Tflag = true;
			break;
		default:
			fprintf(stderr,"Optimizer: invalid flag '%c'\n",*flags);
		}
	}
}

int pic_flag;	/* set by this function only */
int ieee_flag = true;

int target_cpu = blend; /* set by this function only */
int fixed_stack = 0;    /* the default is non-fixed frame mode */
int eiger = 0; /*eiger == 1 iff target_cpu == P4, to make it safe*/

	char **
yysflgs( p ) char **p; /* parse flags with sub fields */
{
	register char *s;	/* points to sub field */

	s = *p + 1;
	if (*s == NULL) {
		switch(**p) {
		case 'K': case 'Q': case 'X': case 'S': case 'y':
			fatal(__FILE__,__LINE__,"-%c suboption missing\n",**p);
			break;
		}
	}
	switch( **p ) {
	case 'K':
		if( *s == 's' ) {
			fprintf(stderr,"Optimizer: Warning: -Ksz abd -Ksd are obsolete.\n");
			return p;
		}
		else if ((*s == 'P' && strcmp(s,"PIC") == 0) ||
			 (*s == 'p' && strcmp(s,"pic") == 0)) {
			pic_flag=true;
			return p;
		}
		else if (*s == 'i' && strcmp(s,"ieee") == 0) {
			ieee_flag = true;
			return p;
		}
		else if (*s == 'n' && strcmp(s,"noieee") == 0) {
			ieee_flag = false;
			return p;
		}
#ifdef DEBUG
		else if (*s == 'e' && !strcmp(s,"env")) {
			do_getenv = true;
			return p;
		}
#endif
		fatal(__FILE__,__LINE__,"-K suboption invalid\n");
		break;
	case 'Q':
		switch( *s ) {
		case 'y': identflag = true; break;
		case 'n': identflag = false; break;
			default: 
			fatal(__FILE__,__LINE__,"-Q suboption invalid\n");
			break;
		}
		return( p );
	case 'X':	/* set ansi flags */
		switch( *s ){
		case 't': ccmode = Transition; break;
		case 'a': ccmode = Ansi; break;
		case 'c': ccmode = Conform; break;
		default: 
			fatal(__FILE__,__LINE__,"-X requires 't', 'a', or 'c'\n");
			break;
		}
		return( p );
	case '_':	/* suppress optimizations (for debugging) */
		SkipWhite(s);
		for(;*s;s++)
			switch(*s) {
#ifdef FLIRT
			case 'o':
				CHECK=search(s,&LOC);
				return (p);
				break;
#endif
			case 'r':
				never_register_allocation = true;
				suppress_register_allocation = true;
				break;
			case 's':
				never_stack_cleanup = true;
				suppress_stack_cleanup = true;
				break;
			case 'e':
				never_enter_leave = true;
				suppress_enter_leave = true;
				break;
			default:
				fatal(__FILE__,__LINE__,"-_ requires 'r' or 'e' or 's'\n");
				break;
			}
		return(p);
	case '3':
		if (!strcmp(s,"86")) {
			target_cpu = P3;
			return p;
		}
		fatal(__FILE__,__LINE__,"-386 suboption invalid\n");
		break;
	case '4':
		if (!strcmp(s,"86")) {
			target_cpu = P4 ;
			eiger = 1;
			return p;
		}
		fatal(__FILE__,__LINE__,"-486 suboption invalid\n");
		break;
	case 'p':
		if (!strcmp(s,"entium")) {
			target_cpu = P5;
			return p;
		} else 
		 if (!strcmp(s,"6")) {
			target_cpu = P6;
			return p;
		}
		fatal(__FILE__,__LINE__,"p suboption invalid\n");
		break;
	case 'y':
		if (!strcmp(s,"86")) {
			target_cpu = blend;
			return p;
		}
		fatal(__FILE__,__LINE__,"y suboption invalid\n");
		break;
	case 'n':
		nflag = (int)strtoul(s,(char **)NULL,0);
		return p;
	default:
		return( p );
	}
/* NOTREACHED */
}

#ifdef FLIRT

	static int
search(opt,index)
	char *opt;      /* name of optimization */
	int *index;     /* returnes pointer to location */
{
	int limit,x;
	int first,last,middle;

	static int optim_code[optim_num] = {
	O_BBOPT1, O_BBOPT2, O_CLEAN_ZERO, O_COMTAIL, O_CONST_INDEX, O_DETESTL, 
	O_FP_LOOP,
	O_FPEEP, O_FPEEP01, O_FPEEP02, O_FPEEP03, O_FPEEP04, O_FPEEP05,
	O_FPEEP06, O_FPEEP07, O_FPEEP08, O_FPEEP09, O_FPEEP10, O_FPEEP11, O_FPEEP12,
	O_IMM_2_REG, O_IMULL, O_INSERT_MOVES, O_LOOP_DECMPL, O_LOOP_DESCALE,
	O_MRGBRS, O_OPTIM, O_PEEP,
	O_POSTPEEP, O_RAOPTIM, O_REGALS_2_INDEX_OPS,
	O_REMOVE_OVERLAPPING_REGALS, O_REORD, O_REORDTOP,
	O_REPLACE_CONSTS, O_REPLACE_REGALS_W_REGS, O_RM_ALL_TVRS1,
	O_RM_ALL_TVRS2, O_RM_ALL_TVRS3, O_RM_ALL_TVRS4, O_RM_DEAD_INSTS,
	O_RM_MVMEM, O_RM_MVXY, O_RM_RD_CONST_LOAD,
	O_RM_TMPVARS1, O_RM_TMPVARS2, O_RMBRS, O_RMLBLS, O_RMRDLD,
	O_RMRDMV, O_RMRDPP, O_RMUNRCH, O_SCHEDULE,
	O_SETAUTO_REG, O_SETCOND, O_STACK_CLEANUP,
	O_SW_PIPELINE, O_TRY_AGAIN_W_RT_DISAMB, O_TRY_BACKWARD,
	O_TRY_FORWARD, O_XCHG_SAVE_LOCALS, O_ZVTR
	};

	first=0;
	last=optim_num;
	middle=(first+last)/2;
	limit=0;
	while (middle != limit) {
		x=strcmp(opt,optim_tbl[middle]);
			if (x==0) {                 /*found the optimization */
					*index=middle;
							return(optim_code[middle]);
			}
			else if (x<0)
				last=middle-1;
				 else
					 first=middle+1;
			limit=middle;
			middle=(first+last)/2;
	}
	*index=middle;
	return (OTHER_OPTIM);
}

#endif

	static int
lookup(op,indx)		/* look up op code and return opcode ordinal */
	 char *op;		/* mnemonic name */
	 int *indx;		/* returned index into optab[] */
{
#define ALIAS 0xF0000
	register int f,l,om,m,x;

	static int ocode[numops] = {
	AAA, AAD, AAM, AAS, ALIAS + 2,
	ADCB, ADCL, ADCW, ADDB, ADDL,
	ADDW, ANDB, ANDL, ANDW, ARPL,
	BOUND, BOUNDL, BOUNDW, ALIAS + 1, BSFL,
	BSFW, ALIAS + 1, BSRL, BSRW, BSWAP,
	ALIAS + 4, ALIAS + 1, BTCL, BTCW, BTL,
	ALIAS + 1, BTRL, BTRW, ALIAS + 1, BTSL,
	BTSW, BTW, CALL, CALL_0, CALL_1, CALL_2, CBTW, CLC,
	CLD, CLI, ALIAS + 2, CLRB, CLRL,
	CLRW, CLTD, CLTS, CMC, 
 	ALIAS + 4, ALIAS + 1, CMOVAEL,CMOVAEW, CMOVAL, CMOVAW,
	ALIAS + 4, ALIAS + 1, CMOVBEL, CMOVBEW, CMOVBL, CMOVBW,
	ALIAS + 1, CMOVCL, CMOVCW, ALIAS + 1, CMOVEL, CMOVEW,
	ALIAS + 4, ALIAS + 1, CMOVGEL, CMOVGEW, CMOVGL, CMOVGW,
	ALIAS + 4, ALIAS + 1, CMOVLEL, CMOVLEW, CMOVLL, CMOVLW,
	ALIAS + 4, ALIAS + 1, CMOVNAEL, CMOVNAEW, CMOVNAL, CMOVNAW,
	ALIAS + 4, ALIAS + 1, CMOVNBEL, CMOVNBEW, CMOVNBL, CMOVNBW,
	ALIAS + 1, CMOVNCL, CMOVNCW, ALIAS + 1, CMOVNEL, CMOVNEW,
	ALIAS + 4, ALIAS + 1, CMOVNGEL, CMOVNGEW, CMOVNGL, CMOVNGW,
	ALIAS + 4, ALIAS + 1, CMOVNLEL, CMOVNLEW, CMOVNLL, CMOVNLW,
	ALIAS + 1, CMOVNOL, CMOVNOW, ALIAS + 1, CMOVNPL, CMOVNPW,
	ALIAS + 1, CMOVNSL, CMOVNSW, ALIAS + 1, CMOVNZL, CMOVNZW,
	ALIAS + 1, CMOVOL, CMOVOW, ALIAS + 4, ALIAS + 1, CMOVPEL,
	CMOVPEW, CMOVPL, ALIAS + 1, CMOVPOL, CMOVPOW, CMOVPW,
	ALIAS + 1, CMOVSL, CMOVSW, ALIAS + 1, CMOVZL, CMOVZW,
	ALIAS + 2, CMPB, CMPL, ALIAS + 2, CMPSB, CMPSL,
	CMPSW, CMPW, ALIAS + 2, CMPXCHGB, CMPXCHGL,
	CMPXCHGW, CTS, CWTD, CWTL, DAA,
	DAS, ALIAS + 2, DECB, DECL, DECW,
	ALIAS + 2, DIVB, DIVL, DIVW, ENTER,
	ESC, F2XM1, FABS, FADD, FADDL,
	FADDP, FADDS, FBLD, FBSTP, FCHS,
	FCLEX, 
	FCMOVB, FCMOVBE, FCMOVE, FCMOVNB, FCMOVNBE, FCMOVNE,
	FCMOVNU, FCMOVU, 
	FCOM, FCOML, FCOMP, FCOMPL,
	FCOMPP, FCOMPS, FCOMS, FCOS, FDECSTP,
	FDIV, FDIVL, FDIVP, FDIVR, FDIVRL,
	FDIVRP, FDIVRS, FDIVS, FFREE, FIADD,
	FIADDL, FICOM, FICOML, FICOMP, FICOMPL,
	FIDIV, FIDIVL, FIDIVR, FIDIVRL, FILD,
	FILDL, FILDLL, FIMUL, FIMULL, FINCSTP,
	FINIT, FIST, FISTL, FISTP, FISTPL,
	FISTPLL, FISUB, FISUBL, FISUBR, FISUBRL,
	FLD, FLD1, FLDCW, FLDENV, FLDL,
	FLDL2E, FLDL2T, FLDLG2, FLDLN2, FLDPI,
	FLDS, FLDT, FLDZ, FMUL, FMULL,
	FMULP, FMULS, FNCLEX, FNINIT, FNOP,
	FNSAVE, FNSTCW, FNSTENV, FNSTSW, FPATAN,
	FPREM, FPREM1, FPTAN, FRNDINT, FRSTOR,
	FSAVE, FSCALE, FSETPM, FSIN, FSINCOS,
	FSQRT, FST, FSTCW, FSTENV, FSTL,
	FSTP, FSTPL, FSTPS, FSTPT, FSTS,
	FSTSW, FSUB, FSUBL, FSUBP, FSUBR,
	FSUBRL, FSUBRP, FSUBRS, FSUBS, FTST,
	FUCOM, FUCOMP, FUCOMPP, FWAIT, FXAM,
	FXCH, FXTRACT, FYL2X, FYL2XP1, HLT,
	ALIAS + 2, IDIVB, IDIVL, IDIVW, ALIAS + 2,
	IMULB, IMULL, IMULW, ALIAS + 6, INB,
	ALIAS + 2, INCB, INCL, INCW, INL,
	INS, INSB, INSL, INSW, INT,
	INTO, INVD, INVLPG, INW, IRET,
	JA, JAE, JB, JBE, JC,
	JCXZ, JE, JG, JGE, JL,
	JLE, JMP, JNA, JNAE, JNB,
	JNBE, JNC, JNE, JNG, JNGE,
	JNL, JNLE, JNO, JNP, JNS,
	JNZ, JO, JP, JPE, JPO,
	JS, JZ, LAHF, LAR, LARL,
	LARW, LCALL, LCALL_0, LCALL_1, LCALL_2,
	LDS, LDSL, LDSW,
	ALIAS + 1, LEAL, LEAVE, LEAW, LES,
	LESL, LESW, ALIAS + 1, LFSL, LFSW,
	LGDT, ALIAS + 1, LGSL, LGSW, LIDT,
	LJMP, LLDT, LMSW, LOCK, ALIAS + 2,
	LODSB, LODSL, LODSW, LOOP, LOOPE,
	LOOPNE, LOOPNZ, LOOPZ, LRET, LSL,
	LSLL, LSLW, ALIAS + 1, LSSL, LSSW,
	LTR, ALIAS + 2, MOVB, MOVL, ALIAS + 4,
	ALIAS + 1, MOVSBL, MOVSBW, MOVSL, ALIAS + 1,
	MOVSWL, MOVW, MOVZBL, MOVZBW, MOVZWL,
	ALIAS + 2, MULB, MULL, MULW, ALIAS + 2,
	NEGB, NEGL, NEGW, NOP, ALIAS + 2,
	NOTB, NOTL, NOTW, ALIAS + 2, ORB,
	ORL, ORW, ALIAS + 2, OUTB, OUTL,
	OUTS, OUTSB, OUTSL, OUTSW, OUTW,
	ALIAS + 2, POPA, POPAL, POPAW, POPF,
	POPFL, POPFW, POPL, POPW, ALIAS + 7,
	PUSHA, PUSHAL, PUSHAW, PUSHF, PUSHFL,
	PUSHFW, PUSHL, PUSHW, ALIAS + 2, RCLB,
	RCLL, RCLW, ALIAS + 2, RCRB, RCRL,
	RCRW, REP, REPE, REPNE, REPNZ,
	REPZ, RET, ALIAS + 2, ROLB, ROLL,
	ROLW, ALIAS + 2, RORB, RORL, RORW,
	SAHF, ALIAS + 2, SALB, SALL, SALW,
	ALIAS + 2, SARB, SARL, SARW, ALIAS + 2,
	SBBB, SBBL, SBBW, SCAB, SCAL,
	ALIAS + 2, SCASB, SCASL, SCASW, SCAW,
	ALIAS + 2, SCMPB, SCMPL, SCMPW, SETA,
	SETAE, SETB, SETBE, SETC, SETE,
	SETG, SETGE, SETL, SETLE, SETNA,
	SETNAE, SETNB, SETNBE, SETNC, SETNE,
	SETNG, SETNL, SETNLE, SETNO, SETNP,
	SETNS, SETNZ, SETO, SETP, SETPE,
	SETPO, SETS, SETZ, SGDT, ALIAS + 5,
	SHLB, ALIAS + 1, SHLDL, SHLDW, SHLL,
	SHLW, ALIAS + 5, SHRB, ALIAS + 1, SHRDL,
	SHRDW, SHRL, SHRW, SIDT, SLDT,
	ALIAS + 2, SLODB, SLODL, SLODW, ALIAS + 2,
	SMOVB, SMOVL, SMOVW, SMSW, ALIAS + 2,
	SSCAB, SSCAL, SSCAW, ALIAS + 2, SSTOB,
	SSTOL, SSTOW, STC, STD, STI,
	ALIAS + 2, STOSB, STOSL, STOSW, STR,
	ALIAS + 2, SUBB, SUBL, SUBW, ALIAS + 2,
	TESTB, TESTL, TESTW, VERR, VERW,
	WAIT, WBINVD, ALIAS + 2, XADDB, XADDL,
	XADDW, ALIAS + 2, XCHGB, XCHGL, XCHGW,
	XLAT, ALIAS + 2, XORB, XORL, XORW
};

	f = 0;
	l = numops;
	om = 0;
	m = (f+l)/2;
	while (m != om) {
		x = strcmp(op,optbl[m]);
		if (x == 0) {
			if (ocode[m] & ALIAS) /* aliased to other opcode. */
				m = (ocode[m] & ~ALIAS) + m;
			*indx = m;
			return(ocode[m]);
			  }
		else if (x < 0)
			l = m-1;
			else
			f = m+1;
		om = m;
		m = (f+l)/2;
		}
	*indx = m;
	return(OTHER);
}

	static FILE *
tmpopen() {
	strcpy( tmpname, tempnam( TMPDIR, "25cc" ) );
	return( fopen( tmpname, "w" ) );
}

	static void
putstr(string)   char *string; {
	/* Place string from the text section into a temporary file
	 * to be output at the end of the function */

	if( stmpfile == NULL )
		stmpfile = tmpopen();
	fprintf(stmpfile,"%s",string);
}

	static void
prstr() {
/* print the strings stored in stmpfile at the end of the function */

	if( stmpfile != NULL ) {
		register int c;

		stmpfile = freopen( tmpname, "r", stmpfile );
		if( stmpfile != NULL )
			while( (c=getc(stmpfile)) != EOF )
				putchar( c );
		else
			{
			fprintf( stderr, "optimizer error: ");
			fprintf( stderr, "lost temp file\n");
			}
		(void) fclose( stmpfile );	/* close and delete file */
		unlink( tmpname );
		stmpfile = NULL;
		}
}

/* opens the temp file for storing input while looking for 'asm' */
	static void
asmopen() {
	strcpy( atmpname, tempnam( TMPDIR, "asm" ) );
	atmpfile = fopen( atmpname, "w" );
	asmotell = ftell( stdout );
}

/* writes to temp file for 'asm' processing */
	int
putasm( lptr )
char *lptr;
{
	if(section == CSdebug) return true;
	if (*lptr == ' ') *lptr = '\t';
	if(fputs( lptr, atmpfile ) == EOF) return 0;
	else return (fputc('\n',atmpfile) != EOF);
}

/* checks for 'asm' in files and undoes code movement */
	static void
asmchk() 
{
	register c;
	long endotell;
	extern int vars;

	if( asmflag ) {
		if( freopen( atmpname, "r", atmpfile ) != NULL ) {
			endotell = ftell( stdout );
			fseek( stdout, asmotell, 0 ); /* This is okay as long 
				as IMPIL is defined because it 
				is not really stdout, it is the file used by
				in-line expansion.  That file is still used, 
				even when in-line expansion is suppressed. 
				If IMPIL is not defined, optim will not work
				correctly to a terminal, but it will work
				correctly to a file.  
				This should be fixed.  */
			while( ( c = getc( atmpfile ) ) != EOF ) putchar( c );
			while( ftell( stdout ) < endotell ) printf( "!\n" );/* ? */
		}
		else fprintf( stderr, "optimizer error: asm temp file lost\n" );
	}
	freopen( atmpname, "w", atmpfile );
	asmotell = ftell( stdout );
	vars=0; 	/* reinitialize for global reg allocation */
}

	void
dstats() { /* print stats on machine dependent optimizations */

#if STATS
	fprintf(stderr,"%d semantically useless instructions(s)\n", nusel);
	fprintf(stderr,"%d useless move(s) before compare(s)\n", nmc);
	fprintf(stderr,"%d merged move-arithmetic/logical(s)\n", nmal);
	fprintf(stderr,"%d useless sp increment(s)\n", nspinc);
	fprintf(stderr,"%d redundant compare(s)\n", nredcmp);
#endif /* STATS */
}

/* print unprocessed text and update statistics file */
	void
wrapup()
{
	if (n0.forw != NULL) {
		printf("	.text\n");
		filter();
		prtext();
		prstr();
		if(identflag)			/* Output ident info.	*/
			printf("\t.ident\t\"optim: %s\"\n",SGU_REL);
		}

	if( aflag ) {
		(void) fclose( atmpfile );	/* close and delete file */
		unlink( atmpname );
	}

	exit_line_section(); /* print the last .line entry */
	print_debug(); /* Dump out the debugging info for the whole file. */

#ifdef EMON
	if (dflag) pcg_end();
#endif

}

/* change the instruction subl $numauto,%esp to subl $(numauto+4),%esp,
** update the value of the variable numauto and return the old value of
** numauto +4.
** There is a thing with double aligned. We emit the instruction andl $-8,ebp.
** This may sub $4 from ebp, and may not. In that case we also add 4 to
** numauto. Therefore, we have to return the previous numauto, since it was
** not used, and not the previous numauto +4, which is illegal.
*/
int
inc_numauto(x) int x;
{
NODE *p;
int oldnumauto;
int retval;
static NODE *get_func_real_start();

	if (fixed_stack) {
		retval = func_data.int_off + 4 - func_data.regs_spc;
		func_data.int_off +=x;
		func_data.frame += x;
		func_data.pars += (x/4);
		/*return (func_data.int_off - func_data.regs_spc);*/
		return retval;
	} else {
	auto_elim = false;
	/* get to beginning of prolog */
	if (func_data.two_entries) {
		p = get_func_real_start();
	} else {
		for (p = n0.forw; p != 0; p = p->forw)
			if (p->op == MOVL || p->op == PUSHL || p->op == POPL) break;
	}

	if (!p) fatal(__FILE__,__LINE__,"inc numauto internal error\n");

	/*skip over code for getting the address of returned struct, if any*/
	if (isgetaddr(p)) p = p->forw->forw;

	/* skip over profiling code */
	if (isprof( p->forw )) p = p->forw->forw;

	/* expect saving of frame pointer */
    if (p->op != PUSHL || !usesvar("%ebp", p->op1))
        fatal(__FILE__,__LINE__,"inc numauto second internal error\n");
	
	p = p->forw;

	/* expect adjust the frame pointer */
    if ((p->nlive &EBP)
        && (p->op != MOVL || !usesvar("%esp",p->op1) || !usesvar("%ebp",p->op2)))
            fatal(__FILE__,__LINE__,"inc numauto third internal error\n");

    if (p->forw->op == ANDL && p->forw->op1[0] == '$'
        && (int)strtoul(&p->forw->op1[1],(char **)NULL,0) == -8
	&& samereg(p->forw->op2,"%ebp"))
            p = p->forw;

	p = p->forw;

	if (p->op == SUBL && p->op1[0] == '$' && samereg(p->op2,"%esp")) {
		oldnumauto = (int)strtoul(&p->op1[1],(char **)NULL,0);	/* remember # of bytes */
		p->op1 = getspace(ADDLSIZE);
		sprintf(p->op1,"$%d",numauto = oldnumauto+x);
	} else if (p->op == PUSHL && strcmp(p->op1,"%eax") == 0) {
		oldnumauto = INTSIZE;
		chgop(p,SUBL,"subl");
		p->op2 = "%esp";
		p->op1 = getspace(ADDLSIZE);
		sprintf(p->op1,"$%d",numauto = oldnumauto+x);
		p->sets = p->uses = ESP | CONCODES_AND_CARRY;
	} else {
		oldnumauto = 0;
		p = prepend(p,"%esp");
		chgop(p,SUBL,"subl");
		p->op1 = getspace(ADDLSIZE);
		sprintf(p->op1,"$%d",numauto = x);
		p->sets = p->uses = ESP | CONCODES_AND_CARRY;
	}
	return double_aligned ? (oldnumauto) : (oldnumauto+4);
	} /* not fixed stack case */
}/*end inc_numauto*/

	static void
setautoreg() { /* set indicator for number of autos/regs */

	register NODE *p;
	NODE *new;
	int d_param_offset;
	int org_numauto;
	int num_double_params = double_params();
	static NODE *get_func_real_start();
#ifdef FLIRT
	static int numexec=0;
#endif

	COND_RETURN("setauto_reg");

#ifdef FLIRT
	++numexec;
#endif

	if (!double_aligned) num_double_params = 0;
	numauto = 0;
	for(p = n0.forw; p != &ntail && (is_debug_label(p) || !islabel(p)) ; p = p->forw)
		;	/* Skip over initial .text and stuff */
	if (func_data.two_entries) {
		p = get_func_real_start();
		p = p->back;
	}
	else if ( 		/* this line assumes left->right execution */
		 p == NULL
		 ||  p == &ntail
		 ||  !islabel(p)
		 ||  !ishl(p)
	   ) {					/* Can't determine sizes */
		numauto = BIGAUTO;
		numnreg = BIGREGS;
		fprinst(p); 
		fatal(__FILE__,__LINE__,"cant find numauto\n");
		return;
	}
	if (p->forw->op == POPL && usesvar("%eax",p->forw->op1) &&
		p->forw->forw->op == XCHGL && usesvar("%eax",p->forw->forw->op1) &&
		strcmp("0(%esp)",p->forw->forw->op2) == 0)
		p = p->forw->forw;
      /* at the beggining of the program expect:
         *
         *  push %ebp
         *  movl %esp,%ebp
         *
         *  or:
         *
         *  subl $const,%esp    (in case of fixed stack)
         */
    if (!fixed_stack) {
    if (        /* this line assumes left->right execution */
             (p = p->forw)->op != PUSHL
         ||  strcmp(p->op1,"%ebp") != 0
         ||  (p = p->forw)->op != MOVL
         ||  strcmp(p->op1,"%esp") != 0
         ||  strcmp(p->op2,"%ebp") != 0
       ) {                  /* Can't determine sizes */
        numauto = BIGAUTO;
        numnreg = BIGREGS;
        fprinst(p);
        fatal(__FILE__,__LINE__,"cant find numauto\n");
        return;
    }
    }

	p = p->forw;
    if (!fixed_stack) {
	if (p->op == SUBL && p->op1[0] == '$' && 
		p->zero_op1 == 0 && samereg(p->op2,"%esp")) {
		numauto = (int)strtoul(&p->op1[1],(char **)NULL,0);	/* remember # of bytes */
		if (double_aligned) {
			org_numauto = numauto;
			numauto += 4+8*num_double_params;
			if (num_double_params && (org_numauto % 8)) {
				numauto += 4;
				org_numauto += 4;
			}
			p->op1 = getspace(ADDLSIZE);
			sprintf(p->op1,"$%d",numauto);
		}
		p = p->forw;
	} else if (p->op == PUSHL && strcmp(p->op1,"%eax") == 0) {
		numauto = INTSIZE;
		if (double_aligned) {
			org_numauto = INTSIZE;
			numauto += 4+8*num_double_params;
			if (num_double_params) {
				numauto+= 4;
				org_numauto += 4;
			}
			chgop(p,SUBL,"subl");
			p->op1 = getspace(ADDLSIZE);
			sprintf(p->op1,"$%d",numauto);
			p->op2 = "%esp";
			new_sets_uses(p);
		}
		p = p->forw;
	} else if (num_double_params) {
		org_numauto = 0;
		numauto = 8*num_double_params+4;
		new = prepend(p,NULL);
		chgop(new,SUBL,"subl");
		new->op1 = getspace(NEWSIZE);
		new->op2 = "%esp";
		sprintf(new->op1,"$%d",numauto);
		new_sets_uses(new);
	}
	}
    else    /* we are in fixed stack mode */
        numauto = func_data.int_off;

	numnreg = 0;
    if (!fixed_stack) {
	while ( p != &ntail
		&&  p->op == PUSHL
		&&  ( strcmp(p->op1,"%ebx") == 0
		   || strcmp(p->op1,"%esi") == 0
		   || strcmp(p->op1,"%ebi") == 0
		   || strcmp(p->op1,"%edi") == 0 ) ) {
		numnreg++;
		p = p->forw;
	}
	/* add instruction to mov the double params to the autos area */
	if (num_double_params) {
		char *rgl;
		new = p->back;
		FLiRT(O_SETAUTO_REG,numexec);
		while ((rgl = next_double_param()) != 0) {
			d_param_offset =  (*rgl == '*') ? (int)strtoul(&rgl[1],(char **)NULL,0)
							: (int)strtoul(rgl,(char **)NULL,0);
			org_numauto += 8;
			new = insert(new);
			chgop(new,MOVL,"movl");
			new->op1 = getspace(NEWSIZE);
			sprintf(new->op1,"%d(%%ebp)",d_param_offset);
			new->op2 = "%edx";
			new_sets_uses(new);
			new = insert(new);
			chgop(new,MOVL,"movl");
			new->op1 = getspace(NEWSIZE);
			sprintf(new->op1,"%d(%%ebp)",d_param_offset+4);
			new->op2 = "%ecx";
			new_sets_uses(new);
			new = insert(new);
			chgop(new,MOVL,"movl");
			new->op1 = "%edx";
			new->op2 = getspace(NEWSIZE);
			sprintf(new->op2,"%d(%%ebp)",-org_numauto);
			new_sets_uses(new);
			new = insert(new);
			chgop(new,MOVL,"movl");
			new->op1 = "%ecx";
			new->op2 = getspace(NEWSIZE);
			sprintf(new->op2,"%d(%%ebp)",-org_numauto+4);
			new_sets_uses(new);
			set_param2auto(rgl,-org_numauto);
			rgl = getspace(NEWSIZE);
			sprintf(rgl,"%d(%%ebp)",d_param_offset+4);
			set_param2auto(rgl,-org_numauto+4);
		}
	}
	for ( ; p != &ntail; p = p->forw) {
		int m;
		int offset;
		if (scanreg(p->op1,0) == EBP) m = 1;
		else if (scanreg(p->op2,0) == EBP) m = 2;
		else continue;
		if ((offset = is_double_param_regal(p->ops[m])) != 0) {
			p->ops[m] = getspace(NEWSIZE);
			sprintf(p->ops[m],"%d(%%ebp)",offset);
		} else {
			char rgl[NEWSIZE];
			sprintf(rgl,"%d(%%ebp)"
			     ,((int)strtoul(p->ops[m],(char **)NULL,0) - 4));
			if ((offset = is_double_param_regal(rgl)) != 0) {
                p->ops[m] = getspace(NEWSIZE);
                sprintf(p->ops[m],"%d(%%ebp)",4 + offset);
            }
		}
	}
	}
	return;
}/*end setautoreg*/

/*
 * The following code reorders the Top of a C subroutine.
 * So that the 2 extraneous jumps are removed.
 */
	static void
reordtop()
{
	register NODE *pt, *st, *end;
	char *reg_temp,*temp;
#ifdef FLIRT
	static int numexec=0;
#endif

	COND_RETURN("reordtop");

#ifdef FLIRT
	numexec++;
#endif

	for(pt = n0.forw; pt != &ntail; pt = pt->forw)
		if ( islabel(pt) && !is_debug_label(pt))
			break;
	if ( islabel(pt) && ishl(pt) ) {
		pt = pt->forw;
		if ( !isuncbr(pt) && pt->op != JMP )
			return;
		if ( pt->op1[0] != '.' )
			return;
		if ( !islabel(pt->forw) || ishl(pt->forw) )
			return;
		for (st = pt->forw; st != &ntail; st = st->forw) {
			if ( !islabel(st) )
				continue;
			if ( strcmp(pt->op1, st->opcode) != 0 )
				continue;
			/* Found the beginning of code to move */
			for( end = st->forw;
				end != &ntail && !isuncbr(end);
				end = end->forw )
				;
			if ( end == &ntail )
				return;
			if ( end->op != JMP ||
				strcmp(end->op1,pt->forw->opcode) != 0 )
				return;

			/* Relink various sections */

			/* if the subl inst is missing for fixed frame, add it */
			if (fixed_stack) {
				/* st points to the lable that begins the opening block */
				NODE *p = st;
				boolean is_subl = false;
				
				while (p->op != JMP) {
					if (p->op == SUBL && samereg(p->op2,"%esp")) {
						is_subl = true;
						break;
					}
					p = p->forw;
				}
				if (!is_subl) {
					reg_temp = getspace(LABELSIZE);
					temp = getspace(LABELSIZE);
					p = st;
					sprintf(temp,"$.FSZ%d",func_data.number);
					sprintf(reg_temp,"%%esp");
					addi(p,SUBL,"subl",temp,reg_temp);
					new_sets_uses(p);
				}
			}
			st->back->forw = end->forw;
			end->forw->back = st->back;
			FLiRT(O_REORDTOP,numexec);
			pt->back->forw = st->forw;
			st->forw->back = pt->back;

			end->forw = pt->forw;
			pt->forw->back = end;
			return;			/* Real Exit */
		}
	}
}

static boolean
func_has_pic_call(NODE *second_entry)
{
NODE *p;
	for (p = second_entry; p && p != &ntail; p = p->forw) {
		if (p->op == CALL && !strcmp(p->op1,p->forw->opcode))
			return true;
	}
	return false;
}

static NODE*
move_call_to_body(NODE *first_prolog,NODE *second_prolog,NODE *first_in_body)
{
NODE *p, *q, *jump_inst;
	for (p = first_prolog; p != second_prolog; p = p->forw) {
		if (p->op == JMP) {
			jump_inst = p;
			break;
		}
	}
	for (p = second_prolog; p && p != &ntail; p = p->forw) {
		if (islabel(p) && !strcmp(p->opcode,jump_inst->op1)) {
			for (q = p; q && q->op != JMP; q = q->forw) {
			}
			q = q->back;
			break;
		}
	}
	p = p->forw;
	DELNODE(p->back);
	p->back->forw = q->forw;
	q->forw->back = p->back;
	p->back = first_in_body;
	q->forw = first_in_body->forw;
	first_in_body->forw->back = q;
	first_in_body->forw = p;
	jump_inst->op1 = first_in_body->opcode;
	return jump_inst;
}

/*move the epilog from the bottom to the top*/
static void
reord_double_top()
{
NODE *p,*first_jump,*second_jump,*first_in_body =NULL;
NODE *first_prolog = NULL, *second_prolog = NULL,*first_end = NULL,*second_end;
boolean call_moved = false;

	COND_RETURN("reord_double_top");

	for (ALLN(p)) {
		if ( islabel(p) && !is_debug_label(p))
			break; /*point to the name of the function*/
	}
	if (p->forw->op == JMP) first_jump = p =  p->forw;
	else  return;
	if (islabel(p->forw)) p = p->forw->forw;
	else return;
	if (p->op == JMP) {
		second_jump = p;
		p = p->forw;
	}
	else return;
	if (islabel(p)) first_in_body = p;
	else return;
	for ( ; p && p != &ntail; p = p->forw) {
		if (islabel(p)) {
			if (!strcmp(p->opcode,first_jump->op1))
				first_prolog = p;
			if (!strcmp(p->opcode,second_jump->op1))
				second_prolog = p;
		}
		if (pic_flag && !call_moved && first_prolog
			&& second_prolog && first_in_body)  {
			if (func_has_pic_call(second_prolog)) first_end
				= move_call_to_body(first_prolog,second_prolog,first_in_body);
			call_moved = true;
		}
		if ((p->op == JMP) && (!strcmp(p->op1,first_in_body->opcode))) {
			if (first_end == NULL)
				first_end = p;
			else
				second_end = p;
		}
	}
	second_end->forw->back = first_prolog->back;
	first_prolog->back->forw = second_end->forw;
	/* tie first prolog in */
	first_end->forw = first_jump->forw;
	first_jump->forw->back = first_end;
	first_jump->forw = first_prolog;
	first_prolog->back = first_jump;
	/* tie second prolog in */
	second_end->forw = second_jump->forw;
	second_jump->forw->back = second_end;
	second_jump->forw = second_prolog;
	second_prolog->back = second_jump;

}/*end reord_double_top*/

static int
get_numauto(NODE *p)
{
	for ( ; p && !islabel(p); p = p->forw) {
		if (p->op == SUBL && isconst(p->op1) && !strcmp(p->op2,"%esp"))
			return (int)strtoul(p->op1+1,(char **)NULL,0);
		if (p->op == PUSHL && !strcmp(p->op1,"%eax"))
			return 4;
	}
	return 0;
}/*end get_numauto*/

/*for a function with two entry point, the two entries have a common sub sequence.
**eliminate this: move the common part to the body. The first entry had only the
**common part, so now it will have a jump to the body. The second entry had spills
**it will still have them.
**In exact frame mode we have dependencies on ebp, so the spills must be rewritten 
**to reference esp instead.
*/
static void
eliminate_duplicate_part()
{
NODE *p,*first_prolog = NULL, *second_prolog = NULL;
NODE *q, *first_end,*second_end;
NODE *first_spill;
	COND_RETURN("eliminate_duplicate_part");
	for (ALLN(p)) {
		if (p->op == JMP && !strcmp(p->op1,p->forw->opcode)) {
			if (first_prolog == NULL) {
				p = p->forw->forw;
				DELNODE(p->back->back);
				DELNODE(p->back);
				first_prolog = p;
			} else if (second_prolog == NULL) {
				first_end = p->back->back;
				p = p->forw->forw;
				DELNODE(p->back->back);
				DELNODE(p->back);
				second_prolog = p;
			} else {
				second_end = p;
				break;
			}
		}
	}/*found all boundaries, now remove*/
	for (p=first_prolog,q=second_prolog;p!=first_end->forw;p=p->forw,q=q->forw){
		if (p->op != JMP && same_inst(p,q)) {
			q->save_restore = p->save_restore;
			DELNODE(p);
		}
	}
	/*now re-arrange the 2'nd prolog to 1'st spill, then save regs*/
	for (p = second_prolog; p != second_end; p = p->forw) {
		if (setreg(p->op1) & (EAX|EDX|ECX)) {
			first_spill = p;
			break;
		}
	}
	regs_for_incoming_args = 0;
	for (p = first_spill; ; p = p->forw ) {
		if (!(setreg(p->op1) & (EAX|ECX|EDX)))
			break;	
	}
	/*point to the label after which to move the spills*/
	second_prolog = second_prolog->back;
	/*first re-write the 2'nd operands, then move the block of insts
	**in fixed frame just remove the FSZ from the operand. In exact frame,
	**rewrite from ebp based to esp based.
	*/
	if (fixed_stack) {
		for (p = first_spill; p != second_end; p = p->forw) {
			if (scanreg(p->op2,false) & ESP) {
				int x = (int)strtoul(p->op2,(char **)NULL,0);
				p->op2 = getspace(LEALSIZE);
				sprintf(p->op2,"%d(%%esp)",x);
			}
		}
	} else {
		for (p = first_spill; p != second_end; p = p->forw) {
			int numauto = get_numauto(second_prolog);
			if (scanreg(p->op2,false) & EBP) {
				int x = (int)strtoul(p->op2,(char **)NULL,0) + numauto -4;
				p->op2 = getspace(LEALSIZE);
				sprintf(p->op2,"%d(%%esp)",x);
			}
		}
	}
	second_end = second_end->back;
	DELNODE(second_end->forw);
	second_end = second_end->forw; /*point at the label*/
	second_prolog->forw->back = second_end;
	second_end->forw->back = first_spill->back;
	first_spill->back->forw = second_end->forw;
	second_end->forw = second_prolog->forw;
	second_prolog->forw = first_spill;
	first_spill->back = second_prolog;

}/*end eliminate_duplicate_part*/

/*return the instruction after the second label in the function.
**for functions with two entry points, this is the first instruction after
**the second entry point.
*/
NODE *
second_start()
{
NODE *p, *lab = NULL;
	/*if (! func_data.two_entries) return NULL; tested at call site*/
	for (ALLN(p)) {
		if (islabel(p)) {
			if (!lab) lab = p;
			else {
				while (islabel(p)) p = p->forw;
				return p;
			}
		}
	}
	/* NOTREACHED */
	fatal(__FILE__,__LINE__,"second entry lost\n");
}

/*for two entries, skip both and find the real beginning*/
static NODE *
get_func_real_start()
{
NODE *p;
	for (ALLN(p)) {
		if (   (   p->op == PUSHL
		        && setreg(p->op1) == EBP)
		    || (   fixed_stack
		        && p->op == SUBL
			&& setreg(p->op2) == ESP))
			return p;
	}
	return NULL;
}

void
add_live_to_second_entry()
{
NODE *p;
	if (! func_data.two_entries) return;
	p = second_start();
	if (!p) fatal(__FILE__,__LINE__,"no second entry\n");
	p->uses |= regs_for_incoming_args;
	p->sets |= regs_for_incoming_args;
}

void
dehack_second_entry()
{
NODE *p;
	if (! func_data.two_entries) return;
	p = second_start();
	p->uses = uses(p);
	p->sets = sets(p);
}

/* With fixed stack code we will have two different ways of ending (and beginnig)
 * a function:
 * 1.   beginning: subl $..fr_sz_x,%esp         frame-size will be larger by three regs
 *                 movl %ebx,..fr_sz_x(%esp)
 *                 movl %esi,-4+..fr_sz_x(%esp)
 *                 movl %edi,-8+..fr_sz_x(%esp)
 *      ending:
 *                 mov ..fr_sz_x(%esp),%ebx
 *                 mov -4+..fr_sz_x(%esp),%ebx
 *                 mov -8+..fr_sz_x(%esp),%ebx
 *                 addl $..fr_sz_x,%esp
 *                 ret
 *
 *      notice that in this way, if fr_sz is updated by the regular registers, and
 *      each time we wabt to add a register (like raparam) we add to the offset
 *      being subtracted from %esp, the 'mov' instruction need no change, and '
 *      neither do the rest of the instructions that use %esp through an offset
 *      of ..fr_sz_x . Problem: suppose we have a : mov %eax, 4(%esp)
 *      and later on : mov 4(%esp),%ebx . Do we need to update the offsets ?
 *      No ! %esp is lower but so is %esp when inserting data !
 *
 *      Parameters will be available starting from  : ..fr_sz_x+4(%esp)
 *      (the 4 stand for the ret address.
 *
 *
 *
 * 2.   beginnig:   push %edi
 *                  push %esi
 *                  push %ebx
 *                  subl $..fr_sz_x,%esp    frame size does not include three regs.
 *
 *      ending:     addl $..fr_sz_x,%esp
 *                  popl %ebx
 *                  popl %esi
 *                  popl %edi
 *                  ret
 *
 *      Again, no problem with additional registers to be saved. The only difference
 *      from the upper way is that parameters will be reachable starting from:
 *              ((#initial regs)*4 + 4 + ..fr_sz_x)(%esp)
 *                                   |        |
 *                                   |        -> includes registers added (pushed).
 *                              ret address
 *      WARNING: additional registers besides those pushed before subl must NOT
 *               be pushed, but moved into the stack.
 *
 *      A problem might arrise when addressing parameters: the calculation is not
 *      identical in the two ways. This can be solved by identifying what way it is
 *      (which is done anyway) and to set a variable either to zero (for the first
 *      way) or the the value reqiured, by the number of registers "pushed".
 */

    static void
reordbot_fs()  /* this is for fixed stack */
{
    NODE    *retlp,     /* ptr to first label node in return seq. */
            *firstp;    /* ptr to first non-label node in return seq. */
    register NODE *endp,            /* ptr to last node in return seq. */
                  *lp,              /* ptr to label nodes in return seq. */
                  *p, *q;     /* ptrs to nodes in function */


    endp = &ntail;
    do {            /* scan backwards looking for RET */
        endp = endp->back;
        if (endp == &n0)
            return;
    } while (endp->op != RET);      /* Now endp points to RET node */

    firstp = endp;

    /* if first inst before ret is not ADDL something is wrong */
    if ((firstp->back->op != ADDL) || (!samereg(firstp->back->op2,"%esp")))
            return;
    firstp = firstp->back;
    /* if the compiler restores registers, the only thing left to do
     * is to find jumps to this piece of code and to unify those jumps.
     * Go back until finished pop's and add to esp.
     *
     * firstp pointing at addl. Check for restores, mark them while passing through. */

    while (IS_FIX_POP(firstp->back))
        firstp = firstp->back;

    /* Now firstp points to first non-label */
    retlp = firstp->back;
    if (retlp->sets & (EAX | FP0)) {
        retlp = retlp->back;
        firstp = firstp->back;
    }
    if (!islabel(retlp) || ishl(retlp))
        return;
    do
        retlp = retlp->back;
    while (islabel(retlp) && !ishl(retlp));
                /* Now retlp points before first label */

    for (p = ntail.back; p != &n0; p = p->back) {
                /* Scan backwds for JMP's */
        if (p->op != JMP)
            continue;
        for (lp = retlp->forw; lp != firstp; lp = lp->forw) {
                /* Scan thru labels in ret seq. */
            if (strcmp(p->op1,lp->opcode) == 0) {
                /* found a JMP to the ret. seq., so copy
                it over the JMP node */
                copyafter(firstp,endp,p);
                q = p;
                p = p->back;
                DELNODE(q);
                break;
            }
        }
    }
}


/* The code generated for returns is as follows:
 * 	jmp Ln
 *	...
 *
 * Ln:
 * 	popl %ebx	/optional
 * 	popl %esi	/optional
 * 	popl %edi	/optional
 * 	leave
 * 	ret
 *
 * This optimization replaces unconditional jumps to the return
 * sequence with the actual sequence.
 */
	static void
reordbot(no_leave) int no_leave;
{
	NODE	*retlp,		/* ptr to first label node in return seq. */
		*firstp;	/* ptr to first non-label node in return seq. */
	register NODE *endp,	/* ptr to last node in return seq. */
		*lp,		/* ptr to label nodes in return seq. */
	*new, *p, *q;		/* ptrs to nodes in function */
	endp = &ntail;
	do {			/* scan backwards looking for RET */
		endp = endp->back;
		if (endp == &n0)
			return;
	} while (endp->op != RET);	
				/* Now endp points to RET node */
	firstp = endp->back;
	if (firstp->op != LEAVE)
		return;
	/*if remove enter leave was done, then two cases:
	**if there were autos, then restore esp by adding
	**the number of autos to it.
	**if there were no autos then delete the leave instruction.
	**if remove enter leave was not done, the leave instruction
	**stays.
	*/
	if (no_leave) {
		if (autos_pres_el_done) {
			chgop(firstp,ADDL,"addl");
			firstp->op1 = getspace(ADDLSIZE);
			/* UNDOCUMENTED CHANGE */
			for (p = firstp->back; ; p = p->back)
				if (isbr(p) || islabel(p) || p->op == ASMS 
				  || (p->sets | p->uses) & ESP)
				   break;
			if (   p->op == ADDL
			    && p->op1[0] == '$'
			    && p->sets == (ESP | CONCODES_AND_CARRY)) {
				sprintf(firstp->op1,"$%d",(numauto
					 + (int)strtoul(&p->op1[1],(char **)NULL,0)));
				DELNODE(p);
			} else
				sprintf(firstp->op1,"$%d",numauto);
			firstp->op2 = "%esp";
			firstp->uses = ESP;
			firstp->sets = ESP | CONCODES_AND_CARRY;
			if (double_aligned) {
				new = insert(firstp);
				chgop(new,POPL,"popl");
				new->op1 = "%ebp";
				new->uses = ESP;
				new->sets = ESP | EBP;
			}
		} else {
			DELNODE(firstp);
			firstp=endp;
		}
	} else if (target_cpu != P3) {
		/* leave -> movl %ebp,%esp ; popl %ebp */
		new = prepend(firstp, "%esp");
		chgop(new, MOVL, "movl");
		chgop(firstp, POPL, "popl");
		new->op1 = firstp->op1 = "%ebp";
		new->op2 = "%esp";
		new->uses = EBP;
		new->sets = ESP;
		firstp->uses = ESP;
		firstp->sets = EBP | ESP;
	}
	while (firstp->back->op == POPL) /* Restores of saved registers */
		firstp = firstp->back;
				/* Now firstp points to first non-label */
	retlp = firstp->back;
	if (retlp->sets & (EAX | FP0)) {
		retlp = retlp->back;
		firstp = firstp->back;
	}
	if (!islabel(retlp) || ishl(retlp))
		return;
	do
		retlp = retlp->back;
	while (islabel(retlp) && !ishl(retlp));
				/* Now retlp points before first label */

	for (p = ntail.back; p != &n0; p = p->back) {
				/* Scan backwds for JMP's */
		if (p->op != JMP)
			continue;
		for (lp = retlp->forw; lp != firstp; lp = lp->forw) {
				/* Scan thru labels in ret seq. */
			if (strcmp(p->op1,lp->opcode) == 0) {
				/* found a JMP to the ret. seq., so copy
				it over the JMP node */
				copyafter(firstp,endp,p);
				q = p;
				p = p->back;
				DELNODE(q);
				break;
			}
		}
	}
}
	static void
copyafter(first,last,current) 
NODE *first; 
register NODE *last;
NODE *current;
{
	register NODE *p, *q;
	q = current;
	for(p = first; p != last->forw; p = p->forw) {
		if (is_debug_label(p)) continue;
		q = insert(q);
		chgop(q,(int)p->op, p->opcode);
		q->op1 = p->op1;
		q->op2 = p->op2;
		q->uses = p->uses;
		q->sets = p->sets;
		q->esp_offset = p->esp_offset;
		q->ebp_offset = p->ebp_offset;
		q->save_restore = p->save_restore ;
		q->uniqid = IDVAL;
	}
}



char *get_label() {
	NODE *pn;
	for( ALLN( pn ) ) {
		/* first label has the function name */
		if( islabel( pn ) && !is_debug_label(pn) )
			return pn->opcode;
	}
	/* NOTREACHED */
}

static void do_remove_enter_leave();
static process_body();
static process_block();
static process_header();

/* driver which conditionally performs enter-leave removal */
static
remove_enter_leave(autos_pres) int autos_pres;
{
	NODE *pn;
	int depth;	/* change in stack due to register saves */
#ifdef DEBUG
	static int dcount;
	static int lcount= -2;
	static int hcount=999999;


	if (lcount == -2) {
		char *h=getenv("hcount");
		char *l=getenv("lcount");
		if (h) hcount=(int)strtoul(h,(char **)NULL,0);
		if (l) {
			lcount=(int)strtoul(l,(char **)NULL,0);
			fprintf(stderr,"count=[%d,%d]\n",lcount,hcount);
		}
		else   lcount = -1;
	}
	++dcount;
	if ( dcount < lcount || dcount > hcount)
		return 0;
	if (lcount != -1)
		fprintf(stderr,"%d(%s) ", dcount, get_label());
#endif


	/* check if function prolog is as expected */
	switch (process_header(&pn,&depth,autos_pres,2)) {
		case 0:
			fprintf(stderr,"wrong header\n");
			return 0;
		case 2:
			return 1;
	}

	/* check if rest of function as as expected, mark statements that contain
	   frame pointer references; these will be rewritten as stack pointer
	   references
	*/
	if (process_body(pn)) {
	/* rewrite the prolog and body of a function, epilog is rewritten
	** in reordbot
	*/
		do_remove_enter_leave(depth);
		if (!auto_elim || double_aligned)
			autos_pres_el_done = true;
		return 1;
	}
	fprintf(stderr,"wrong body\n");
	return 0;
}

static int
const_at(p) NODE *p;
{
unsigned int reg;
	reg = scanreg(p->op1,false);
	for(; p!= &n0; p = p->back) {
		if (p->sets & reg) {
			if  ( *p->op1 == '$')
				return (int)strtoul(p->op1+1,(char **)NULL,0);
			else
				reg = p->uses;
		}
	}
	return 0;
}/*end const_at*/

/* given a node that effects the stack pointer, returns the change caused by this
   function
*/

static
process_push_pop(node,relax)
NODE *node; boolean relax;
{
	switch(node->op) {
	case PUSHL: case PUSHFL: 
		return 4;
	case PUSHW: case PUSHFW:
		return 2;
	case POPL:  case POPFL:
		return -4;
	case POPW:  case POPFW:
		return -2;
	case LEAL:
		return -(int)strtoul(node->op1,(char **)NULL,0);
	case SUBL:
		if (isreg(node->op1) && samereg(node->op2,"%esp"))
			return (const_at(node));
		if (*node->op1 == '$' && node->op1[1])
			return ((int)strtoul(node->op1+1,(char **)NULL,0));
	/* FALLTHRU */
	case ADDL:
		if (isreg(node->op1) && samereg(node->op2,"%esp"))
			return (-const_at(node));
		if (*node->op1 == '$' && node->op1[1])
			return (-(int)strtoul(node->op1+1,(char **)NULL,0));
	/* FALLTHRU */
	case LEAVE:
		return 0;
	/* FALLTHRU */
#if EH_SUP
	case MOVL:
		if (relax && node->sasm == SAFE_ASM) return 0;
	/* FALLTHRU */
#endif
	default:
	fprinst(node);
	fatal(__FILE__,__LINE__,"process_push_pop(): improper push pop\n");
	}/*end switch*/
	/* NOTREACHED */
}
/* rewrite operands of the format:
	*n(%ebp)
	n(%ebp)
	n(%ebp,r)
to use the stack pointer
*/
static char *
rewrite(pn,rand,depth)
NODE *pn;
char *rand;	/* operand to rewrite */
int depth;	/* stack growth since entry into the called address, this
		   includes saving registers, and any growth causes by pushing
		   arguments for function calls
		*/
{
	char *star="";
	char *lparen;
	int offset;
	int is_auto;
	if (*rand == '*') {
		++rand;
		star = "*";
	}
	if (*rand == '-' || (pn->ebp_offset != 0))
		is_auto = true;
	else
		is_auto = false;
	if (double_aligned && is_auto) {
		if (*star == '*')
			rand--;
		return rand;
	}
	offset=(int)strtoul(rand,&lparen,10);

	/* check if operand is of the desired format */
	if (!lparen || lparen[0] != '(' || lparen[1]!='%' || lparen[2]!='e' 
		|| lparen[3]!='b' || lparen[4] != 'p') {
		if (*star == '*')
			rand--;
		return rand;
	}

	pn->esp_offset = is_auto;
	pn->ebp_offset = offset;
	pn->uses = pn->uses &~EBP | ESP;
	rand = getspace(NEWSIZE);
	lparen[3] = 's'; /*make it ebp for the moment*/

	/* the new offset. The original offset assumes the function return address
	   and old frame pointer are on stack. With this optimization only the
	   function return address are on stack. This is the reason for the "-4".
	   depth
	*/
	if (double_aligned) depth += 4;
	offset += depth;
	if (!is_auto) offset -= 4;
	if (!auto_elim) offset += numauto;

	sprintf(rand,"%s%d%s",star,offset,lparen);
	lparen[3] = 'b'; /*retrieve */
	return rand;
}
static void
do_remove_enter_leave(depth) int depth; /* depth of stack after prolog */
{
	NODE *pn, *pf = NULL;
	for (pn=n0.forw; pn != 0; pn = pf) {
		pf = pn->forw;
		if (pn->extra == REMOVE) {
			DELNODE(pn);
		}
		else if (pn->extra > REMOVE) {
			if (pn->op1)
				pn->op1 = rewrite(pn,pn->op1,pn->extra+depth);
			if (pn->op2)
				pn->op2 = rewrite(pn,pn->op2,pn->extra+depth);
			if (pn->op3)
				pn->op3 = rewrite(pn,pn->op3,pn->extra+depth);
		
		}
	}
}
/* For each instruction after the prolog, this routine calculates ther
   runtime stack depth before this instruction. We can make this
   calculation, because we  require that the execution of each basic
   block of the function has a net change of 0 on the stack. This
   routine returns 1 if this requirement is met, and the function does
   not have too many insturctions (> limit).  We except the basic
   block containing the function's epilog from this requirement.
*/
/* The function was changed, Jan, 1991. It is more liberal now.
   It scans the flow graph rather than the flat function body, and
   therefore it can calculate the run time depth at an instruction
   without the requirement that the stack level is unchanged by the
   basic blocks. The limit on the number of instruction was also
   removed.
*/
#define CLEAR		0
#define TOUCHED		1
#define PROCESSED	2
static 
process_body (pn)
register NODE *pn; /*first instruction after the prolog */
{
	BLOCK *b;

	bldgr(false);
	set_refs_to_blocks(); /* connect switch tables nanes to blocks */
	for (b = b0.next; b ; b = b->next) { /*init*/
		b->marked = CLEAR;
		b->entry_depth = b->exit_depth =0;
	}
	for (b = b0.next; b ; b = b->next)
		if (b->marked != PROCESSED) {
			if (process_block(b,pn) == 0)
				return 0;
		}

	return 1;
}


static int
process_block(b,pn) BLOCK *b; NODE *pn;
{
BLOCK *b1;
NODE *p;
REF *r;
SWITCH_TBL *sw;

boolean tmpsret = false;

		b->marked = PROCESSED;
		if (func_data.two_entries)
			if (b == b0.next)
				return 1; 
			else if (b == b0.next->next)
				p = pn;
			else p = b->firstn;
		else p = b == b0.next ? pn : b->firstn;
		b->exit_depth = b->entry_depth;
		for ( ; p != b->lastn->forw; p = p->forw) {
			if (p->uses&EBP && p->op != PUSHA)
				p->extra = (short) b->exit_depth;
			if (tmpsret && p->op == CALL) {
					p->op3 = "/TMPSRET";
					tmpsret = false;
					b->exit_depth -= 4;
			}				
			if (p->sets&ESP  ) {
				b->exit_depth += process_push_pop(p,0);
				if (p->extra == TMPSRET)
					tmpsret = true;
			}
			if (p->op == CALL && p->forw->op == LABEL  
			  && (strcmp(p->op1,p->forw->opcode) == 0))
				b->exit_depth += 4;
		}
		if (b->nextl) {
			if (b->nextl->marked > CLEAR) {
				if (b->nextl->entry_depth != b->exit_depth) {
					return 0;
				} else if (b->nextl->marked != PROCESSED) {
					if (process_block(b->nextl,pn) == 0)
						return 0;
				}
			} else {
				b->nextl->entry_depth = b->exit_depth;
				b->nextl->marked = TOUCHED;
				if (process_block(b->nextl,pn) == 0)
					return 0;
			}
		}
		if (b->nextr) {
			if (b->nextr->marked > CLEAR) {
				if (b->nextr->entry_depth != b->exit_depth) {
					return 0;
				} else if (b->nextr->marked != PROCESSED) {
					if (process_block(b->nextr,pn) == 0)
						return 0;
				}
			} else /* if (b->nextr->marked == CLEAR) */ {
				b->nextr->marked = TOUCHED;
				b->nextr->entry_depth = b->exit_depth;
				if (process_block(b->nextr,pn) == 0)
					return 0;
			}
		}
		if (is_jmp_ind(b->lastn)) {
			sw = get_base_label(b->lastn->op1);
			for (r = sw->first_ref; r; r = r->nextref) {
				if (sw->switch_table_name == r->switch_table_name) {
					b1 = r->switch_entry;
					if (b1->marked > CLEAR) {
						if (b->exit_depth != b1->entry_depth) {
							return 0;
						}
						else if (b1->marked != PROCESSED)
							if (process_block(b1,pn) == 0)
								return 0;
					} else {
						b1->marked = TOUCHED;
						b1->entry_depth = b->exit_depth;
						if (process_block(b1,pn) == 0) {
							return 0;
						}
					}
				}
				if ( r == sw->last_ref )
					break;
			}
		}
	return 1;
}

/* Calculates the stack depth after the function prolog; flags
   instructions in the "enter" sequence for removal.
   Does not handle functions that return structures, or functions
   with auto's.
*/
/*
	As from Jan. 91, it does. That enables fp elimination from
	any function.
*/
static 
process_header(first_node, stack_level,autos_pres,x)
NODE **first_node; 
int *stack_level; /* stack level after the prolog */
int autos_pres; /* function initially had auto's, but regal assigned these
		   to registers */
int x; /*value to return if there is a "push ebp" and no "mov esp ebp"*/
{
	register NODE *pn;
	int count=0;
	long pushes=0;

	/* get to beginning of prolog */
	if (func_data.two_entries) {
		for (pn = n0.forw; pn != 0; pn = pn->forw) {
			if (pn->op == PUSHL || pn->op == POPL) break;
		}
	} else {
		for (pn = n0.forw; pn != 0; pn = pn->forw) {
			if (pn->op == MOVL || pn->op == PUSHL || pn->op == POPL) break;
		}
	}

	if (!pn) {
		return 0;
	}

	/*skip over code for getting the address of returned struct, if any*/
	if (isgetaddr(pn))
		pn = pn->forw->forw;

	/* skip over profiling code */
	if (isprof( pn->forw ))
		pn = pn->forw->forw;

	/* expect saving of frame pointer */
	if (pn->op != PUSHL || !usesvar("%ebp", pn->op1)) {
		return 0;
	}
	pn->extra = double_aligned ? NO_REMOVE: REMOVE;
	pn = pn->forw;

	/* expect adjust the frame pointer */
	if (pn->op != MOVL ||
		!usesvar("%esp",pn->op1) || !usesvar("%ebp",pn->op2)) {
		eliminate_ebp();
		return x;
	}
	pn->extra = double_aligned ? NO_REMOVE: REMOVE;
	/* if optimizing alignment of doubles, need to adjust ebp */
	/* if called second time, then the andl instruction is already in */
	if (double_aligned && x==2) {
		if (pn->forw->op == ANDL
		 && pn->forw->op1[0] == '$'
		 && ((int)strtoul(&pn->forw->op1[1],(char **)NULL,0)) == -8
		 && samereg(pn->forw->op2,"%ebp")
		 )
			pn = pn->forw;
		else { /*hasnt been inserted yet, it's first time */
			pn = insert(pn);
			chgop(pn,ANDL,"andl");
			pn->op1 = "$-8";
			pn->op2 = "%ebp";
			pn->nlive = pn->back->nlive;
			pn->uses = EBP;
			pn->sets = EBP | CONCODES_AND_CARRY;
		}
	}

	pn = pn->forw;

	if (!autos_pres)
	/* EMPTY */
		;
	/* expect set up the locals */
	else if (pn->op == SUBL && samereg("%esp",pn->op2)) {
		pn->extra = (double_aligned || ! auto_elim) ? NO_REMOVE: REMOVE;
		pn = pn->forw;
	} else if (pn->op == PUSHL &&
		   samereg("%eax",pn->op1)) {
		pn->extra = (double_aligned || ! auto_elim) ? NO_REMOVE: REMOVE;
		pn = pn->forw;
	}
	else {
		fprintf(stderr,"CASE DOES HAPPEN\n");
		return 0;
	}

	/* process the register saves. Register saves start by pushing non-scratch
	   registers and end when:
		a. A scratch register is pushed.
		b. A non-scratch register is pushed for the second time
	*/
	for (; pn != 0; pn = pn->forw) {
		int reg;
		if (pn->op != PUSHL || !isreg(pn->op1) ||
			((reg=(pn->uses&~ESP))&pushes) || reg&(EAX|EDX|ECX|FP0|FP1))
			break;
		count += 4;
		pushes |= reg;

	}
	*first_node = pn;
	*stack_level = count;
	return 1;
}
 char *
getstmnt() 
{
 register char *s;
 register int c;
 static char *front, *back;		/* Line buffer */ 

#define eoi(c) ((c=='\n')||(c==';')||(c==NULL)||(c==CC)) /* end of instruction */
#define eol(c) ((c=='\n')||(c==NULL)) /* end of line */
	/* Each line of input can contain multiple instructions separated
	 * by semicolons.
	 * getstmnt() returns the next instruction as a null terminated
	 * string.
	 */

	if( front == NULL )	/* initialize buffer */
		{front = (char *)malloc(LINELEN+1);
		 if(front == NULL)
			fatal(__FILE__,__LINE__,"getstmnt: out of buffer space\n");
		 back = front + LINELEN;
		}
	/* read until end of instruction */
	s = front;
	while( (c = getchar()) != EOF )
		{
		 /* Watch for string context. */
		 if (c == '"') {
		    int in_escape = 0;

		    for (;;) {
			if(s >= back)
			    s = ExtendCopyBuf(&front,&back,(unsigned)2*(back-front+1));
			*s++ = (char)c;
		        if (((c = getchar()) == '"') && ! in_escape)
			    break;
			if (in_escape) in_escape = 0;
			else if (c == '\\') {
			    in_escape = 1;
			    /* Note: assembler will accept a string broken
				     across a line with \<newline> */
			}  /* if */
		    }  /* for */
		 }  /* if */
		 if(s >= back)
			s = ExtendCopyBuf(&front,&back,(unsigned)2*(back-front+1));
		 if(eoi(c))
			{switch(c)
				{case ';':
				 case NULL:
				 case '\n':
					*s = NULL;
					return(front);
				 case CC:
					*s++ = (char)c;
					break;
				}
			 /* here if CC, read to end of line */
			 while((c = getchar()) != EOF)
				{if(s >= back)
					s = ExtendCopyBuf(&front,&back,(unsigned)2*(back-front+1));
				 if(eol(c))
					{*s = NULL;
					 return(front);
					}
				 else
					*s++ = (char)c;
				}
			 /* premature EOF */
			 if(s > front)
				{*s = NULL;
				 return(front);
				}
			 return(NULL);
			}
		 else
			*s++ = (char)c;
		}
	/* EOF */
	if(s > front)	/* premature */
		{*s = NULL;
		 return(front);
		}
	return(NULL);
}
static char *
ExtendCopyBuf(p0,pn,nsize)
char **p0;
char **pn;
unsigned int nsize;
{
 char *b0 = *p0;
 char *bn = *pn;
 unsigned int osize;

 /* input buffer looks like:
  *
  *	-----------------------
  *	|   |   | ... |   |   |
  *	-----------------------
  *       ^                 ^
  *       |                 |
  *	 *p0               *pn == s
  *
  * where the current user pointer, s, is at the end of buffer.
  * after buffer extension, the new buffer looks like:
  *
  *	---------------------------------------------
  *	|   |   | ... |   |   |   |   | ... |   |   |
  *	---------------------------------------------
  *       ^                 ^                     ^
  *       |                 |                     |
  *      *p0                s                    *pn
  *
  * where s is at the same distance from the beginning of the buffer,
  * and the contents of the buffer from *p0 to s is unchanged,
  * and s is returned.
  */

 osize = bn - b0 + 1;
 if(nsize <= osize)
	fatal(__FILE__,__LINE__,"ExtendCopyBuf: new size <= old size\n");
 b0 = realloc(b0,nsize);
 if(b0 == NULL)
	fatal(__FILE__,__LINE__,"ExtendCopyBuf: out of space\n");
 bn = b0 + nsize - 1;
 *p0 = b0;
 *pn = bn;
 return(b0 + osize - 1);
}


#ifdef DEBUG
static NODE *
next_block(p) NODE *p;
{
BLOCK *b;
NODE *q;
int found = false;
	for (b = b0.next; b; b = b->next) {
		for (q = b->firstn; q != b->lastn->forw; q = q->forw) {
			if (q == p) {
				found = true;
				break;
			}
		}
		if (found) break;
	}
	if (!b || !b->next) return ntail.back;
	return b->next->firstn;
}/*end next_block*/
#endif

static void
stack_cleanup()
{
	NODE *p;
	NODE *last_add = NULL;
	int stack_level, stack_count, block_stack_count, fcn_count;
#ifdef DEBUG
	int first_iter = 1;
#endif

#ifdef FLIRT
	static int numexec=0;
#endif

	COND_RETURN("stack_cleanup");

#ifdef FLIRT
	numexec++;
#endif

	if (!process_header(&p, &stack_level, numauto,0)) {
		return;
	}

	ldanal();
	stack_count = block_stack_count = 0;

	for (; p != &ntail ; p = p->forw) {
#ifdef DEBUG
		if (first_iter || ((islabel(p) || isbr(p->back)) && stack_count == 0)) {
			first_iter = 0;
			COND_SKIP(p = next_block(p),"%d ",second_idx,0,0);
		}
#endif
		if (((((p->op >= CMPB && p->op <= TESTL) || p->op == SAHF)
			  && isbr(p->forw)) ||
			 p->op == ASMS || isbr(p) || islabel(p) || p->op == LEAVE ||
			 (p->op == POPL && strcmp(p->op1, "%ecx")))
		 && !stack_count) {
			if (block_stack_count > 0) {
				if (process_push_pop(last_add,1) != block_stack_count) {
					FLiRT(O_STACK_CLEANUP,numexec);
					if (block_stack_count == 4 && ! (p->nlive & ECX)) {
						chgop(last_add,ADDL,"addl");
						last_add->op1 = "$4";
						last_add->op2 = "%esp";
						last_add->uses = ESP;
						last_add->sets = ESP | CONCODES_AND_CARRY;
					} else if (last_add->op == ADDL && *last_add->op1 == '$') {
						last_add->op1 = getspace(ADDLSIZE);
						sprintf(last_add->op1, "$%d", block_stack_count);
					} else if (p->op == LEAL) {
						last_add->op1 = getspace(LEALSIZE);
						sprintf(last_add->op1, "%d(%%esp)", block_stack_count);
					} else if (last_add->op == POPL) {
						chgop(last_add, LEAL, "leal");
						last_add->uses = ESP;
						last_add->sets = ESP;
						last_add->op1 = getspace(LEALSIZE);
						sprintf(last_add->op1, "%d(%%esp)", block_stack_count);
						last_add->op2 = "%esp";
					}
					if (!((p->uses | p->nlive) & CONCODES_AND_CARRY) &&
						!(last_add->op == POPL &&
						(!islabel(p) && p->nlive & ECX ||
						  islabel(p) && p->back->nlive & ECX))) {
						DELNODE(last_add);
						INSNODE(last_add, p);
						lexchin(p, last_add);
					}
				} else if (block_stack_count == 4 && ! (p->nlive & ECX) ) {
						chgop(last_add, POPL, "popl");
						last_add->op1 = "%ecx";
						last_add->op2 = NULL;
						last_add->uses = ESP;
						last_add->sets = ESP | ECX;
						FLiRT(O_STACK_CLEANUP,numexec);
				}
				last_add = NULL; 
				block_stack_count = 0;
				if (p->op >= CMPB && p->op <= TESTL)
					p = p->forw;
			}
		}
		else if (p->sets & ESP && p->extra != TMPSRET) {
			stack_count += process_push_pop(p,1);
		}
		else if (p->op == CALL || p->op == LCALL) {
			p = p->forw;
			if (p->sets & ESP && p->op != LEAVE &&
				(p->op != POPL || !strcmp(p->op1, "%ecx")))
			   if ((fcn_count = -process_push_pop(p,1)) ==
				   stack_count) {	/* all current calls are done */
				block_stack_count += stack_count;
				stack_count = 0;
				if (last_add)
					DELNODE(last_add);
				last_add = p;
			   }
			   else if (fcn_count >= 0) {
				stack_count -= fcn_count;
			   }
			   else {		/* another push instruction? */
				p = p->back;
			   }
			else		/* end of block? */
			   p = p->back;
		}
	}
}/*end stack_cleanup*/

void
sets_and_uses()
{
NODE *p;
	for(ALLN(p)) {
		p->sets = sets(p);
		p->uses = uses(p);
	}
}/*end sets_and_uses*/

static char *
bi2bp(op) char *op;
{
char *t;
	if (!strcmp(op,"*%ebi")) return "*%ebp";
	if (!isreg(op)) {
		while ((t = strstr(op,"%ebi")) != 0) {
			t+=3;
			*t = 'p';
		}
		while ((t = strstr(op,"%bi")) != 0) {
			t += 2;
			*t = 'p';
		}
	} else {
		if (scanreg(op,false) == EBI)
			return "%ebp";
		else if (scanreg(op,false) == BI)
			return "%bp";
	}
	return op;
}/*end bi2bp*/

static void
forget_bi()
{
	NODE *p;
	int i;
	for (ALLN(p))
		for (i = 1; i <= 3; i++)
			if (p->ops[i] && (scanreg(p->ops[i],0) & EBI)) {
				p->ops[i] = bi2bp(p->ops[i]);
				if (p->uses & Ebi) {
					p->uses &= ~EBI;
					p->uses |= EBP;
				} else if (p->uses & BI) {
					p->uses &= ~BI;
					p->uses |= BP;
				}
				if (p->sets & Ebi) {
					p->sets &= ~EBI;
					p->sets |= EBP;
				} else if (p->sets & BI) {
					p->sets &= ~BI;
					p->sets |= BP;
				}
			}
}/*end forget_bi*/

static void
eliminate_ebp()
{
NODE *p;
	for (ALLN(p)) {
		if ((p->uses | p->sets) & EBP)
			DELNODE(p);
	}
}/*end eliminate_ebp*/


static void
hide_safe_asm()
{
NODE *p;
	if (!found_safe_asm) 
		return; /* save time */
	for (ALLN(p))
		if (p->op > SAFE_ASM) {
			p->op -= SAFE_ASM;
			p-> sasm = SAFE_ASM;
		}
}/*end hide_safe_asm*/

static void
recover_safe_asm()
{
NODE *p;
	if (!found_safe_asm) 
		return; /* save time */
	for (ALLN(p)) {
		p->op += p->sasm;	
	}
}/*end recover_safe_asm*/

static void
reset_crs()
{
NODE *p;
int i;
	for (ALLN(p)) {
		for (i = 1; i < 3; i++) {
			if (p->ops[i] && p->ops[i][0] == '&' &&
				(!strncmp(p->ops[i],"&cr",3) || !strncmp(p->ops[i],"&tr",3) ||
				 !strncmp(p->ops[i],"&dr",3)))
				*p->ops[i] = '%';
		}
	}
}/*end reset_crs*/

#ifdef DEBUG
static boolean
holds_const(node,limit) NODE *node, *limit;
{
NODE *q;
int reg = scanreg(node->op1,false);
	for (q = node->back; q != limit; q = q->back) {
		if ((q->sets & ~CONCODES_AND_CARRY) == reg) {
			if (isconst(q->op1) && isdigit(q->op1[1]))
				return true;
			else
				reg = q->uses;
		}
	}
	return false;
}/*end holds_const*/

static boolean
change_esp_by_const(node,limit)
NODE *node, *limit;
{
	switch(node->op - SAFE_ASM) {
	case PUSHL: case PUSHW: case POPL: case POPW:
	case PUSHFL: case PUSHFW: case POPFL: case POPFW:
		return true;
	case LEAL:
		if (node->uses == ESP && node->sets == ESP)
			return true;
		else
			return false;
		/* NOTREACHED */
		break;
	case SUBL: case ADDL:
		if (isreg(node->op1) && samereg(node->op2,"%esp")) {
			if (holds_const(node,limit))
				return true;
			else
				return false;
		} else if (*node->op1 == '$' && isdigit(node->op1[1])) {
			return true;
		} else
			return false;
	/* FALLTHRU */
	case LEAVE:
		return true;
	default:
		return false;
	}
	/* NOTREACHED */
}

static boolean
verify_safe_asm()
{
NODE *p,*q,*r,*last;
int found;
boolean retval = true;
	for (ALLN(p)) {
		if (is_safe_asm(p)) {
			for (last = p; !is_safe_asm(last) ; last = last->forw) ;
				for (q = p; q != last; q = q->forw) {
					if (sa_islabel(q)) {
						for (r = n0.forw; r != p; r = r->forw) {
							if ((isbr(r) || sa_isbr(r))
								&& !strcmp(q->opcode,r->op1)) {
									fprintf(stderr,
									"UNSAFE: label refed from before ");
									fprinst(q); fprinst(r); 
									retval = false;
								}
						}
						for (r = last; r != &ntail; r = r->forw) {
							if ((isbr(r) || sa_isbr(r))
								&& !strcmp(q->opcode,r->op1)) {
									fprintf(stderr,
									"UNSAFE: label refed from after ");
									fprinst(q); fprinst(r); 
									retval = false;
								}
						}
					}
					if (sa_isbr(q) && q->op1) {
						found = false;
						for (r = p; r != last; r = r->forw) {
							if (sa_islabel(r) && !strcmp(q->op1,r->opcode))
								found = true;
						}
						if (!found) {
							fprintf(stderr,"UNSAFE: br target out of asm");
							fprinst(q); 
							retval = false;
						}
					}
					if (!fp_removed && q->sets & EBP) {
						fprintf(stderr,"UNSAFE: sets EBP ");
						fprinst(q); 
						retval = false;
					}
					if ((q->sets & ESP) && !change_esp_by_const(q,p)) {
						fprintf(stderr,"UNSAFE: sets ESP ");
						fprinst(q); 
						retval = false;
					}
				}
		p = last;
		}
	}
	return retval;
}/*end verify_safe_asm*/

boolean
last_one()
{
	if (not) return (not == d);
	return (opt_idx == last_opt);
}

boolean
last_func()
{
	static boolean seen_before = false;
	if (max == d) {
		if ( ! seen_before ) {
			fprintf(stderr,"\n");
			seen_before = true;
		}  /* if */
		return true;
	}  /* if */

	return false;
}

static void
get_env()
{
char *str;
				str=getenv("max"); /* last function */
				if (str) max=atoi(str);
				else max=9999;
				str=getenv("min"); /* first function */
				if (str) min=atoi(str);
				else min=0;
				str=getenv("not"); /* first function */
				if (str) not=atoi(str);
				else not=0;
				str = getenv("from"); /* first optimization */
				if (str) first_opt = atoi(str);
				else first_opt = 0;
				str = getenv("to"); /* last optimization */
				if (str) last_opt = atoi(str); 
				else last_opt = 999999;
				str = getenv("start"); /* first loop to be optimized */
				if (!str) start = 0;
				else start = atoi(str);
				str = getenv("finish"); /* last loop to be optimized */
				if (!str) finish = 100000;
				else finish = atoi(str);
				str = getenv("min5");
				if (str) min5 = atoi(str);
				else min5 = 0;
				str = getenv("max5");
				if (str) max5 = atoi(str);
				else max5 = 9999;
				str = getenv("SKIP_FP");
				if (str) skip_fp_elim = true;
				else skip_fp_elim = false;
}

#endif /* def DEBUG */

boolean
cond_return(s) char *s;
{
	/* This function returns true if optimization 's' should _not_ be
	   performed for a particular function.  This may either be due to a
	   /SUPPRESS_<optimization-name> directive already seen for the
	   function, or when built with DEBUG, due to -Kenv from/to limits. */

	suppress_list_type* sl = suppress_list;
	while (sl != NULL) {
		if (strcmp(sl->optimization_name,s) == 0) {
#if defined(DEBUG) && !defined(FLIRT)
			if (first_opt && last_func())
				fprintf(stderr, "suppressing %d %s!, \n", ++opt_idx, s);
#endif
			return true;
		}
		sl = sl->next;
	}

#ifdef DEBUG
	if (first_opt && last_func() ) {
		opt_idx++;
		if (opt_idx < first_opt || opt_idx > last_opt) return true;
		else {
#ifndef FLIRT
			fprintf(stderr,"%d %s, ",opt_idx,s);
#endif
			second_idx = 0;
			return false;
		}
	}
#endif

	return false;
}

void
find_first_and_last_fps()
{
NODE *p;
extern NODE *first_fp, *last_fp;
	first_fp = last_fp = NULL;
	/* initialize special nodes */
	for (p = n0.forw; p != &ntail; p = p->forw)
		if (isfp(p)) {
			first_fp = p;
			break;
		}
	for (p = ntail.back; p != &n0; p = p->back)
		if (isfp(p)) {
			last_fp = p;
			break;
		}
}/*end find_first_and_last_fps*/

/*reset global variables */
static void
init_globals()
{
	numauto = 0;  /* reset for next rtn */
	asmflag = false;
	rvregs = 1; /* re-init ret value reg cnt */
	autos_pres_el_done = false; /*reset*/
	fp_removed = false;
	suppress_enter_leave = never_enter_leave; /*reset*/
#if EH_SUP
	must_use_frame = false;
#endif
	found_safe_asm = false; /*reset */
	fixed_stack = 0;
	frame_reg = EBP | ESP;
	set_FSZ = true;
	last_branch = 0;
	func_data = init_func_data;
	regs_for_out_going_args = 0;
	regs_for_incoming_args = 0;
	asz_list.name = 0;
	asz_list.size = 0;
	asz_list.next =  NULL;
	asz_walk = NULL;
#if EH_SUP
	try_block_nesting = 0;
	try_block_index = 0;
	suppress_list = NULL;
#endif
}

static void
unmark_intrinsics()
{
NODE *p;
	for (ALLN(p)) p->usage = 0;
}

/* Function reoders stack locals and saved registers
** Goes in steps as follows:
** 1. find the subl $numauto,%esp in the prolog, and also the last save reg.
** count the number of saved regs.
** 2. move the subl instruction after the last save reg instruction.
** 3. find all the epilogs. in each one, move the addl $numauto,%esp instruction
** before the first restore instruction.
** 4. change the function body: instruction that use %esp as an index register
** should change the offset by the size of the area of the saved registers.
** EXTERNAL DEPENDENCE: remove_enter_leave marks the instructions that use the
** autos at p->esp_offset, and xchg_save_locals uses this mark to be able to
** change the offset for the autos and not for the params.
** !!!  It is nessesary to preserve "esp_offset" for all NODEs using %esp 
** between remove_enter_leave() and xchg_save_locals(). 
*/
static void 
xchg_save_locals()
{
NODE *p, *sub ,*p1,  *add;
int cnt=-1,cnt1=-1, i, offs;
char *op ,*cp;
int num_of_merg=0, num_of_ret=0;

#ifdef FLIRT
static int numexec=0;
#endif

#ifdef DEBUG
	if (getenv("no_reord_stack")) return;
#endif
	
    COND_RETURN("xchg_save_locals");

#ifdef FLIRT
	++numexec;
#endif

	/* If there is a frame pointer the reorder is meaningless, as the
	** instruction before the ret is mov ebp,esp and not addl,$n,esp,
	** and there is no AGI.
	*/
	if (suppress_enter_leave || double_aligned) return;

	/* get to beginning of prolog */
	for (p = n0.forw; p != 0; p = p->forw) {
		if (islabel(p) ) break;
	}

	if ( (p=p->forw) == NULL ) return;

	/*skip over code for getting the address of returned struct, if any*/
	if (isgetaddr(p)) 
		p = p->forw->forw;

	/* skip over profiling code */
	if (isprof( p->forw ))
		p = p->forw->forw;

	/* Case using "push %eax" for locals. Change to sub in order to be able to
	** combine with a possible next subl from esp.
	*/
	if ( p->op==PUSHL && samereg(p->op1,"%eax") ){
		chgop(p,SUBL,"subl");
		FLiRT(O_XCHG_SAVE_LOCALS,numexec);
		p->op1="$4";
		p->op2="%esp";
		p->sets |= CONCODES_AND_CARRY;
	}			
	if (p->op != SUBL || p->op1[0] != '$' || !samereg(p->op2,"%esp")	
	    || p->forw->op != PUSHL || !(setreg(p->forw->op1)&(ESI|EDI|EBP|EBX))) {
		return;
	}

	sub=p;
	do{  
		p=p->forw;
		cnt++;  /* count the saved registers */
	} while (p->op == PUSHL && setreg(p->op1)&(ESI|EDI|EBP|EBX));
	if (cnt == 0) return;
	p1=p;

/* find all RET */
	for(p = ntail.back; p!=&n0 ; p = p->back)
	{
		if (p->op != RET ) continue;
		p=p->back;
		if (p->op!=ADDL) return ;
		cnt1=-1;
		do {
			p=p->back;
			cnt1++;
		}while (p->op == POPL &&  cnt1<cnt && setreg(p->op1)&(ESI|EDI|EBP|EBX));
		if( cnt1 != cnt ) return;
		if (p->op == ADDL) num_of_merg++;
		num_of_ret++;
	}

	/* exchange on the top  */
	FLiRT(O_XCHG_SAVE_LOCALS,numexec);
	p1=p1->back;
	move_node(sub,p1);

	/*this opt. runs after reordbot, there are several epilogs. */
	/* exchange on the all RET  */
    for(p = ntail.back; p!=&n0 ; p = p->back){
		if (p->op == RET ){
			p=p->back;
			add=p;
			cnt1=-1;
			do  p=p->back,cnt1++; while (p->op == POPL && cnt1<cnt );
			if( p->op == ADDL && p->op1[0] == '$' && samereg(p->op2,"%esp")){
				int n = (int)strtoul(p->op1+1,(char **)NULL,0);
				int m = (int)strtoul(add->op1+1,(char **)NULL,0);
				char *op = getspace(ADDLSIZE+strlen(p->op1));
				sprintf(op,"$%d",m+n);
				p->op1 = op;
				FLiRT(O_XCHG_SAVE_LOCALS,numexec);
				DELNODE(add);
			}
			else 
				move_node(add,p);
		}
	}

	/* Go over the code and change the offset in operand that use the stack
	** pointer as an index register: subtract the size of the saved registers
	** area from the offset.
	*/
	cnt<<=2;
	for (ALLN(p)){
		if ( p->uses & ESP  &&  p->esp_offset ){
			for(i=1;i<=3;i++){
				if (p->ops[i] == NULL ) continue;
				if ( (cp = strchr(p->ops[i],'('))  == NULL) continue;
      			if (scanreg( p->ops[i],false ) & ESP ){
      				int star=0;
					if ( *(p->ops[i])=='*')  star=1;
      				offs=(int)strtoul((p->ops[i]+star),(char **)NULL,0);
					op = getspace(ADDLSIZE+strlen(p->ops[i]));
					if(star) sprintf(op,"*%d%s",offs-cnt,cp);
					else sprintf(op,"%d%s",offs-cnt,cp);
					p->ops[i]=op;
				}
			}
			FLiRT(O_XCHG_SAVE_LOCALS,numexec);
			p->esp_offset=0;
		}
	}
}/*end xchg_save_locals*/

static void
replace_asz()
{
NODE *p;
int i;
int offset;
asz_element *walk;
char *s;
int x;
	for (ALLN(p)) {
		for (i = 1; i <= 3; i++) {
			if (p->ops[i] && (s = strstr(p->ops[i],".ASZ"))) {
				s += 4;
				x = (int)strtoul(s,(char **)NULL,0);
				offset = (int)strtoul(p->ops[i],(char **)NULL,0);
				for (walk = asz_list.next; walk; walk = walk->next) {
					if (x == walk->name) {
						offset += walk->size;
						break;
					}
				}
				if (strstr(p->ops[i],"%esp")) {
					p->ops[i] = getspace(LEALSIZE);
					sprintf(p->ops[i],"%d(%%esp)",offset);
				} else {
					p->ops[i] = getspace(LEALSIZE);
					sprintf(p->ops[i],"$%d",offset);
					p->zero_op1 = 1;
				}
			}
		}
	}
}
/*This is to work around a problem with the combination of:
**1. fixed stack
**2. indirect function call, where the func addr is on the stack, and
**3. The calle returns a struct.
**The calling sequence in this case looks like:
**movl func_addr, -12+.FSZ(%esp)
**movl arg 0(%esp) -- any number of these
**pushl %eax -- addr for the returned struct
**call -8+.FSZ(%esp) --> the operand is the same as above but LOOK DIFFERENT,
**8 v.s. 12, and optim relies heavily on same REGAL operands looking the same,
**both EBP based in exact frame before fp-elim is done, and in fixed-frame.
**Trainning optim to associate these two operands is too messy and error prone,
**and faking it is a better solution.
*/

static void
match_ind_calls()
{
NODE *p;
char *s;
int x;
	if (!fixed_stack) return;
	for (ALLN(p)) {
		if (p->extra == TMPSRET && p->forw->op == CALL 
			&& *(p->forw->op1) == '*' && scanreg(p->forw->op1,0) & ESP) {
			s = getspace(LEALSIZE);
			x = (int)strtoul(p->forw->op1+1,(char **)NULL,0);
			sprintf(s,"*%d+%s%d(%%esp)",x-4,frame_string,func_data.number);
			p->forw->op1 = s;
		}
	}
}

static void
restore_ind_calls()
{
NODE *p;
char *s;
int x;
	if (!fixed_stack) return;
	for (ALLN(p)) {
		if (p->extra == TMPSRET && p->forw->op == CALL 
			&& *(p->forw->op1) == '*' && scanreg(p->forw->op1,0) & ESP) {
			s = getspace(LEALSIZE);
			x = (int)strtoul(p->forw->op1+1,(char **)NULL,0);
			sprintf(s,"*%d+%s%d(%%esp)",x+4,frame_string,func_data.number);
			p->forw->op1 = s;
		}
	}
}

#if EH_SUP
static void
force_frame()
{
NODE *push = NULL;
NODE *movl;
NODE *p,*q, *popl = NULL;
	COND_RETURN("force_frame");
	for (ALLN(p)) {
		if (p->op == PUSHL && setreg(p->op1) == EBP) {
			push = p;
			break;
		}
	}
	if (push == NULL) {
		push = insert(n0.forw);
		chgop(push,PUSHL,"pushl");
		push->op1 = "%ebp";
		push->op2 = NULL;
	}
	movl = push->forw;
	if (movl->op == MOVL && setreg(movl->op1) == ESP 
		&& setreg(movl->op2) == EBP)
		return;
		
	movl = insert(push);
	chgop(movl,MOVL,"movl");
	movl->op1 = "%esp";
	movl->op2 = "%ebp";

	for (ALLN(p)) {
		if (p->op == RET) {
			for (q = p; q != n0.forw ; q = q->back) {
				if (q->op == POPL && setreg(q->op1) == EBP) {
					popl = q;
					break;
				}
			}
			if (popl == NULL) {
				popl = prepend(p,NULL);
				chgop(popl,POPL,"popl");
				popl->op1 = "%ebp";
			}
			movl = popl->back;
			if (movl->op == MOVL && setreg(movl->op1) == EBP 
				&& setreg(movl->op2) == ESP)
				continue;
			movl = prepend(popl,NULL);
			chgop(movl,MOVL,"movl");
			movl->op1 = "%ebp";
			movl->op2 = "%esp";
		}
	}
}

#endif

#ifdef DEBUG
void
calc_frame_offsets()
{
NODE *p;
int i;
int offset;
	for (ALLN(p)) {
		for (i = 1; i <= 3; i++) {
			if (scanreg(p->ops[i],1) == ESP) {
				offset = (int)strtoul(p->ops[i],(char **)NULL,0);
				if (strstr(p->ops[i],".FSZ")) offset += func_data.frame;
				if (strstr(p->ops[i],".RSZ")) offset -= func_data.regs_spc;
				p->ops[i] = getspace(LEALSIZE);
				sprintf(p->ops[i],"%d(%%esp)",offset);
			}
		}
	}
}
#endif


#ifdef EMON
#include <sys/stat.h>


static char *unix_em_index_fname = "/tmp/index.em";
static char *unix_em_routines_fname = "/tmp/routines.em";

static FILE *em_index_fid,*em_routines_fid;
static int em_last_index;
static char *routine_name;

void
pcg_begin()
{
    struct stat stbuf;
    char *em_index_file;
    char *em_routines_file;


	    em_index_file = unix_em_index_fname;
	    em_routines_file = unix_em_routines_fname;
	if (stat(em_index_file,  &stbuf) == -1) {
	  /* index.em file does not exist, create it */
	    em_index_fid = fopen(em_index_file, "w");
	    if (em_index_fid == NULL) {
		printf("Unable to open %s file. emon will not be activated.\n", 
	    		em_index_file);
	    	dflag = false;
	    }
	    em_last_index = 0;
	    function_index = 0;
	}
	else {
	    em_index_fid = fopen(em_index_file, "r+");
	    if (em_index_fid == NULL) {
	        printf("Unable to open %s file. emon will not be activated\n", 
			em_index_file);
	        dflag = false;
	    }
	    else {
	        fscanf(em_index_fid, "%d\n", &em_last_index);
	        if (em_last_index < 0)
		    	em_last_index = 0;
		    function_index = em_last_index;
	    }
	}
	em_routines_fid = fopen(em_routines_file, "a");
	if (em_routines_fid == NULL) {
	    printf("Unable to open %s file, emon will not be activated.\n",
		em_routines_file);
	    dflag = false;
    }
}


void
pcg_end()
{
		fseek(em_index_fid, 0L, 0);
		fprintf(em_index_fid, "%d\n", function_index);
}


void
pcg_routine_end()
{
		if (function_index >= 2048) {
			fprintf(stderr, "**WARNING** em_database index >=2048\n");
			fprintf(stderr, "            Please increase MAX_ENTRY size \n");
			fprintf(stderr, "            Or break application into small groups \n");
		}
		routine_name = get_label();
		fprintf(em_routines_fid, "%d %s %s\n", function_index,
			input_file_name, routine_name);
}

char *
ibasename(name) char *name;
{
char *base_name = name;
int len = strlen(base_name);
	if (base_name[len-2] == '.') base_name[len-2] = (char) 0;
	return base_name;
}

void
add_emons()
{
NODE *p,*q;
	for (ALLN(p)) {
		switch (p->op) {
			/* Epilog add: 
        			pushl     $n
        			call      em_stop
        			addl      $4, %esp
			*/
			case RET:
				q = prepend(p,NULL);
				chgop(q,PUSHL,"pushl");
				q->op1 = getspace(ADDLSIZE);
				sprintf(q->op1,"$%d",function_index);
				q->op2 = NULL;
				q = insert(q);
				chgop(q,CALL,"call");
				q->op1 = "em_stop";
				q->op2 = NULL;
				q = insert(q);
				chgop(q,ADDL,"addl");
				q->op1 = "$4";
				q->op2 = "%esp";
				break;
			/* Before call:
        			pushl     $n
        			call      em_stop
        			addl      $4, %esp
			   After call:
        			pushl     $0
        			pushl     $n
        			call      em_start
        			addl      $8, %esp
			*/
			case CALL:
				q = prepend(p,NULL);
				chgop(q,PUSHL,"pushl");
				q->op1 = getspace(ADDLSIZE);
				sprintf(q->op1,"$%d",function_index);
				q->op2 = NULL;
				q = insert(q);
				chgop(q,CALL,"call");
				q->op1 = "em_stop";
				q->op2 = NULL;
				q = insert(q);
				chgop(q,ADDL,"addl");
				q->op1 = "$4";
				q->op2 = "%esp";
				q = insert(p);
				chgop(q,PUSHL,"pushl");
				q->op1 = "$0";
				q->op2 = NULL;
				q = insert(q);
				chgop(q,PUSHL,"pushl");
				q->op1 = getspace(ADDLSIZE);
				sprintf(q->op1,"$%d",function_index);
				q->op2 = NULL;
				q = insert(q);
				chgop(q,CALL,"call");
				q->op1 = "em_start";
				q->op2 = NULL;
				q = insert(q);
				chgop(q,ADDL,"addl");
				q->op1 = "$8";
				q->op2 = "%esp";
				p = q;
				break;
		}
	}

	/*Prolog add: 
        pushl     $1
        pushl     $n       / n is the function index
        call      em_start
        addl      $8, %esp
	*/
	p = n0.forw;
	while ( is_any_label(p) ) p=p->forw;
	p=p->back;
	p=insert(p);
	chgop(p,PUSHL,"pushl");
	p->op1 = "$1";
	p->op2 = NULL;
	p = insert(p);
	chgop(p,PUSHL,"pushl");
	p->op1 = getspace(ADDLSIZE);
	sprintf(p->op1,"$%d",function_index);
	p->op2 = NULL;
	p = insert(p);
	chgop(p,CALL,"call");
	p->op1 = "em_start";
	p->op2 = NULL;
	p = insert(p);
	chgop(p,ADDL,"addl");
	p->op1 = "$8";
	p->op2 = "%esp";

	/* after "main:" Add: 

        pushl     $em_exit
        call      atexit
        addl      $4, %esp
	*/
	if (islabel(n0.forw) && !strcmp(n0.forw->opcode,"main")) {
		p = insert(n0.forw);
		chgop(p,PUSHL,"pushl");
		p->op1 = "$em_exit";
		p->op2 = NULL;
		p = insert(p);
		chgop(p,CALL,"call");
		p->op1 = "atexit";
		p->op2 = NULL;
		p = insert(p);
		chgop(p,ADDL,"addl");
		p->op1 = "$4";
		p->op2 = "%esp";
	}

}
#endif
