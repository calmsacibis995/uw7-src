#ident	"@(#)fur:i386/cmd/fur/inline.c	1.8"
#ident	"$Header:"

#include <fcntl.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#ifdef __STDC__
#include <stdarg.h>
#include <stdlib.h>
#else
#include <varargs.h>
#include <malloc.h>
#endif
#include "fur.h"
#include <macros.h>
#include <sys/mman.h>
#define _KERNEL
#include <sys/bitmasks.h>
#undef _KERNEL
#include "op.h"

void inline_expand(int func);
int analyze_ea(int block);
void substitute_ea(int block);
int analyze_esr(int block);
void gen_inlined_code(int block);

static void
make_map(int func, int len)
{
	int i;

	if (Func_info[func].map) {
		if (len == Func_info[func].clen)
			return;
		Func_info[func].map = REALLOC(Func_info[func].map, len * sizeof(int));
		return;
	}
	else
		Func_info[func].map = MALLOC(len * sizeof(int));
	for (i = Func_info[func].start_block; i < Func_info[func].end_block; i++)
		if (ORDER(i) != NO_SUCH_ADDR)
			Func_info[func].map[ORDER(i)] = i;
}

#ifndef PARTIAL_INLINING
static int
check_jump_table(int inserted_func)
{
	int i;

	for (i = Func_info[inserted_func].start_block; i < Func_info[inserted_func].end_block; i++)
		if (END_TYPE(i) == END_IJUMP)
			return(1);
	return(0);
}
#else
static int
check_pic_jump_table(int inserted_func)
{
	int i;

	for (i = 0; i < NJumpTables; i++) {
		if (JumpTables[i].base_address == NO_SUCH_ADDR)
			continue;
		if ((START_ADDR(Func_info[inserted_func].start_block) < JumpTables[i].base_address) && (JumpTables[i].base_address <= END_ADDR(Func_info[inserted_func].end_block)))
			return(1);
	}
	return(0);
}
#endif /* ifndef PARTIAL_INLINING */

inline_blocks(int block, int mark)
{
	int i, j;
	int callsite = JUMP_TARGET(block);
	int inserted_func = FUNCNO(callsite);
	int enclosing_func = FUNCNO(block);

	make_map(inserted_func, Func_info[inserted_func].clen);
	make_map(enclosing_func, Func_info[enclosing_func].clen);

#ifndef PARTIAL_INLINING
	if (check_jump_table(inserted_func)) {
		if (VERBOSE1)
			printf("Warning: function %s%s%s which has a jump table will not be inlined\n", FULLNAME(Funcs[inserted_func]));
		return(0);
	}
#else
	/* Note that this is done when generating the ordering file AND
	* when using the ordering file, because we don't want to be wrong
	* about this.
	*/
	if (check_pic_jump_table(inserted_func)) {
		if (VERBOSE1)
			printf("Warning: function %s%s%s which has a PIC jump table will not be inlined\n", FULLNAME(Funcs[inserted_func]));
		return(0);
	}
#endif
	if (mark) {
		for (i = 0; i < (int) Func_info[inserted_func].clen; i++)
			DEL_FLAGS(Func_info[inserted_func].map[i], BEENHERE);
		for (i = 0; i < (int) Func_info[enclosing_func].clen; i++)
			DEL_FLAGS(Func_info[enclosing_func].map[i], BEENHERE);
	}
	for (i = ORDER(callsite); i < (int) Func_info[inserted_func].clen; i++) {
		j = Func_info[inserted_func].map[i];
		if ((j != Func_info[inserted_func].start_block) && (FLAGS(j) & ENTRY_POINT)) { 
			if (Debug)
				fprintf(stderr, "Will not inline %s%s%s (into %s%s%s) which has multiple entry points\n", FULLNAME(Funcs[inserted_func]), FULLNAME(Funcs[enclosing_func]));
			return(0);
		}
#ifdef PARTIAL_INLINING
		if (FLAGS(j) & LOW_USAGE)
			break;
#endif

		if (mark)
			ADD_FLAGS(j, BEENHERE);
#ifdef PARTIAL_INLINING
		if (NEW_END_TYPE(j) == END_IJUMP) {
			i++;
			break;
		}
#endif
	}
	return(i - ORDER(callsite));
}

