#ident	"@(#)amigo:common/interface.c	1.94"
#include "amigo.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pfmt.h>

#ifndef NODBG
#include "costing.h" /* to allow fiddling costing parameters */
int costing_call_count, nodes_costed_count;
int junk;	/*
		** Variable that can be set with JUNK=... in environment
		** to use for miscellaneous debugging.
		*/
#ifndef __STDC__
		long strtol();
		extern char * getenv();
#endif
#endif

#include <ctype.h>

static int do_loop_unrolling = 1;
static int do_redundant_loop= 1;
extern Arena global_arena; /* defined in cg/common/comm2.c */
Arena file_arena = 0;
int do_amigo_flag = 1;

void init_depth_first();
void remove_unreachable();

#ifndef NODBG /* gather timing stats for optimizations */

extern reg_alloc_ran;
unsigned opt_min=0;
unsigned opt_max=0xffffffff;
unsigned opt_curr;
static unsigned func_min=0;
static unsigned func_max=0xffffffff;
unsigned func_curr;
struct flag_data aflags[] = {	/*
				** Array is indexed by flag_enum,
				** conditionally defined in debug.h
				*/
	{0,1,"ah"},	/* redundant loop */
	{0,1,"all"},	/* level one debugging for all amigo opts */
	{0,1,"bb"},	/* basic block */
	{0,1,"bp"},	/* boolean propagation */
	{0,1,"bs"},	/* binary search */
	{0,1,"cf"},	/* constant folding */
	{0,1,"cm"},	/* code motion */
	{0,1,"cp"},	/* copy propagation */
	{0,1,"cse"},	/* local cse */
	{0,1,"cst"},	/* costing */
	{0,1,"df"},	/* data flow */
	{0,1,"ds"},	/* dead store */
	{0,1,"env"},	/* set tunables with getenv */
	{0,1,"ep"},	/* expression propagation */
	{0,1,"exp"},	/* expressions */
	{0,1,"fs"},	/* function stats */
	{0,1,"funcs"},	/* functions to optimize */
	{0,1,"h"},	/* expression hashing */
	{0,1,"jj"},	/* jumps to jumps */
	{0,1,"l"},	/* loop information */
	{0,1,"li"},	/* local information */
	{0,1,"ls"},	/* loop simplification */
	{0,1,"lu"},	/* loop unrolling */
	{0,1,"md"},	/* machine dependent opt */
	{0,1,"mem"},	/* memory allocation */
	{0,1,"opts"},	/* optimization instance to do */
	{0,1,"post"},	/* post_amigo_rewrite */
	{0,1,"pre"},	/* pre_amigo_rewrite */
	{0,1,"ra"},     /* global register allocation */
	{0,1,"sc"},     /* scopes */
	{0,1,"sp"},     /* spill code */
	{0,1,"sr"},	/* strength reduction */
	{0,1,"ta"},	/* trees after optimization */
	{0,1,"tb"},	/* trees before optimization */
	{0,1,"tbcp"},	/* trees before copy propagation */
	{0,1,"tblu"},	/* trees before loop unrolling */
	{0,1,"tbra"},	/* trees before reg_alloc */
	{0,1,"tff"},	/* tune fixed frame */
	{0,1,"tm"},	/* timing info */
	{0,1,"un"},	/* unreachable code removal */
	{0,1,""}	/* sentinel ( indexed by flag_max ) */
}, save_aflags[flag_max];

static void
usage() {
	int i;
	DPRINTF("usage: -G [OPT:OPT .. :OPT:[funcs[=<n1>-<n2>]]:[opts[=<n1>-<n2>]]:[v<n1><n2>\n");
	DPRINTF("\tOPT :- [~] OPTION <level>\n");
	
	DPRINTF("\tOPTION :- ");
	DPRINTF("%s",aflags[0].flag_name);
	for( i = 1; i < flag_max; i++ )
		DPRINTF("%s | %s", i%8 ? "" : "\n\t\t", aflags[i].flag_name);
	DPRINTF("\n");
	exit(1);
}

	/*
	** Search the environment for tunables.
	*/
static void
get_env()
{
	char *str; 
	char **ptr = (char **)malloc(sizeof(char *));

	str = getenv("SR_THRESHOLD");
	if(str) {
		long val = strtol(str,ptr,10);
		if(*ptr != str) sr_threshold = (int)val;
		/* overrides default */
	}
	
	str = getenv("REG_SLOPE");
	if(str) {
		long val = strtol(str,ptr,10);
		if(*ptr != str) reg_slope = (int)val;
	}

	str = getenv("REG_HEIGHT");
	if(str) {
		long val = strtol(str,ptr,10);
			if(*ptr != str) reg_height = (int)val;
		}

	str = getenv("LU_THRESHOLD");
	if(str) {
		long val = strtol(str,ptr,10);
		if(*ptr != str) lu_threshold = (int)val;
	}
	
	str = getenv("LU_L_SIZE_THRESHOLD");
	if(str) {
		long val = strtol(str,ptr,10);
		if(*ptr != str) lu_l_size_threshold = (int)val;
	}
	
	str = getenv("LU_F_CALL_PENALTY");
	if(str) {
		long val = strtol(str,ptr,10);
		if(*ptr != str) lu_f_call_penalty = (int)val;
	}
	
	str = getenv("EXP_THRESHOLD");
	if(str) {
		long val = strtol(str,ptr,10);
		if(*ptr != str) exp_threshold = (int)val;
	}
	
	str = getenv("CM_THRESHOLD");
	if(str) {
		long val = strtol(str,ptr,10);
		if(*ptr != str) cm_threshold = (int)val;
	}
	
	str = getenv("CF_THRESHOLD");
	if(str) {
		long val = strtol(str,ptr,10);
		if(*ptr != str) cf_threshold = (int)val;
	}

	str = getenv("PASS_THRU_THRESHOLD");
	if(str) {
		long val = strtol(str,ptr,10);
		if(*ptr != str) pass_thru_threshold = (int)val;
	}

	str = getenv("CSE_THRESHOLD");
	if(str) {
		long val = strtol(str,ptr,10);
		if(*ptr != str) cse_threshold = (int)val;
	}

	str = getenv("JUNK");
	if(str) {
		long val = strtol(str,ptr,10);
		if(*ptr != str) junk = (int)val;
	}

}

static void 
set_limit(flags, min, max)
char *flags;
unsigned *min, *max;
{
	if (*flags != '=')
		return;
	*min = *max = 0;
	*min = strtol(flags+1, &flags,10);
	*max = strtol(flags+1, &flags,10);
	if (! *min || ! *max)
		usage();
}

char *
get_function_name()
{
	static char buffer[100];
	CGQ_FOR_ALL(root, index)
		if (root->cgq_op ==  CGQ_EXPR_ND2 &&
			root->cgq_arg.cgq_nd2->op == DEFNAM) {
			sprintf(buffer,"%s(%d)",root->cgq_arg.cgq_nd2->name,
				func_curr);
			return buffer;
		}
	CGQ_END_FOR_ALL
	sprintf(buffer,"%s(%d)", "NO NAME", func_curr);
	return buffer;
}

void
init_save_aflags()
{
	int i;
	static done;
	if(!done) {
		for(i=0; i<flag_max; i++)
			save_aflags[i] = aflags[i];
		done = 1;
	}
}

static void
load_aflags()
{
	int i;
	for(i=0; i<flag_max; i++) {
		if((!save_aflags[bs].level) || func_curr == func_max )
			aflags[i].xflag = save_aflags[i].xflag;
			/* If bs and level 2, only print debugging
			** for last function.
			*/
		if(!((save_aflags[bs].level) & 2) || func_curr == func_max ) {
			aflags[i].level = save_aflags[i].level;
			aflags[i].flag_name = save_aflags[i].flag_name;
		}
	}
}

#endif
	