should_i_inline(int block)
{
	int new_blocks;
	int callsite = JUMP_TARGET(block);
	int i, j, size;
	int order_end;
	int inserted_func = FUNCNO(callsite);
	int enclosing_func = FUNCNO(block);
	int maxsize;

	new_blocks = inline_blocks(block, 0);
	if (!new_blocks)
		return(0);
	order_end = new_blocks + ORDER(callsite);
	for (i = ORDER(callsite), size = 0; i < order_end; i++) {
		int new_end_instlen;

		j = Func_info[inserted_func].map[i];
		size += END_ADDR(j) - START_ADDR(j);
		switch (END_TYPE(j)) {
		case END_IJUMP:
			size += 1000;
			break;
		case END_RET:
			size--;
			break;
		case END_JUMP:
			if ((JUMP_TARGET(j) != NO_SUCH_ADDR) && (ORDER(JUMP_TARGET(j)) < order_end))
				new_end_instlen = 2;
			else
				new_end_instlen = 4;
			size += new_end_instlen - END_INSTLEN(j);
			break;
		case END_CJUMP:
			if ((JUMP_TARGET(j) != NO_SUCH_ADDR) && (ORDER(JUMP_TARGET(j)) < order_end))
				new_end_instlen = 2;
			else
				new_end_instlen = 5;
			size += new_end_instlen - END_INSTLEN(j);
			break;
		}
	}
	if (getenv("MAXSIZE"))
		maxsize = atoi(getenv("MAXSIZE"));
	else
		maxsize = 100000;
	if (VERBOSE1)
		printf("%sall from %s%s%s to %s%s%s at 0x%x: Calls = %d, Called = %d, Size = %d\n",
			(size >= maxsize) ? "Not inlining c" : "C",
			FULLNAME(Funcs[FUNCNO(block)]),
			FULLNAME(Funcs[FUNCNO(callsite)]),
			END_ADDR(block) - END_INSTLEN(block), FREQ(block), Func_info[FUNCNO(callsite)].ncalls, size);
	return (size < maxsize);
/*	if (Func_info[FUNCNO(callsite)].ncalls > FREQ(block)) {*/
}

matchblock(int block, char *str)
{
	char buf[512];
	char *p;
	int blocks_from_start;

	strcpy(buf, str);
	p = strchr(buf, '+');
	*p = '\0';
	blocks_from_start = atoi(p + 1);
	if (block - Func_info[FUNCNO(block)].start_block != blocks_from_start)
		return(0);
	if (p = strchr(buf, '@')) {
		if (!FNAME(Funcs[FUNCNO(block)]) || (FNAME(Funcs[FUNCNO(block)])[0] == '\0'))
			return(0);
		*p = '\0';
		if (strcmp(FNAME(Funcs[FUNCNO(block)]), buf) != 0)
			return(0);
		p++;
	}
	else
		p = buf;
	if (strcmp(NAME(Funcs[FUNCNO(block)]), p) != 0)
		return(0);
	return(1);
}

int
spec_seamy(int block)
{
	char buf[512];
	char *env, *p;

	if (!(env = getenv("SPEC_SEAMY")))
		return(0);
	strcpy(buf, env);
	for (p = strtok(buf, " "); p; p = strtok(NULL, " "))
		if (matchblock(block, p))
			return(1);
	return(0);
}