void
amigo_flags(flags)
char *flags;
{
	char *token;
#ifndef NODBG /* Currently all amigo flags only for debugging -- PSP */
	init_save_aflags();
	for(token = strtok(flags,":"); token; token = strtok((char *)0,":")) {
		char *p = token, c;
		int i, x_flag = 0;
		char **ptr = (char **)malloc(sizeof(char *));
#ifdef MDOPT
		if(*p == 'v') {
		   /* vectorizer option */
		   mdflags(p);
		   continue;
		}
#endif
		if(*p == '~') { /* e.g., ~cse means turn off cse */
			x_flag = 1;
			++p;
			++token;
		}
		while(isalpha(*p)) ++p; /* scan to end of token */
		c = *p;
		*p= 0;
		for(i=0;i<flag_max;i++) {
			if(strcmp(token,save_aflags[i].flag_name) == 0)
				break;
		}
		if(i == flag_max) {
			usage();
		}
		if(x_flag) {
			save_aflags[i].xflag = 0;
			continue;
		}
		/* turn on -1a register allocation */
		else if (i == ra)
			++a1debug;
		*p = c;
		if(isdigit(*p)) 
			save_aflags[i].level = (unsigned)strtol(p,&p,10);
		else 
			save_aflags[i].level = 1;

		/* Process weird flags: funcs, opts, cst */

		if (i == funcs) {
			do_loop_unrolling = save_aflags[lu].xflag;
			do_redundant_loop = save_aflags[ah].xflag;
			set_limit(p, &func_min, &func_max);
		}

		else if (i == opts) 
			set_limit(p, &opt_min, &opt_max);

		else if (i == env) {
			get_env();
		}

		else if (i == all) {
			extern a1debug; /* acomp reg alloc flag (-1a), p1allo.c */
			save_aflags[bp].level |= save_aflags[i].level;
			save_aflags[cf].level |= save_aflags[i].level;
			save_aflags[cm].level |= save_aflags[i].level;
			save_aflags[cp].level |= save_aflags[i].level;
			save_aflags[cse].level |= save_aflags[i].level;
			save_aflags[ds].level |= save_aflags[i].level;
			save_aflags[ep].level |= save_aflags[i].level;
			save_aflags[funcs].level |= save_aflags[i].level;
			save_aflags[jj].level |= save_aflags[i].level;
			a1debug = 1;	
			save_aflags[lu].level |= save_aflags[i].level;
			save_aflags[md].level |= save_aflags[i].level;
			save_aflags[ra].level |= save_aflags[i].level;
			save_aflags[sp].level |= save_aflags[i].level;
			save_aflags[sr].level |= save_aflags[i].level;
			save_aflags[tff].level |= save_aflags[i].level;
			save_aflags[un].level |= save_aflags[i].level;
		}
		else if (i == cst) {

			DEBUG(save_aflags[cst].level&1,	
				("cf_threshold: %d cm_threshold:\
 %d sr_threshold: %d cse_threshold: %d\n",
				cf_threshold, cm_threshold,
				sr_threshold, cse_threshold));
			
		}
	}
	do_amigo_flag = do_amigo = save_aflags[all].xflag;
#else
	do_amigo_flag = do_amigo = 1;	
	if(*flags == '~' ) {
		if( flags[1] == 0 || strcmp(flags+1,"all") == 0) {
			do_amigo_flag = do_amigo = 0;	
		}
		else if(strcmp(flags+1,"lu") == 0) {
			do_loop_unrolling = 0;
		}
		else if(strcmp(flags+1,"ah") == 0) {
			do_redundant_loop = 0;
		}
		else 
			Amigo_fatal("illegal -G option");
	}
	else if(strcmp(flags,"lu") != 0)
		Amigo_fatal("illegal -G option");
#endif
}

/* RMA maybe optimizations return a value for changed then cleanup
   cleanup includes freeing the previous phase arena and allocated the
   next phase arena, and then calling build_expr_info_array, maybe
   we have a macro RUN_OPT(optimization,args). We need a final cleanup.
   The expr_info_array in phase arena, the kills and expr_info in the global
   arena; however, we may run into problems when running an optimiztion and
   find_kills causes a bitvector to expand, then space is lost. By the way
   we need bitvector expansion. One solution is bv extents, one solution is
   to totally reorganize the kills bitvectors when new expressions are added.
*/