inline_func(int block)
{
	int pop_spot;
	int push_spot;
	int i, j;
	int newclen, oldclen;
	int new_nblocks;
	int new_blocks;
	int callsite = JUMP_TARGET(block);
	int inserted_func = FUNCNO(callsite);
	int enclosing_func = FUNCNO(block);
	int elim;

	inline_expand(inserted_func);
	new_blocks = inline_blocks(block, 1);
	if (!new_blocks)
		return(0);
	if (VERBOSE1)
		printf("\n\nInlining %s%s%s into %s%s%s\n", FULLNAME(Funcs[inserted_func]), FULLNAME(Funcs[enclosing_func]));
	new_nblocks = Nblocks + new_blocks + 2;
	if (new_nblocks >= Sblocks) {
		int growth;

		Sblocks += 1 + Sblocks / 5;
		if (new_nblocks > Sblocks)
			Sblocks = new_nblocks + 1;
		growth = Sblocks - Nblocks - 1;
		Blocks_std = (struct block_std *) REALLOC(Blocks_std, Sblocks * sizeof(struct block_std));
		memset(Blocks_std + Nblocks + 1, '\0', growth * sizeof(struct block_std));
		Blocks_change = (struct block_change *) REALLOC(Blocks_change, Sblocks * sizeof(struct block_change));
		memset(Blocks_change + Nblocks + 1, '\0', growth * sizeof(struct block_change));
		if (Blocks_stats) {
			Blocks_stats = (struct block_stats *) REALLOC(Blocks_stats, Sblocks * sizeof(struct block_stats));
			memset(Blocks_stats + Nblocks + 1, '\0', growth * sizeof(struct block_stats));
		}
		Blocks_gen = (struct block_gen *) REALLOC(Blocks_gen, Sblocks * sizeof(struct block_gen));
		memset(Blocks_gen + Nblocks + 1, '\0', growth * sizeof(struct block_gen));
	}
	if (!Blocks_gen)
		Blocks_gen = (struct block_gen *) CALLOC(Sblocks, sizeof(struct block_gen));
	Blocks_std[new_nblocks] = Blocks_std[Nblocks];
	Blocks_change[new_nblocks] = Blocks_change[Nblocks];
	newclen = Func_info[enclosing_func].clen + new_blocks + 2;
	oldclen = Func_info[enclosing_func].clen;
	make_map(enclosing_func, newclen);
	for (j = Func_info[enclosing_func].clen - 1; j > ORDER(block); j--) {
		int mapped_block;

		mapped_block = Func_info[enclosing_func].map[j];
		ADDTO_ORDER(mapped_block, new_blocks + 2);
		Func_info[enclosing_func].map[ORDER(mapped_block)] = mapped_block;
	}

	/* Point FALLTHROUGH and set BEENHERE.  Let the later loop fix the
	** jump to point to the right copy of the code.
	*/
	push_spot = Nblocks++;
	pop_spot = push_spot + new_blocks + 1;
	SET_FALLTHROUGH(push_spot, JUMP_TARGET(block));
	SET_END_TYPE(push_spot, N_END_TYPES);
	SET_NEW_END_TYPE(push_spot, END_PUSH);
	SET_FUNCNO(push_spot, enclosing_func);
	SET_NEW_END_INSTLEN(push_spot, 5);
	SET_FLAGS(push_spot, BEENHERE);
	SET_JUMP_TARGET(push_spot, FALLTHROUGH(block));
	SET_ORDER(push_spot, ORDER(block) + 1);
	SET_END_ADDR(push_spot, START_ADDR(push_spot));
	SET_END_INSTLEN(push_spot, 0);
	Func_info[enclosing_func].map[ORDER(push_spot)] = push_spot;
	if (Blocks_stats) {
		SET_FREQ(push_spot, FREQ(block));
		SET_FFREQ(push_spot, FREQ(block));
	}
	SET_JUMP_TARGET(block, push_spot);
	SET_NEW_END_TYPE(block, END_JUMP);
	Func_info[enclosing_func].clen = newclen;
	SET_NEW_END_INSTLEN(block, 2);
	for (i = ORDER(callsite); i < new_blocks + ORDER(callsite); i++) {
		j = Func_info[inserted_func].map[i];
		Blocks_std[Nblocks] = Blocks_std[j];
		Blocks_change[Nblocks] = Blocks_change[j];
		if (FLAGS(j) & GEN_TEXT) {
			Blocks_gen[Nblocks] = Blocks_gen[j];
			SET_GENBUF(Nblocks, MALLOC(2 * SGENBUF(Nblocks)));
			memcpy(GENBUF(Nblocks), GENBUF(j), SGENBUF(Nblocks));
			SET_SGENBUF(Nblocks, 2 * SGENBUF(j));
			if (SGENREL(Nblocks)) {
				SET_GENREL(Nblocks, MALLOC(SGENREL(Nblocks) * sizeof(Elf32_Rel)));
				memcpy(GENREL(Nblocks), GENREL(j), SGENREL(Nblocks) * sizeof(Elf32_Rel));
			}
		}
		SET_FUNCNO(Nblocks, FUNCNO(block));
		if (Blocks_stats)
			Blocks_stats[Nblocks] = Blocks_stats[j];
		DEL_FLAGS(Nblocks, ENTRY_POINT|FUNCTION_START);
		if (NEW_END_TYPE(j) == END_RET) {
			SET_NEW_END_TYPE(Nblocks, END_JUMP);
			SET_NEW_END_INSTLEN(Nblocks, 2);
			SET_JUMP_TARGET(Nblocks, pop_spot);
		}
		SET_ORDER(Nblocks, ORDER(block) + 2 + ORDER(j) - ORDER(callsite));
		Func_info[enclosing_func].map[ORDER(Nblocks)] = Nblocks;
		Nblocks++;
	}
	if (Blocks_stats) {
		SET_FREQ(Nblocks, FREQ(block));
		SET_FFREQ(Nblocks, FREQ(block));
	}
	SET_END_ADDR(pop_spot, START_ADDR(pop_spot));
	SET_END_INSTLEN(pop_spot, 0);
	SET_END_TYPE(Nblocks, N_END_TYPES);
	SET_NEW_END_TYPE(Nblocks, END_POP);
	SET_FUNCNO(Nblocks, enclosing_func);
	SET_NEW_END_INSTLEN(Nblocks, 3);
	SET_JUMP_TARGET(Nblocks, NO_SUCH_ADDR);
	SET_FALLTHROUGH(Nblocks, FALLTHROUGH(block));
	SET_ORDER(Nblocks, ORDER(block) + new_blocks + 2);
	Func_info[enclosing_func].map[ORDER(Nblocks)] = Nblocks;
	Nblocks++;

	/* This is a tricky part here.  We have to fix the jumps in the
	* inserted code to point to the right copy of the code.  So, look
	* at each block that was just inserted (they will be marked
	* BEENHERE) and look at where it jumps.  If it jumps to a block
	* marked BEENHERE, this means that it must be adjusted to jump to
	* the new placement of the block
	*/
	for (i = 0; i < Func_info[enclosing_func].clen; i++) {
		j = Func_info[enclosing_func].map[i];
		if ((j > Nblocks) || (j < 0))
			fprintf(stderr, "Internal error, exiting\n");
		if (!(FLAGS(j) & BEENHERE))
			continue;
		if ((JUMP_TARGET(j) != NO_SUCH_ADDR) && (FLAGS(JUMP_TARGET(j)) & BEENHERE) && (FUNCNO(JUMP_TARGET(j)) == inserted_func) && (NEW_END_TYPE(j) != END_CALL))
			SET_JUMP_TARGET(j, Func_info[enclosing_func].map[ORDER(block) + 2 + ORDER(JUMP_TARGET(j)) - ORDER(callsite)]);
		if ((FLAGS(FALLTHROUGH(j)) & BEENHERE) && (FUNCNO(FALLTHROUGH(j)) == inserted_func))
			SET_FALLTHROUGH(j, Func_info[enclosing_func].map[ORDER(block) + 2 + ORDER(FALLTHROUGH(j)) - ORDER(callsite)]);
		if (ORDER(j) != i)
			fprintf(stderr, "Internal error, exiting\n");
	}
/*	if (!getenv("SEAMY") && new_func_to_inline(enclosing_func, inserted_func, block))*/
	if (!getenv("SEAMY")) {
		if (!spec_seamy(block)) {
			if (new_func_to_inline(enclosing_func, inserted_func, block))
				elim = analyze_ea(block);
			else
				elim = 0;
		}
		else {
			printf("Not inlining at 0x%x\n", END_ADDR(block) - END_INSTLEN(block));
			elim = 0;
		}
	}
	else
		elim = 0;
	if (VERBOSE1)
		printf("elim = %d\n", elim);
	if (elim) {
		substitute_ea(block);
		if (!getenv("ONLY_EA"))
			if (analyze_esr(block))
				substitute_esr(block);
		SET_NEW_END_TYPE(push_spot, END_FALL_THROUGH);
		SET_NEW_END_INSTLEN(push_spot, 0);

		SET_NEW_END_TYPE(pop_spot, END_FALL_THROUGH);
		SET_NEW_END_INSTLEN(pop_spot, 0);
		gen_inlined_code(block);
	}
	else if (getenv("NEVER_SEAMY")) {
		SET_NEW_END_TYPE(block, END_CALL);
		SET_JUMP_TARGET(block, callsite);
		SET_NEW_END_INSTLEN(block, 5);
		for (j = ORDER(pop_spot) + 1; j < newclen; j++) {
			int mapped_block;

			mapped_block = Func_info[enclosing_func].map[j];
			ADDTO_ORDER(mapped_block, -(new_blocks + 2));
			Func_info[enclosing_func].map[ORDER(mapped_block)] = mapped_block;
		}
		Func_info[enclosing_func].clen = oldclen;
	}
	if (getenv("DEBUG_SPACE")) {
		Special_align1 = push_spot;
		Special_align2 = FALLTHROUGH(pop_spot);
	}
	return(elim);
}

void
inline_expand(int func)
{
	int i;

	if (Func_info[func].flags & FUNC_EXPANDED)
		return;
	Func_info[func].flags |= FUNC_EXPANDED;
	for (i = Func_info[func].start_block; i < Func_info[func].end_block; i++) {
		if (!(FLAGS(i) & INLINE_CALL))
			continue;
		if (JUMP_TARGET(i) == NO_SUCH_ADDR) {
			if (!Existwarnings)
				fprintf(stderr, "Cannot inline call at address 0x%x - function not available\n", END_ADDR(i) - END_INSTLEN(i));
			continue;
		}
		if (!inline_func(i))
			DEL_FLAGS(i, INLINE_CALL);
	}
}

void
inline_proc()
{
	int func;
	int i;

	Only_esr = !!getenv("ONLY_ESR");
	find_block_reloc();
	for (i = 0; i < Nfunc_order; i++) {
		func = Func_order[i];
		inline_expand(func);
	}
}

void
gen_inlined_code(int block)
{
	int i;

	for (i = 0; i < Nchanged_blocks; i++) {
		DEL_FLAGS(Changed_blocks[i], GEN_THIS_BLOCK);
		proc_block_gentext(Changed_blocks[i]);
	}
}

esp_changed(struct instable *dp, int arg)
{
	return (Varinfo[ESP].alias < Before_stack);
}