Boolean
amigo()
{
	Loop first_loop; /* inner to outer ordering */
	struct Block *top_down_basic;
				/* Don't do loop opts, unless there are loops */
	Boolean has_loop, found_ind_vars;
	int dead_stores;	/* number of objects for dead store */
	int regalloc;		/* number of objects for register alloc */
	Arena reg_arena;

#ifndef NODBG
	unsigned int save_opt = opt_curr;
	costing_call_count = 0, nodes_costed_count = 0;
	++func_curr;
	if (save_aflags[funcs].level&1 && (func_curr < func_min || func_curr > func_max)) 
		return 0;
	load_aflags();
#endif

	PUSH_TIME; /* Global AMIGO timer */
      	if(! file_arena) file_arena = arena_init();

	pre_amigo_rewrite();

	PUSH_TIME;
	hash_exprs();
	ACCUMULATE_TIME(Hash_exprs);

	DEBUG_UNCOND(if (aflags[cf].xflag))
	{
	PUSH_TIME;
	local_const_folding();
	ACCUMULATE_TIME(Local_const_folding);
	}

	DEBUG_COND(aflags[exp].level&1,print_exprs(aflags[exp].level));

	PUSH_TIME;
	has_loop = false;
	found_ind_vars = false;
	top_down_basic=basic_block(&has_loop);
	ACCUMULATE_TIME(Top_down_basic);

	DEBUG_COND(aflags[tb].level&1, print_trees((int)aflags[tb].level,top_down_basic));

	DEBUG(aflags[fs].level&1,("before opt: blocks=%d exprs=%d occurs=%d objects=%d\n",
		get_block_count(), get_expr_count(), get_occur_count(),
		get_object_count()));

	PUSH_TIME;
	init_depth_first(top_down_basic);
	ACCUMULATE_TIME(Init_depth_first);

	DEBUG_UNCOND(if (aflags[un].xflag))
	{
	PUSH_TIME;
	remove_unreachable(top_down_basic);
	ACCUMULATE_TIME(Remove_unreachable);
	}

	PUSH_TIME;
	init_depth_first(top_down_basic);
	ACCUMULATE_TIME(Init_depth_first);
	DEBUG_COND(aflags[li].level&2,print_exprs(aflags[li].level));

	PUSH_TIME;
	local_info(top_down_basic);
	ACCUMULATE_TIME(Local_info);

	DEBUG_UNCOND(if (aflags[bp].xflag))
	{
	DEBUG(aflags[tbcp].level&4,("BLOCKS BEFORE BP\n"));
	DEBUG_COND(aflags[bp].level&4,pr_blocks(top_down_basic));
	DEBUG_COND(aflags[bp].level&2,
			print_trees(3|(aflags[bp].level),top_down_basic));

	PUSH_TIME;
	boolean_prop();
	ACCUMULATE_TIME(Boolean_prop);

	DEBUG(aflags[tbcp].level&4,("BLOCKS AFTER BP\n"));
	DEBUG_COND(aflags[tbcp].level&4,pr_blocks(top_down_basic));
	DEBUG_COND(aflags[tbcp].level&4,
			print_trees(3|(aflags[bp].level),top_down_basic));
	}

DEBUG_COND(aflags[ls].level&512,pr_blocks(top_down_basic));
	if(has_loop) {
		Boolean do_simplify = true;
		DEBUG_COND(aflags[l].level&128, print_trees((int)aflags[l].level,top_down_basic));
		PUSH_TIME;
		first_loop = loop_build(top_down_basic, &do_simplify);
		if(do_simplify) {

			do_simplify = 0;

			PUSH_TIME;
			top_down_basic=basic_block(&has_loop);
			ACCUMULATE_TIME(Top_down_basic);

			PUSH_TIME;
			init_depth_first(top_down_basic);
			ACCUMULATE_TIME(Init_depth_first);

			PUSH_TIME;
			local_info(top_down_basic);
			ACCUMULATE_TIME(Local_info);

			first_loop = loop_build(top_down_basic, &do_simplify);

			DEBUG_COND(aflags[ls].level&512,pr_blocks(top_down_basic));
			DEBUG_COND(aflags[ls].level&8,pr_blocks(top_down_basic));
			DEBUG_COND(aflags[ls].level&8, print_trees(3,top_down_basic));

		}
		ACCUMULATE_TIME(Loop_build);

		DEBUG_COND(aflags[bb].level&1,pr_blocks(top_down_basic));

#ifdef MDOPT
		PUSH_TIME;
		/* turn off with -Gv45 */
		(void)MDOPT((Cgq_index)0,
			first_loop,top_down_basic,get_block_count());
		ACCUMULATE_TIME(Vect);
#endif

		DEBUG_UNCOND(if (aflags[cm].xflag))
		{
		PUSH_TIME;
		loop_invariant_code_motion(first_loop);
		ACCUMULATE_TIME(Loop_invariant_code_motion);
		}

		DEBUG_UNCOND(if (aflags[sr].xflag))
		{
		PUSH_TIME;
		find_ind_vars(first_loop);
		found_ind_vars = true;
		strength_reduction(first_loop);
		ACCUMULATE_TIME(Strength_reduction);
		}
		DEBUG_UNCOND(if (aflags[lu].xflag))
		{
		PUSH_TIME;

		DEBUG(aflags[tblu].level,("TREES BEFORE LOOP UNROLLING\n"));
		DEBUG_COND(aflags[tblu].level&3,pr_blocks(top_down_basic));
		DEBUG_COND(aflags[tblu].level&1,
			print_trees((int)aflags[tblu].level,top_down_basic));

		if((do_redundant_loop || do_loop_unrolling) && ! found_ind_vars )
			find_ind_vars(first_loop);

		if(do_redundant_loop && hosted()) 
			redundant_loop(first_loop);
		if(do_loop_unrolling && loop_unrolling(first_loop)) {
			Boolean do_simplify = false;
			DEBUG_UNCOND(if (aflags[cf].xflag))
				{
				PUSH_TIME;
				local_const_folding();
				ACCUMULATE_TIME(Local_const_folding);
				}

			top_down_basic = basic_block(&has_loop);
			local_info(top_down_basic);
			PUSH_TIME;
			first_loop = loop_build(top_down_basic, &do_simplify);
			ACCUMULATE_TIME(Loop_build);
			DEBUG_COND(aflags[exp].level&&aflags[lu].level,
				print_exprs(aflags[exp].level));
			init_depth_first(top_down_basic);
			remove_unreachable(top_down_basic);
			DEBUG_COND(aflags[bb].level&1 && aflags[lu].level&1,
				pr_blocks(top_down_basic));
			DEBUG_COND(aflags[lu].level&32,
				print_trees(3,top_down_basic));

		}
		ACCUMULATE_TIME(Loop_unrolling);
		}
	}

#ifndef NODBG
	else
		DEBUG_COND(aflags[bb].level&1,pr_blocks(top_down_basic));
#endif

	DEBUG_UNCOND(if (aflags[cp].xflag))
	{
	DEBUG(aflags[tbcp].level&4,("BLOCKS BEFORE CP\n"));
	DEBUG_COND(aflags[tbcp].level&4,pr_blocks(top_down_basic));
	DEBUG_COND(aflags[tbcp].level&1,
			print_trees((int)aflags[tbcp].level,top_down_basic));

	PUSH_TIME;
	copy_prop(top_down_basic);
	ACCUMULATE_TIME(Copy_prop);

	DEBUG(aflags[tbcp].level&4,("BLOCKS AFTER CP\n"));
	DEBUG_COND(aflags[tbcp].level&4,pr_blocks(top_down_basic));
	DEBUG_COND(aflags[tbcp].level&4,
			print_trees((int)aflags[tbcp].level,top_down_basic));
	}

		/* Print exprs with level 1 detail */
	DEBUG_COND(aflags[exp].level&2,print_exprs(1));

	dead_stores = remap_object_ids(~0);
#ifndef NODBG
		/* Print exprs with level 1 detail */
	DEBUG_COND(aflags[ds].level&2,print_exprs(aflags[ds].level));
	if (!aflags[ds].xflag)
		dead_stores = 0;
#endif
	if (dead_stores) {
		reg_arena = arena_init();
		OBJECT_SET_SIZE = dead_stores;
		get_use_def(reg_arena, VALID_CANDIDATE);
		live_on_entry(OBJECT_SET_SIZE, reg_arena, VALID_CANDIDATE);

		PUSH_TIME;
		dead_store();
		ACCUMULATE_TIME(Dead_store);
		arena_term(reg_arena);
	}
	DEBUG_UNCOND(if (aflags[md].xflag))
	{
	mdopt();
	}

#ifndef NO_CSE
	DEBUG_UNCOND(if (aflags[cse].xflag))
	{
	PUSH_TIME;
	global_cse(top_down_basic);
	ACCUMULATE_TIME(Local_cse);
	}
#endif
	DEBUG_UNCOND(if (aflags[tff].xflag))
	{
	count_calls_in_blocks(top_down_basic);
	}

#ifndef NODBG
	if(aflags[ra].xflag) {
#endif
	regalloc = remap_object_ids(CAN_BE_IN_REGISTER);
	if (regalloc) {
		reg_arena = arena_init();
		OBJECT_SET_SIZE = regalloc;
		get_use_def(reg_arena, CAN_BE_IN_REGISTER);
		live_on_entry(OBJECT_SET_SIZE, reg_arena, CAN_BE_IN_REGISTER);
		fixup_live_on_entry();
		
		PUSH_TIME;
		reg_alloc(reg_arena,top_down_basic);
		ACCUMULATE_TIME(Reg_alloc);
#ifndef NODBG
		reg_alloc_ran = 1;
#endif
		arena_term(reg_arena);
	}
#ifndef NODBG
	}
#endif

	mark_cbranches_for_reversal();
	post_amigo_rewrite();

	DEBUG_COND(aflags[exp].level&8,print_exprs(8));

	ACCUMULATE_TIME(AMIGO);
	
	DEBUG_COND(aflags[ta].level&1, print_trees((int)aflags[ta].level,top_down_basic));

	DEBUG(aflags[fs].level&~1,
		("after opt: blocks=%d exprs=%d occurs=%d objects=%d\n",
		get_block_count(), get_expr_count(), get_occur_count(),
		get_object_count()));

	DEBUG(aflags[cst].level&2,("costing calls: %d\n", costing_call_count));
	DEBUG(aflags[cst].level&2,("nodes costed: %d\n", nodes_costed_count));

	DEBUG(aflags[funcs].level&1 || aflags[opts].level&1, 
		("func=%s first_opt=%d last_opt=%d\n",
		get_function_name(), save_opt+1, opt_curr) );
	return 1;
}