no_changes_left()
{
	return (!Ndelete && (Varinfo[ESP].alias < Before_stack));
}

static int Funcstart_stack;

int
analyze_ea(int block)
{
	static int count = 0;
	int i;
	int nstack;
	int savings;
	int nargs;
	int dont_elim = 100000;

	if (setjmp(Giveup))
		return(0);
	Targblock = FALLTHROUGH(JUMP_TARGET(block));
	Retblock = JUMP_TARGET(JUMP_TARGET(block));
	STACK_RESET();
/*	GROWSTACK(100);*/
	if (VERBOSE1)
		printf("========== Sink assignment stage =========\n");
	set_traversal_params(0, NULL, NULL, NULL, NULL);
	sink_safe_assignments(Targblock);
	STACK_RESET();
	GROWSTACK(100);
	makealias(EBP, PSEUDO_REG_EARLY_STACK, 0);
	if (VERBOSE1)
		printf("========== Number of args stage =========\n");
	/* First, let's see how many arguments we are looking at */
	Before_stack = Varinfo[ESP].alias;
	set_traversal_params(JUST_WATCHING_REGS, proc_block_analyze_ea, esp_changed, NULL, NULL);
	if (NEW_END_TYPE(Retblock) == END_CALL)
		traverse(Retblock);
	else
		proc_block_analyze_ea(Retblock);

	nstack = Before_stack - Varinfo[ESP].alias + 1;

	if (VERBOSE2)
		printf("Stack is %d long\n", nstack);

	/* Now, let's go through the calling block to see what the
	** arguments look like
	*/
	if (VERBOSE1)
		printf("\n\n========== Analyze args stage =========\n");
	clearregs();
	STACK_RESET();
	makealias(EBP, PSEUDO_REG_EARLY_STACK, 0);
	growstack(30 - NBASE);
	Before_stack = Varinfo[ESP].alias;
	set_traversal_params(MARK_READ_ONLY_AFTER_WRITTEN, proc_block_analyze_ea, NULL, NULL, NULL);
	clear_func_saved_vars(NULL, FUNCNO(Targblock));
	clear_func_saved_vars(NULL, FUNCNO(Retblock));
	analyze_args(block);
	MARK_UNKNOWN_TO_CALLEE(ESI);
	MARK_UNKNOWN_TO_CALLEE(EDI);
	MARK_UNKNOWN_TO_CALLEE(EBP);
	MARK_UNKNOWN_TO_CALLEE(EBX);
	Varinfo[Varinfo[ESP].alias].flags |= BARRIER;

	nargs = min(nstack, Varinfo[ESP].alias - Before_stack);
	Funcstart_stack = Last_arg = Varinfo[ESP].alias;

	if (VERBOSE2)
		printf("Stack is %d long\n", nstack);

	Before_stack = Varinfo[ESP].alias;

	/* You cannot reference past the return address */
	Varinfo[Varinfo[ESP].alias].flags |= BARRIER;

	for (i = Before_stack; i > NBASE; i--) {
		if (!READ_VAR(i))
			MAKE_WATCHED_FOR_READ(i);
		MAKE_WATCHED_FOR_WRITE(i);
		UNMARK_WRITE(i);
	}

	/* Now, let's go through the function and see if we have any
	* references to any of our watched variables
	*/
	if (VERBOSE1)
		printf("\n\n========== Analyze function stage =========\n");
	set_traversal_params(SEE_ARGS|DO_SUBST|FOLLOW_CALLS|SHOW_SUBST|VISIT_UNTIL_DONE|VISIT_ON_PATHS|INTERPROCEDURAL_JUMPS, proc_block_analyze_ea, NULL, NULL, NULL);
	traverse(Targblock);
	if (Varinfo[Before_stack].flags & (SAW_WRITE|SAW_READ)) {
		printf("Giving up, the return address has been read or written\n");
		return(0);
	}
	clearregs();
	savings = 0;
	if (Only_esr)
		Last_arg = Before_stack - 1;
	else if (getenv("DONT_ELIM"))
		dont_elim = atoi(getenv("DONT_ELIM"));
	if (WAS_WATCHED_READ(Before_stack)) {
		if (VERBOSE1)
			printf("Return address read\n");
		return(0);
	}
	for (i = Before_stack; i >= Last_arg; i--) {
		if (IS_WATCHED_FOR_READ(i)) {
			MAKE_NOT_WATCHED(i);
			if (!WAS_WATCHED_READ(i)) {
				if (count++ > dont_elim)
					break;
				MARK_VALUE_UNNEEDED(i);
				if (!WAS_WATCHED_WRITTEN(i) && (i > Before_stack - nargs))
					MARK_LOCATION_UNNEEDED(i);
				savings++;
				if (VERBOSE2)
					printf("Watched variable survived: %s\n", PR(i));
			}
			else
				if (VERBOSE2)
					printf("Watched variable did not survive: %s\n", PR(i));
		}
	}
	if (!getenv("NO_FLOAT")) {
		if (VERBOSE1)
			printf("========== Float assignment stage =========\n");
		float_safe_assignments(Targblock);
	}
	return(savings);
}