void
amigo_fatal(msg,file,line)
char *msg, *file;
int line;
{
	pfmt(stderr,MM_ERROR,":585:%s at file: %s line: %d\n", msg,file,line);
	fflush(stderr);
	abort();
}

#ifndef NODBG

#include <sys/types.h>
#include <sys/times.h>

static int
get_clock()
{
	struct tms tbuf;
	times(&tbuf);
	return tbuf.tms_utime;
}

#define TIME_MAX 10
static int time_stack[TIME_MAX];
static int *stack_top = time_stack;

void
push_time()
{
	*stack_top = get_clock();
	++stack_top;
	if (stack_top >= time_stack+TIME_MAX)
		cerror("time stack overflow");
}


int
pop_time() {
	--stack_top;
	if (stack_top < time_stack)
		cerror("time stack underflow");

	return (get_clock() - *stack_top);
}
	
int cumulative_times[Max_stats];

static char * timings_strngs[] = { /* indexed by enum timings in debug.h */
	"AMIGO",
	"Aux1",
	"Aux2",
	"Copy_prop",
	"Dead_store",
	"Hash_exprs",
	"Init_depth_first",
	"Jumps_to_jumps",
	"Local_const_folding",
	"Local_cse",
	"Local_info",
	"Loop_build",
	"Loop_invariant_code_motion",
	"Loop_simplification",
	"Loop_unrolling",
	"Reg_alloc",
	"Remove_unreachable",
	"Strength_reduction",
	"Top_down_basic",
	"Vect",
	"Max_stats"
};

void
amigo_time_print()
{ 
	int i;
	if(aflags[tm].level & 1) {
		if (stack_top != time_stack)
			cerror("time stack not empty");
		for(i = 0; i < Max_stats; i++) fprintf(stderr, "Time %s: %d\n",
			timings_strngs[i], cumulative_times[i]);
	}
}
#endif