void
substitute_ea(int block)
{
	int i;

	Ndelete = 0;
	clear_funcs_beenhere();
	if (VERBOSE1)
		printf("\n\n========== Substitution stage =========\n");
	Targblock = FALLTHROUGH(JUMP_TARGET(block));
	for (i = 0; i < NBASE; i++)
		clearvar(i);
	STACK_RESET();
	makealias(EBP, PSEUDO_REG_EARLY_STACK, 0);
	growstack(30 - NBASE);
	if (VERBOSE1)
		printf("********* Analyzing args *************\n");
	set_traversal_params(0, proc_block_substitution, arg_delete_or_modify_instruction, NULL, NULL);
	analyze_args(block);
	MARK_UNKNOWN_TO_CALLEE(ESI);
	MARK_UNKNOWN_TO_CALLEE(EDI);
	MARK_UNKNOWN_TO_CALLEE(EBP);
	MARK_UNKNOWN_TO_CALLEE(EBX);
	Varinfo[Varinfo[ESP].alias].flags |= BARRIER;

	if (VERBOSE1)
		printf("********* Updating the function *************\n");
	set_traversal_params(FOLLOW_RET, proc_block_substitution, delete_or_modify_instruction, no_changes_left, NULL);
	traverse(Targblock);
}

static int Nelimed;

check_for_live()
{
	int i;

	for (i = 0; i < NREGS; i++)
		if (IS_WATCHED_FOR_READ_ON_PATH(i)) {
			if (VERBOSE2)
				printf("%s has not been read or written\n", PR(i));
			UNMARK_ELIM(i);
		}
}

test_restore_needed(struct instable *dp, int arg)
{
	int i, ret = 1;

	for (i = 0; i < NREGS; i++) {
		if (IS_WATCHED_FOR_READ_ON_PATH(i)) {
			if (READ_VAR(i)) {
				if (VERBOSE2)
					printf("%s has been read\n", PR(i));
				MAKE_NOT_WATCHED_ON_PATH(i);
				UNMARK_ELIM(i);
			}
			else if (WROTE_VAR(i)) {
				if (VERBOSE2)
					printf("%s has been written\n", PR(i));
				MAKE_NOT_WATCHED_ON_PATH(i);
			}
			else
				ret = 0;
		}
	}
	return(ret);
}

end_of_path_esr()
{
	int reg;

	for (reg = 0; reg < NREGS; reg++)
		if (Esr_watch[reg].pushvar >= 0) {
			if (IS_WATCHED_FOR_READ_ON_PATH(Esr_watch[reg].pushvar)) {
				if (VERBOSE3)
					printf("Never found pop of %s\n", PR(reg));
				Esr_watch[reg].flags &= ~(ESR_ELIM|ESR_POPPED);
				Esr_watch[reg].nelim = 0;
				Esr_watch[reg].pushvar = -1;
			}
			else if (READ_VAR(reg)) {
				if (VERBOSE3)
					printf("Read after pop of %s\n", PR(reg));
				Esr_watch[reg].pushvar = -1;
				Esr_watch[reg].flags &= ~(ESR_ELIM|ESR_POPPED);
				Esr_watch[reg].nelim = 0;
			}
		}
	return(0);
}

int
analyze_esr(int block)
{
	int i, count;
	extern int Nsubsts;
	static int nesr_elim;

	Nsubsts = 0;
	clear_watch();
	clear_funcs_beenhere();
	if (setjmp(Giveup))
		return(0);
	clearregs();
	Targblock = FALLTHROUGH(JUMP_TARGET(block));
	Retblock = JUMP_TARGET(JUMP_TARGET(block));
	clear_func_saved_vars(NULL, FUNCNO(Targblock));
	clear_func_saved_vars(NULL, FUNCNO(Retblock));
	Nelimed = 4;
	STACK_RESET();
	makealias(EBP, PSEUDO_REG_EARLY_STACK, 0);
	growstack(30 - NBASE);
	MARK_ELIM(ESI);
	MAKE_WATCHED_FOR_READ_ON_PATH(ESI);
	MARK_ELIM(EDI);
	MAKE_WATCHED_FOR_READ_ON_PATH(EDI);
	MARK_ELIM(EBP);
	MAKE_WATCHED_FOR_READ_ON_PATH(EBP);
	MARK_ELIM(EBX);
	MAKE_WATCHED_FOR_READ_ON_PATH(EBX);
	if (VERBOSE2)
		printf("\n\n========== Looking for unused save/restore regs =========\n");
	STACK_RESET();
/*	pushearlys(100);*/
	set_traversal_params(JUST_WATCHING_REGS|VISIT_ON_PATHS|DO_NOT_GIVE_UP_JUST_END|NEVER_FOLLOW_RET, proc_block_analyze_ea, test_restore_needed, NULL, check_for_live);
	traverse(Retblock);
	for (i = 0; i < NREGS; i++) {
		if (IS_ELIM(i)) {
			Esr_watch[i].flags = ESR_ELIM;
			Esr_watch[i].nelim = 0;
			Esr_watch[i].pushvar = -1;
			Freeregs[Nfreeregs++] = i;
			if (VERBOSE2)
				printf("%s made it through\n", PR(i));
		}
		else {
			Esr_watch[i].nelim = 0;
			Esr_watch[i].flags = 0;
			Esr_watch[i].pushvar = -1;
		}
		UNMARK_ELIM(i);
		UNMARK_READ_WROTE(i);
		MAKE_NOT_WATCHED_ON_PATH(i);
	}
	if (VERBOSE2)
		printf("\n\n========== Looking for reads of save/restore regs =========\n");
	clearregs();
	STACK_RESET();
	growstack(Funcstart_stack - Varinfo[ESP].alias);
	Before_stack = Varinfo[ESP].alias;
	Varinfo[Before_stack].flags |= BARRIER;
	clear_func_saved_vars(NULL, FUNCNO(Targblock));
	clear_func_saved_vars(NULL, FUNCNO(Retblock));
	set_traversal_params(VISIT_UNTIL_DONE|VISIT_ON_PATHS, proc_block_analyze_esr, NULL, NULL, end_of_path_esr);
	traverse(Targblock);
	for (count = i = 0; i < NREGS; i++)
		if (Esr_watch[i].nelim) {
			extern int Dont_esr;
			int j;
			int reset_elim = Esr_watch[i].nelim;

			if (nesr_elim++ >= Dont_esr) {
				Esr_watch[i].nelim = 0;
				printf("Will not eliminate pushes/pops of %s\n", PR(i));
			}
			else {
				if (VERBOSE2)
					for (j = 0; j < Esr_watch[i].nelim; j++)
						printf("Will eliminate push/pop of %s at 0x%x\n", PR(i), Esr_watch[i].elim[j]);
				count += Esr_watch[i].nelim;
			}
		}
	return(count);
}

int
substitute_esr(int block)
{
	clear_funcs_beenhere();
	Targblock = FALLTHROUGH(JUMP_TARGET(block));
	if (VERBOSE2)
		printf("\n\n========== Deleting save/restore =========\n");
	clearregs();
	STACK_RESET();
	growstack(Funcstart_stack - Varinfo[ESP].alias);
	set_traversal_params(0, proc_block_substitution, esr_delete_instruction, NULL, NULL);
	traverse(Targblock);
}
