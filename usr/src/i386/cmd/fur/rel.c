#ident	"@(#)fur:i386/cmd/fur/rel.c	1.2.2.8"
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

static ulong	*Bstack;
static ulong	Nbstack;
static ulong	Sbstack;

static int Setflags;

/*
	Compiler's alignment rules:

	align functions on 16-byte boundaries align loops on 16-byte boundaries if
	it takes less than 7 bytes otherwise align on 4-byte boundaries
*/

/*
    represent predecessors with an incedence matrix (per function)
	calculate dominators from that (represent as an incedence matrix)
	calculate natural loops
*/

void
order_to_file()
{
	FILE *ofp;
	int i, j, count;
	int func;
	ulong funcstart;

	if (!(ofp = fopen(Orderfile, "w")))
		error("Cannot open %s\n", Orderfile);

	for (i = 0; i < Nparams; i++)
		if (Params[i].precedence > 1)
			fprintf(ofp, ".param %s=%d\n", Params[i].name, *(Params[i].val));
	for (i = 0; i < Nfunc_order; i++) {
		func = Func_order[i];
		funcstart = Func_info[Func_order[i]].start_block;
		if (Func_info[func].flags & FUNC_GROUPED)
			continue;
		for (count = 0, j = funcstart; j < Func_info[func].end_block; j++)
			if (!(FLAGS(j) & LOW_USAGE) && (ORDER(j) != NO_SUCH_ADDR))
				count++;
		if (!count)
			continue;
		fprintf(ofp, ".function %s%s%s %d %d%s\n", FULLNAME(Funcs[func]), Func_info[func].end_block - Func_info[func].start_block, count, (Func_info[func].flags & FUNC_DONT_ALIGN) ? " dont_align" : "");

		for (j = funcstart; j < Func_info[func].end_block; j++) {
			if (ORDER(j) == NO_SUCH_ADDR)
				continue;
			fprintf(ofp, "%s%s%s+%d %s%s%s+%d", FULLNAME(Funcs[func]), j - funcstart, FULLNAME(Funcs[func]), ORDER(j));
			if (IS_LOOP_HEADER(j))
				fprintf(ofp, " loop");
			if (FLAGS(j) & INLINE_CALL)
				fprintf(ofp, " inline");
			if (FLAGS(j) & LOW_USAGE)
				fprintf(ofp, " low_usage");
#ifdef TAKEN_FALLTHROUGH
			if (InlineCriteria && FREQ(j) && (END_TYPE(j) == END_CJUMP)) {
				if (TFREQ(j) == 0)
					fprintf(ofp, " fallthrough");
				else if (FFREQ(j) == 0)
					fprintf(ofp, " taken");
			}
#endif /* TAKEN_FALLTHROUGH */
			fputc('\n', ofp);
		}
	}
	fclose(ofp);
}

static void
func_no_layout(int func, int flags)
{
	int i;

	for (i = Func_info[func].start_block; i < Func_info[func].end_block; i++)
		DEL_FLAGS(i, (PLACED));
	if (!HAS_EFFECT(Func_info[func].start_block) && (JUMP_TARGET(Func_info[func].start_block) >= Func_info[func].start_block) && (JUMP_TARGET(Func_info[func].start_block) < Func_info[func].end_block) && (END_TYPE(JUMP_TARGET(Func_info[func].start_block) != END_PSEUDO_CALL))) {
		SET_ORDER(JUMP_TARGET(Func_info[func].start_block), 0);
		ADD_FLAGS(JUMP_TARGET(Func_info[func].start_block), PLACED|flags);
	}
	else {
		SET_ORDER(Func_info[func].start_block, 0);
		ADD_FLAGS(Func_info[func].start_block, PLACED|flags);
	}
	Func_info[func].clen = 1;
	for (i = Func_info[func].start_block + 1; i < Func_info[func].end_block; i++)
		if (!(FLAGS(i) & (PLACED|DATA_BLOCK)) && HAS_EFFECT(i)) {
			SET_ORDER(i, Func_info[func].clen++);
			ADD_FLAGS(i, flags);
		}
}

static void
func_changed(int func)
{
	int i;

	Func_info[func].flags |= FUNC_CHANGED;
	for (i = Func_info[func].start_block; i < Func_info[func].end_block; i++)
		DEL_FLAGS(i, LOW_USAGE);
	if (!No_warnings)
		fprintf(stderr, "Warning: function %s has changed\n", NAME(Funcs[func]));
	func_no_layout(func, 0);
}

static void
fill_in_rest(int func)
{
	int i;

	for (i = Func_info[func].start_block; i < Func_info[func].end_block; i++) {
		if (ORDER(i) == NO_SUCH_ADDR) {
			if (!HAS_EFFECT(i) || (FLAGS(i) & DATA_BLOCK))
				continue;
			SET_ORDER(i, Func_info[func].clen++);
			ADD_FLAGS(i, LOW_USAGE);
		}
	}
}

#define pass1 !pass2

/*
* The functions that are not marked FUNC_PLACED are "low usage"
* functions.  This means that the whole function was not called.  These
* should be placed after any of the "low usage" blocks; therefore, they
* should be placed in pass 2.  These functions are assigned the length
* by the number of blocks placed in pass 2 rather than pass 1.
*/
static void
fill_up_order_sub(int gen_order_only, int pass2)
{
	int i, func, j, block, placecount;

	for (i = 0; i < Nfunc_order; i++) {
		func = Func_order[i];
		if (pass1 && !Forcecontiguous && !(Func_info[func].flags & FUNC_PLACED))
			break;
		if (pass1 || !(Func_info[func].flags & FUNC_PLACED))
			Func_info[func].order_start = Norder;
		for (placecount = 0, j = 0, block = Func_info[func].start_block; ; j++, block++) {
			if (Func_info[func].map) {
				if (j >= (int) Func_info[func].clen)
					break;
				block = Func_info[func].map[j];
			}
			else {
				if (block >= Func_info[func].end_block)
					break;
				if (ORDER(block) == NO_SUCH_ADDR)
					continue;
			}
			if (pass1 && !Forcecontiguous && (FLAGS(block) & LOW_USAGE))
				continue;
			if (pass2 && (Func_info[func].flags & FUNC_PLACED)) {
				if (!(FLAGS(block) & LOW_USAGE))
					continue;
				Order[Norder + ORDER(block) - Func_info[func].placecount] = block;
				if (!gen_order_only)
					ADDTO_ORDER(block, Norder - Func_info[func].placecount);
			}
			else {
				Order[Norder + ORDER(block)] = block;
				if (!gen_order_only)
					ADDTO_ORDER(block, Norder);
			}
			placecount++;
		}
		Norder += placecount;
		if (pass1 || !(Func_info[func].flags & FUNC_PLACED)) {
			Func_info[func].placecount = placecount;
			if (!gen_order_only)
				Func_info[func].clen = placecount;
		}
	}
}

static void
fill_up_order(int gen_order_only)
{
	int i;

	Order = (int *) REALLOC(Order, (Nblocks + 1) * sizeof(int));
	Norder = 0;
	for (i = 0; i < Nfuncs; i++)
		Func_info[i].order_start = NO_SUCH_ADDR;

	fill_up_order_sub(gen_order_only, 0);
	if (!Forcecontiguous)
		fill_up_order_sub(gen_order_only, 1);
}

static void
order_from_file()
{
	char *new_spot, *orig_spot; 
	char *p;
	int loop_alignment, inline, low_usage;
	int fallthrough, taken;
	char *flags;
	char *new_plus, *orig_plus;
	char buf[BUFSIZ];
	char buf2[BUFSIZ];
	ulong block;
	FILE *ofp;
	int func, newfunc;
	char *ftype;

	if (!(ofp = fopen(Orderfile, "r")))
		error("Cannot open %s\n", Orderfile);

	while (fgets(buf, BUFSIZ, ofp)) {
		if (buf[0] == '#')
			continue;
		strtok(buf, "#\n");
		strcpy(buf2, buf);
		switch(buf[0]) {
		case '.':
			ftype = strtok(buf, " ");
			if (strcmp(ftype, ".function") == 0) {
				char *name = strtok(NULL, " ");
				char *len = strtok(NULL, " ");
				char *contiguous_len = strtok(NULL, " ");
				int flags = 0;

				while (p = strtok(NULL, " "))
					if (strcmp(p, "dont_align") == 0)
						flags |= FUNC_DONT_ALIGN;
				if (!name || !(len && atoi(len)) || !(contiguous_len && atoi(contiguous_len)))
					error("%s has invalid data on this line: %s\n", Orderfile, buf2);
				if ((func = find_dominant_name(name)) < 0) {
					if (Existwarnings && !No_warnings)
						fprintf(stderr, "Warning: function %s no longer exists\n", name);
					continue;
				}
				if (FUNCNO(Func_info[func].start_block) != func)
					func = FUNCNO(Func_info[func].start_block);
				if (Func_info[func].flags & FUNC_GROUPED) {
					Func_info[func].flags |= FUNC_CHANGED;
					continue;
				}
				if (Func_info[func].flags & FUNC_PLACED)
					error("ERROR: function %s occurs twice\n", name);
				Func_order[Nfunc_order] = func;
				Func_info[func].flags |= FUNC_PLACED|flags;
				Nfunc_order++;
				Func_info[func].clen = atoi(contiguous_len);
				if (atoi(len) != (Func_info[func].end_block - Func_info[func].start_block))
					func_changed(func);
			}
			else if (strcmp(ftype, ".param") == 0)
				param_set_var_val(strtok(NULL, " "), 3);
			break;
		}
	}
	rewind(ofp);

	while (fgets(buf, BUFSIZ, ofp)) {
		strtok(buf, "#\n");
		strcpy(buf2, buf);
		switch(buf[0]) {
		case '\0':
		case '#':
			continue;
		case '.':
			ftype = strtok(buf, " ");
			if (strcmp(ftype, ".function") == 0) {
				char *name = strtok(NULL, " ");

				newfunc = func = find_dominant_name(name);
				continue;
			}
			continue;
		default:
			orig_spot = strtok(buf, " ");
			new_spot = strtok(NULL, " ");
			fallthrough = taken = loop_alignment = inline = low_usage = 0;
			while (flags = strtok(NULL, " ")) {
				if (strcmp(flags, "loop") == 0)
					loop_alignment = 1;
				else if (strcmp(flags, "inline") == 0)
					inline = 1;
				else if (strcmp(flags, "low_usage") == 0)
					low_usage = 1;
				else if (strcmp(flags, "fallthrough") == 0)
					fallthrough = 1;
				else if (strcmp(flags, "taken") == 0)
					taken = 1;
			}
			if (!orig_spot || !new_spot)
				error("%s has invalid data on this line: %s\n", Orderfile, buf2);
			if (orig_spot[0] != '+') {
				orig_plus = strchr(orig_spot, '+');
				if (!orig_plus)
					error("%s has invalid data on this line: %s\n", Orderfile, buf2);
				*orig_plus++ = '\0';
				func = find_dominant_name(orig_spot);
			}
			else
				orig_plus = orig_spot + 1;
			if ((func < 0) || (func > Nfuncs))
				continue;
			if (Func_info[func].flags & FUNC_CHANGED)
				continue;
			if (!(Func_info[func].flags & FUNC_PLACED) || (atoi(orig_plus) >= Func_info[func].end_block - Func_info[func].start_block))
				error("%s has invalid data on this line: %s\n", Orderfile, buf2);
			block = Func_info[func].start_block + atoi(orig_plus);
			if (!HAS_EFFECT(block)) {
				func_changed(func);
				continue;
			}
			if ((new_spot[0] != ':') || ((strcmp(new_spot, ":lu") != 0) && (strcmp(new_spot, ":low_usage") != 0))) {
				if (FLAGS(block) & DATA_BLOCK) {
					func_changed(func);
					continue;
				}
				if (new_spot[0] != '+') {
					new_plus = strchr(new_spot, '+');
					if (new_plus)
						*new_plus++ = '\0';
					newfunc = find_dominant_name(new_spot);
				}
				else
					new_plus = new_spot + 1;
				if ((newfunc < 0) || (newfunc > Nfuncs))
					error("%s has invalid data on this line: %s\n", Orderfile, buf2);
				SET_ORDER(block, atoi(new_plus));
				if (low_usage) {
					if (Forcecontiguous)
						Func_info[newfunc].clen++;
				}
				else if (ORDER(block) >= Func_info[newfunc].clen)
					error("%s has invalid data on this line: %s\n", Orderfile, buf2);
			}
			if (loop_alignment)
				ADD_FLAGS(block, LOOP_HEADER);
			if (inline)
				ADD_FLAGS(block, INLINE_CALL);
			if (low_usage)
				ADD_FLAGS(block, LOW_USAGE);
			if (fallthrough || taken) {
				if (!Blocks_stats) {
					int i;

					Blocks_stats = (struct block_stats *) REZALLOC(Blocks_stats, Sblocks * sizeof(struct block_stats));
					for (i = 0; i < Nblocks; i++) {
						SET_FREQ(i, 2);
						SET_FFREQ(i, 1);
						SET_TFREQ(i, 1);
					}
				}
				if (fallthrough) {
					SET_FFREQ(block, 2);
					SET_TFREQ(block, 0);
				}
				else {
					SET_TFREQ(block, 2);
					SET_FFREQ(block, 0);
				}
			}
		}
	}
	fclose(ofp);
}

int
get_succs(int block, Elf32_Addr **psuccs)
{
	int i;

	for (i = 0; i < NJumpTables; i++)
		if (JumpTables[i].ijump_end_addr == END_ADDR(block)) {
			*psuccs = JumpTables[i].dsts;
			return(JumpTables[i].tablen);
		}
	return(0);
}

/*
* A block may have at most two successors (a TRUE successor and a FALSE
* successor).  Of course, only conditional jumps have both.  If we do
* not know a node's successor, we set it to -1.
*/

#define SET_SUCC(BLOCKNO) do {\
	tsucc = fsucc = -1;\
	tfreq = ffreq = -1;\
	switch (END_TYPE((BLOCKNO))) {\
	case END_JUMP:\
		fsucc = JUMP_TARGET((BLOCKNO));\
		ffreq = FREQ((BLOCKNO));\
		break;\
	case END_CJUMP:\
		fsucc = FALLTHROUGH(BLOCKNO);\
		ffreq = FFREQ((BLOCKNO));\
		tsucc = JUMP_TARGET((BLOCKNO));\
		tfreq = TFREQ((BLOCKNO));\
		break;\
	case END_PUSH:\
	case END_POP:\
	case END_CALL:\
	case END_PSEUDO_CALL:\
	case END_FALL_THROUGH:\
		fsucc = FALLTHROUGH(BLOCKNO);\
		ffreq = FREQ((BLOCKNO));\
		break;\
	}\
	if ((fsucc < funcstart) || (fsucc >= funcend))\
		fsucc = -1;\
	if ((tsucc < funcstart) || (tsucc >= funcend))\
		tsucc = -1;\
} while(0)

#define IS_PATHTO(BLOCK, TARGET) (!Irreducible && BITMASKN_TEST1(Pathtos[BLOCK - funcstart], TARGET - funcstart))

static uint_t Irreducible;
static uint_t *work;
static uint_t **Pathtos;
static char	  *Pathtobuf;
static uint_t **Pdoms;
static char	  *Pdombuf;
static uint_t **Doms;
static char	  *Dombuf;
static ulong Maxdoms;

static int
pred_proc(int funcstart, int len, int block, int pred)
{
	if ((funcstart > block) || (block >= funcstart + len))
		return(0);
	if ((funcstart > pred) || (pred >= funcstart + len))
		return(0);
	memcpy(work, Doms[block - funcstart], BITMASK_NWORDS(len) * sizeof(uint_t));
	BITMASKN_ANDN(work, Doms[pred - funcstart], BITMASK_NWORDS(len));
	BITMASKN_SET1(work, block - funcstart);
	if (memcmp(work, Doms[block - funcstart], BITMASK_NWORDS(len) * sizeof(uint_t))) {
/*		printf("%d changed due to predecessor %d\n", block, pred);*/
		memcpy(Doms[block - funcstart], work, BITMASK_NWORDS(len) * sizeof(uint_t));
		return(1);
	}
	return(0);
}

static int
pathto_proc(int funcstart, int len, int block, int succ)
{
	if ((funcstart > block) || (block >= funcstart + len))
		return(0);
	if ((funcstart > succ) || (succ >= funcstart + len))
		return(0);
	if (BITMASKN_TEST1(Doms[block - funcstart], succ - funcstart))
		return 0;
	memcpy(work, Pathtos[block - funcstart], BITMASK_NWORDS(len) * sizeof(uint_t));
	BITMASKN_SETN(work, Pathtos[succ - funcstart], BITMASK_NWORDS(len));
	if (memcmp(work, Pathtos[block - funcstart], BITMASK_NWORDS(len) * sizeof(uint_t))) {
/*		printf("%d changed due to succ %d\n", block, pred);*/
		memcpy(Pathtos[block - funcstart], work, BITMASK_NWORDS(len) * sizeof(uint_t));
		return(1);
	}
	return(0);
}

#define IS_BACKEDGE(SRC, TARG) (BITMASKN_TEST1(Doms[(SRC) - funcstart], (TARG) - funcstart))

int isnt_reducible(int cur, int funcstart, int funcend);

int
isnt_reducible_func(int funcstart, int funcend)
{
	int i, ret;

	for (i = funcstart; i < funcend; i++)
		DEL_FLAGS(i, (REDUCIBLE_OKAY|BEENHERE));
	ret = isnt_reducible(funcstart, funcstart, funcend);
	for (i = funcstart; i < funcend; i++)
		DEL_FLAGS(i, (REDUCIBLE_OKAY|BEENHERE));
	return(ret);
}

int
isnt_reducible(int cur, int funcstart, int funcend)
{
	int fsucc, tsucc;
	int ffreq, tfreq;
	int i;
	Elf32_Addr *succs;
	int nsuccs;

	if (FLAGS(cur) & REDUCIBLE_OKAY)
		return(0);
	if (FLAGS(cur) & BEENHERE) {
		if (Debug)
			printf("Cycle found %d\n", cur);
		return(1);
	}
	ADD_FLAGS(cur, BEENHERE);
	if (END_TYPE(cur) != END_IJUMP) {
		SET_SUCC(cur);
		if ((fsucc >= 0) && !IS_BACKEDGE(cur, fsucc) && isnt_reducible(fsucc, funcstart, funcend)) {
			if (Debug)
				printf("Cycle found %d\n", cur);
			DEL_FLAGS(cur, BEENHERE);
			return(1);
		}
		if ((tsucc >= 0) && !IS_BACKEDGE(cur, tsucc) && isnt_reducible(tsucc, funcstart, funcend)) {
			if (Debug)
				printf("Cycle found %d\n", cur);
			DEL_FLAGS(cur, BEENHERE);
			return(1);
		}
		DEL_FLAGS(cur, BEENHERE);
		ADD_FLAGS(cur, REDUCIBLE_OKAY);
		return(0);
	}
	if (nsuccs = get_succs(cur, &succs)) {
		for (i = 0; i < nsuccs; i++) {
			if ((succs[i] >= funcend) || (succs[i] < funcstart))
				continue;
			if ((succs[i] >= 0) && !IS_BACKEDGE(cur, succs[i]) && isnt_reducible(succs[i], funcstart, funcend)) {
				if (Debug)
					printf("Cycle found %d\n", cur);
				DEL_FLAGS(cur, BEENHERE);
				return(1);
			}
		}
	}
	DEL_FLAGS(cur, BEENHERE);
	ADD_FLAGS(cur, REDUCIBLE_OKAY);
	return(0);
}

static int
compute_probability(int funcstart, int funcend, int src, int targ, int cur, int start)
{
	int fsucc, tsucc;
	int ffreq, tfreq;
	int totprob, totfreq;
	int i;
	Elf32_Addr *succs;
	int nsuccs;

	if (cur < 0)
		return(0);
	if ((cur == src) && !start)
		return(0);
	if (cur == targ)
		return(1000);
	start = 1;
	SET_SUCC(cur);
	if (END_TYPE(cur) != END_IJUMP)
		return((tfreq * compute_probability(funcstart, funcend, src, targ, tsucc, start) + ffreq * compute_probability(funcstart, funcend, src, targ, fsucc, start)) / (tfreq + ffreq));
	if (!(nsuccs = get_succs(cur, &succs)))
		return(0);
	for (totfreq = i = 0; i < nsuccs; i++) {
		if (FREQ(succs[i]) < 0)
			continue;
		totfreq += FREQ(succs[i]);
	}
	for (totprob = i = 0; i < nsuccs; i++) {
		if (FREQ(succs[i]) < 0)
			continue;
		totprob += FREQ(succs[i]) * compute_probability(funcstart, funcend, src, targ, succs[i], start) / totfreq;
	}
	return(totprob);
}

static void
compute_pathto(int funcstart, int funcend)
{
	int i, j, changed;
	int fsucc, tsucc;
	int ffreq, tfreq;
	Elf32_Addr *succs;
	int nsuccs;
	int len = funcend - funcstart;

	for (i = 0; i < len; i++) {
		Pathtos[i] = (uint_t *) (Pathtobuf + i * BITMASK_NWORDS(len) * sizeof(uint_t));
		BITMASKN_CLRALL(Pathtos[i], BITMASK_NWORDS(len));
		BITMASKN_SET1(Pathtos[i], i);
	}
	do {
		changed = 0;
		for (i = funcstart; i < funcend; i++) {
			if (END_TYPE(i) == END_IJUMP) {
				if (!(nsuccs = get_succs(i, &succs)))
					continue;
				for (j = 0; j < nsuccs; j++)
					changed += pathto_proc(funcstart, len, i, succs[j]);
			}
			else {
				SET_SUCC(i);
				if (fsucc >= 0)
					changed += pathto_proc(funcstart, len, i, fsucc);
				if (tsucc >= 0)
					changed += pathto_proc(funcstart, len, i, tsucc);
			}
		}
	} while (changed);
	if (getenv("FLOW_GRAPH")) {
		for (i = 0; i < len; i++) {
			for (j = 0; j < len; j++)
				if (BITMASKN_TEST1(Pathtos[i], j))
					printf("There is a path from %d to %d\n", funcstart + i, funcstart + j);
		}
	}
}

/* Aho & Ullman - page 670 */
static void
compute_dominators(int funcstart, int funcend)
{
	int i, j, changed;
	int fsucc, tsucc;
	int ffreq, tfreq;
	Elf32_Addr *succs;
	int nsuccs;
	int len = funcend - funcstart;

	if (getenv("FLOW_GRAPH")) {
		for (i = funcstart; i < funcend; i++) {
			if (!(FLAGS(i) & START_NOP))
				continue;
			SET_SUCC(i);
			if ((tsucc >= 0) && (fsucc >= 0)) {
				printf("True successor of %d is %d\n", i, tsucc);
				printf("False successor of %d is %d\n", i, fsucc);
			}
			else if (fsucc < 0)
				printf("No successors of %d\n", i);
			else
				printf("Only successor of %d is %d\n", i, fsucc);
		}
	}
	if (len > Maxdoms) {
		Maxdoms = len;
		work = REALLOC(work, BITMASK_NWORDS(Maxdoms) * sizeof(uint_t));
		Dombuf = REALLOC(Dombuf, Maxdoms * BITMASK_NWORDS(Maxdoms) * sizeof(uint_t));
		Doms = REALLOC(Doms, Maxdoms * sizeof(uint_t *));
		Pdombuf = REALLOC(Pdombuf, Maxdoms * BITMASK_NWORDS(Maxdoms) * sizeof(uint_t));
		Pdoms = REALLOC(Pdoms, Maxdoms * sizeof(uint_t *));
		Pathtobuf = REALLOC(Pathtobuf, Maxdoms * BITMASK_NWORDS(Maxdoms) * sizeof(uint_t));
		Pathtos = REALLOC(Pathtos, Maxdoms * sizeof(uint_t *));
	}
	for (i = 0; i < len; i++)
		Doms[i] = (uint_t *) (Dombuf + i * BITMASK_NWORDS(len) * sizeof(uint_t));
	BITMASKN_CLRALL(Doms[0], BITMASK_NWORDS(len));
	BITMASKN_SET1(Doms[0], 0);
	for (i = 1; i < len; i++) 
		BITMASKN_SETALL(Doms[i], BITMASK_NWORDS(len));
	do {
		changed = 0;
		for (i = funcstart; i < funcend; i++) {
			if (END_TYPE(i) == END_IJUMP) {
				if (!(nsuccs = get_succs(i, &succs)))
					continue;
				for (j = 0; j < nsuccs; j++)
					changed += pred_proc(funcstart, len, succs[j], i);
			}
			else {
				SET_SUCC(i);
				if (fsucc >= 0)
					changed += pred_proc(funcstart, len, fsucc, i);
				if (tsucc >= 0)
					changed += pred_proc(funcstart, len, tsucc, i);
			}
		}
	} while (changed);
	if (getenv("FLOW_GRAPH")) {
		for (i = 0; i < len; i++) {
			for (j = 0; j < len; j++)
				if (BITMASKN_TEST1(Doms[j], i))
					printf("Block %d dominates %d\n", funcstart + i, funcstart + j);
		}
	}
}

static int
succ_proc(int funcstart, int len, int block, int succ)
{
	if ((funcstart > block) || (block >= funcstart + len))
		return(0);
	if ((funcstart > succ) || (succ >= funcstart + len))
		return(0);
	if (!BITMASKN_TEST1(Doms[block - funcstart], succ - funcstart)) {
		memcpy(work, Pdoms[block - funcstart], BITMASK_NWORDS(len) * sizeof(uint_t));
		BITMASKN_ANDN(work, Pdoms[succ - funcstart], BITMASK_NWORDS(len));
	}
	else
		BITMASKN_CLRALL(work, BITMASK_NWORDS(len));
	BITMASKN_SET1(work, block - funcstart);
	if (memcmp(work, Pdoms[block - funcstart], BITMASK_NWORDS(len) * sizeof(uint_t))) {
/*		printf("%d changed due to successor %d\n", block, succ);*/
		memcpy(Pdoms[block - funcstart], work, BITMASK_NWORDS(len) * sizeof(uint_t));
		return(1);
	}
	return(0);
}

static void
compute_postdominators(int funcstart, int funcend)
{
	int i, j, changed;
	int fsucc, tsucc;
	int ffreq, tfreq;
	Elf32_Addr *succs;
	int nsuccs;
	int len = funcend - funcstart;

	for (i = 0; i < len; i++)
		Pdoms[i] = (uint_t *) (Pdombuf + i * BITMASK_NWORDS(len) * sizeof(uint_t));
	for (i = 0; i < len; i++)
		BITMASKN_SETALL(Pdoms[i], BITMASK_NWORDS(len));

	do {
		changed = 0;
		for (i = funcstart; i < funcend; i++) {
			if (END_TYPE(i) == END_IJUMP) {
				if (!(nsuccs = get_succs(i, &succs)))
					continue;
				for (j = 0; j < nsuccs; j++)
					changed += succ_proc(funcstart, len, i, succs[j]);
			}
			else {
				SET_SUCC(i);
				if ((fsucc < 0) && (tsucc < 0)) {
					BITMASKN_CLRALL(Pdoms[i - funcstart], BITMASK_NWORDS(len));
					BITMASKN_SET1(Pdoms[i - funcstart], i - funcstart);
				}
				else {
					if (fsucc >= 0)
						changed += succ_proc(funcstart, len, i, fsucc);
					if (tsucc >= 0)
						changed += succ_proc(funcstart, len, i, tsucc);
				}
			}
		}
	} while (changed);
	if (getenv("FLOW_GRAPH")) {
		for (i = 0; i < len; i++) {
			for (j = 0; j < len; j++)
				if (BITMASKN_TEST1(Pdoms[j], i))
					printf("Block %d postdominates %d\n", funcstart + i, funcstart + j);
		}
	}
}

/*
* Search for the most-likely path to an exit from the function.  The
* starting point is block i; ending points are either return blocks,
* jumps to targets outside of the function OR reaching a node that was
* placed IN AN EARLIER TRACE.  This works recursively by choosing the
* "Best" successor of the current node as the next node to place.

* The tricky part is cycles.  If we encounter a cycle, we search the
* whole cycle for the best way out.  This is the next node placed.
* Notice that there may be many cycles that start and begin with the
* same node.  We choose to break out of the biggest one.
*/
static void
placetrace(int i, int funcstart, int funcend)
{
	int fsucc, tsucc;
	int succno;
	int ffreq, tfreq;
	Elf32_Addr *succs;
	int nsuccs;

	if (FLAGS(i) & PLACED)
		return;
	if (Nbstack == Sbstack) {
		Sbstack += 50;
		Bstack = (ulong *) REALLOC(Bstack, Sbstack * sizeof(ulong));
	}
	Bstack[Nbstack++] = i;
	if (!(FLAGS(i) & PLACED_THIS_TRACE)) {
		if (HAS_EFFECT(i)) {
			SET_ORDER(i, Norder);
			Norder++;
		}
		ADD_FLAGS(i, PLACED_THIS_TRACE|Setflags);
	}
	else {
		/* cycle in trace */
		int bestfreq = 0, bestcandidate, freq, j, k, candidate;

		for (k = Nbstack - 2; Bstack[k] != i; k--)
			;
		for (j = k + 1; j < Nbstack; j++)
			ADD_FLAGS(Bstack[j], IN_CYCLE);
		freq = 0;
		for (j = k + 1; j < Nbstack; j++) {
			if (END_TYPE(Bstack[j]) == END_IJUMP) {
				if (!(nsuccs = get_succs(Bstack[j], &succs))) {
					candidate = -1;
					freq = -1;
				}
				else {
					for (candidate = -1, succno = 0; succno < nsuccs; succno++) {
						if (FLAGS(succs[succno]) & IN_CYCLE)
							continue;
						if ((candidate != -1) || (freq < FREQ(succs[succno]))) {
							candidate = succs[succno];
							freq = FREQ(candidate);
						}
					}
				}
			}
			else {
				SET_SUCC(Bstack[j]);
				if ((fsucc >= 0) && !(FLAGS(fsucc) & IN_CYCLE)) {
					candidate = fsucc;
					freq = ffreq;
				}
				else if ((tsucc >= 0) && !(FLAGS(tsucc) & IN_CYCLE)) {
					candidate = tsucc;
					freq = tfreq;
				}
			}
			if (freq > bestfreq) {
				bestfreq = freq;
				bestcandidate = candidate;
			}
		}

		for (j = k + 1; j < Nbstack; j++)
			DEL_FLAGS(Bstack[j], IN_CYCLE);

		if (bestfreq != 0)
			placetrace(bestcandidate, funcstart, funcend);
		return;
	}
	if (END_TYPE(i) == END_IJUMP) {
		if (!(nsuccs = get_succs(i, &succs))) {
			fsucc = tsucc = -1;
			tfreq = ffreq = -1;
		}
		else {
			fsucc = tsucc = -1;
			ffreq = tfreq = -1;
			for (fsucc = -1, succno = 0; succno < nsuccs; succno++) {
				if (succs[succno] == fsucc)
					continue;
				if ((succs[succno] < funcstart) || (succs[succno] >= funcend)) {
					printf("Interprocedural jump table?\n");
					continue;
				}
				if ((fsucc < 0) || (ffreq < FREQ(succs[succno]))) {
					fsucc = succs[succno];
					ffreq = FREQ(fsucc);
				}
			}
			for (succno = 0; succno < nsuccs; succno++) {
				if (succs[succno] == fsucc)
					continue;
				if ((succs[succno] < funcstart) || (succs[succno] >= funcend)) {
					printf("Interprocedural jump table?\n");
					continue;
				}
				if ((2 * ffreq < 3 * FREQ(succs[succno])) && IS_PATHTO(succs[succno], fsucc)) {
					if (Debug)
						printf("Found fall-through triangle of %d to %d under %d\n", succs[succno], fsucc, i);
					fsucc = succs[succno];
				}
			}
		}
	}
	else
		SET_SUCC(i);
	if (tsucc < 0) {
		if (fsucc < 0)
			return;
		placetrace(fsucc, funcstart, funcend);
	}
	else {
		if (fsucc >= 0) {
			if (Debug && (ffreq >= tfreq) && (3 * tfreq >= 2 * ffreq) && IS_PATHTO(tsucc, fsucc))
				printf("Found fall-through of %d to %d under %d\n", tsucc, fsucc, i);
			if (Debug && (tfreq >= ffreq) && (3 * ffreq >= 2 * tfreq) && IS_PATHTO(fsucc, tsucc))
				printf("Found fall-through of %d to %d under %d\n", fsucc, tsucc, i);
		}
		if (((ffreq >= tfreq) && !((3 * tfreq >= 2 * ffreq) && IS_PATHTO(tsucc, fsucc))) ||
				((3 * ffreq >= 2 * tfreq) && IS_PATHTO(fsucc, tsucc)))
			placetrace(fsucc, funcstart, funcend);
		else {
			if ((tsucc < funcend) && (tsucc >= funcstart))
				placetrace(tsucc, funcstart, funcend);
			else if (ffreq * LowUsageRatio > tfreq) {
				/* special case - if we have an interprocedural jump
				** and the fall through frequency is "close to" the taken
				** frequency, put the fall through close by.
				*/
				placetrace(fsucc, funcstart, funcend);
			}
		}
	}
}

/*
* Laying out a function is done by laying out traces through the code

* Start with the first node (actually, this could be done better for
* functions that have multiple entry points) and place a trace.  Then,
* look around for nodes that were run freqently (but not as much as an
* alternate branch) and place traces starting with them.  Do this until
* you run out of nodes that were run "enough".  Leave those for the end
* of the program.
*/
static void
layout_function(int funcno)
{
	int tracestart;
	int low_usage_freq;
	long tracefreq;
	int i, j;
	Elf32_Addr *succs;
	int nsuccs;
	int funcstart = Func_info[funcno].start_block;
	int funcend = Func_info[funcno].end_block;

	if (!Func_info[funcno].ncalls)
		return;
	for (i = Func_info[funcno].start_block; i < Func_info[funcno].end_block; i++)
		DEL_FLAGS(i, PLACED);
	compute_dominators(funcstart, funcend);
/*	compute_postdominators(funcstart, funcend);*/
	compute_pathto(funcstart, funcend);

	if (isnt_reducible_func(funcstart, funcend)) {
		Irreducible = 1;
		if (Debug)
			printf("Function beginning at %d(0x%x) is not reducible\n", funcstart, START_ADDR(funcstart));
	}
	else
		Irreducible = 0;
	tracestart = funcstart;
	Setflags = 0;
	low_usage_freq = (Func_info[funcno].ncalls / LowUsageRatio) + BOOL(Func_info[funcno].ncalls % LowUsageRatio);
	low_usage_freq--; /* Make sure it is LESS THAN any acceptable value */
	while (tracestart >= 0) {
		Nbstack = 0;
		placetrace(tracestart, funcstart, funcend);
		tracestart = -1;
		tracefreq = 0;
		for (i = funcstart; i < funcend; i++) {
			if (FLAGS(i) & PLACED_THIS_TRACE) {
				DEL_FLAGS(i, (PLACED_THIS_TRACE));
				ADD_FLAGS(i, PLACED);
			}
			if ((FLAGS(i) & PLACED) && (END_TYPE(i) == END_CJUMP)  && (min(FFREQ(i), TFREQ(i)) > tracefreq)) {
				if (!(FLAGS(FALLTHROUGH(i)) & (PLACED|PLACED_THIS_TRACE)) && (FALLTHROUGH(i) < funcend) && (FALLTHROUGH(i) >= funcstart)) {
					tracefreq = FFREQ(i);
					tracestart = FALLTHROUGH(i);
				}
				else if (!(FLAGS(JUMP_TARGET(i)) & (PLACED|PLACED_THIS_TRACE)) && (JUMP_TARGET(i) >= funcstart) && (JUMP_TARGET(i) < funcend)) {
					tracefreq = TFREQ(i);
					tracestart = JUMP_TARGET(i);
				}
			}
			else if ((FLAGS(i) & PLACED) && (END_TYPE(i) == END_IJUMP)) {
				if (!(nsuccs = get_succs(i, &succs)))
					continue;
				for (j = 0; j < nsuccs; j++) {
					if (!(FLAGS(succs[j]) & PLACED) && (succs[j] >= funcstart) && (succs[j] < funcend) && (FREQ(succs[j]) > tracefreq)) {
						tracefreq = FREQ(succs[j]);
						tracestart = succs[j];
					}
				}
			}
		}
		if (tracefreq < low_usage_freq)
			Setflags = LOW_USAGE;
	}
}

static int
comp_freq(const void *v1, const void *v2)
{
	return(Func_info[*((ulong *) v2)].ncalls - Func_info[*((ulong *) v1)].ncalls);
}

compute_has_effect()
{
	int i;

	for (i = 0; i < Nblocks; i++) {
		if (COMPUTE_HAS_EFFECT(i))
			ADD_FLAGS(i, HAS_EFFECT_FLAG);
		else {
			DEL_FLAGS(i, HAS_EFFECT_FLAG);
			if (FLAGS(i) & START_NOP)
				SET_JUMP_TARGET(i, i + 1);
		}
		SET_ORDER(i, NO_SUCH_ADDR);
	}

}

void
setup_fallthrough()
{
	int i;

	for (i = 0; i < Nblocks; i++)
		SET_FALLTHROUGH(i, i + 1);
}

void
setup_targets()
{
	int i, j;

	if (!Force && !Blocks_insertion) {
		for (i = 0; i < Nblocks; i++) {
			if (END_TYPE(i) != END_IJUMP) {
				while ((JUMP_TARGET(i) != NO_SUCH_ADDR) && !HAS_EFFECT(JUMP_TARGET(i)))
					SET_JUMP_TARGET(i, JUMP_TARGET(JUMP_TARGET(i)));
			}
			switch (END_TYPE(i)) {
			case END_PUSH:
			case END_POP:
			case END_CJUMP:
			case END_CALL:
			case END_PSEUDO_CALL:
			case END_FALL_THROUGH:
				SET_FALLTHROUGH(i, i + 1);
				while (FLAGS(FALLTHROUGH(i)) & START_NOP)
					FALLTHROUGH(i)++;
				while (!HAS_EFFECT(FALLTHROUGH(i)))
					SET_FALLTHROUGH(i, JUMP_TARGET(FALLTHROUGH(i)));
			}
		}
		for (i = 0; i < NJumpTables; i++)
			for (j = 0; j < JumpTables[i].tablen; j++)
				while (!HAS_EFFECT(JumpTables[i].dsts[j]))
					JumpTables[i].dsts[j] = JUMP_TARGET(JumpTables[i].dsts[j]);
	}
}

/*
* Choose an ordering for the code.  If a frequency file and a list of
* functions are supplied, use the frequency file to layout the code
* within the functions.  If only the list is given, just layout the
* blocks of each function in their original order.  If only the
* frequency file is given, leave the function order unchanged and use
* the frequency file for ordering the blocks within the functions.
*/
void
setup_order()
{
	FILE *funcfp = NULL;
	int i, j, func;
	char buf[BUFSIZ];

	Func_order = MALLOC((Nfuncs + 1) * sizeof(int));

	/*
	** The case Orderfile && !Freqfile && Functionfile has been eliminated
	** while parsing arguments
	*/
	if (Orderfile && !Freqfile && !Functionfile)
		order_from_file();
	else {
		if (Functionfile) {
			if (!(funcfp = fopen(Functionfile, "r")))
				error("Cannot open %s\n", Functionfile);
			Nfunc_order = 0;
			while (fgets(buf, BUFSIZ, funcfp)) {
				if (buf[0] == '#')
					continue;
				buf[strlen(buf) - 1] = '\0';
				if ((i = find_dominant_name(buf)) < 0) {
					if (Existwarnings && !No_warnings)
						fprintf(stderr, "Cannot find symbol: %s\n", buf);
				}
				else {
					if (Func_info[i].flags & FUNC_GROUPED)
						continue;
					if (Func_info[i].flags & FUNC_FOUND)
						fprintf(stderr, "Duplicate function: %s\n", buf);
					else {
						Func_info[i].flags |= FUNC_FOUND;
						Func_order[Nfunc_order++] = i;
					}
				}
			}
			fclose(funcfp);
		}
		else if (Freqfile) { /* Freqfile && !Functionfile */
			for (func = 0, Nfunc_order = 0; func < Nfuncs; func++) {
				if ((func >= 1) && (Func_info[func].start_block == Func_info[func - 1].start_block))
					continue;
				if (Func_info[func].flags & FUNC_GROUPED)
					continue;
				Func_order[Nfunc_order++] = func;
			}
			qsort(Func_order, Nfunc_order, sizeof(ulong), comp_freq);
			for (Nfunc_order--; (Nfunc_order >= 0) && (Func_info[Func_order[Nfunc_order]].ncalls == 0); Nfunc_order--)
				;
			Nfunc_order++;
		}
	}
	if (Freqfile) {
		for (i = 0; i < Nfunc_order; i++) {
			func = Func_order[i];
			Func_info[func].flags |= FUNC_PLACED;
			Norder = 0;
			if (Func_info[func].flags & FUNC_SAVE_ORDER)
				func_no_layout(func, 0);
			else
				layout_function(func);
			if (FREQ(Func_info[func].start_block) <= Func_info[func].ncalls / 100)
				Func_info[func].flags |= FUNC_DONT_ALIGN;
			Func_info[func].clen = Norder;
		}
	}
	else if (Functionfile) {
		for (i = 0; i < Nfunc_order; i++) {
			func = Func_order[i];
			Func_info[func].flags |= FUNC_PLACED;
			Norder = 0;
			func_no_layout(func, 0);
			Func_info[func].clen = Norder;
		}
	}

	/* Add all the grouped functions, in sequence */
	for (i = 0; i < Ngroups; i++)
		for (j = Groupinfo[i].start; j < Groupinfo[i].end; j++)
			Func_order[Nfunc_order++] = j;

	/*
	* Catch any functions not caught in earlier passes.  Every block in
	* these functions are "low usage".  Do not set FUNC_PLACED.  This
	* is a signal to fill_up_order to say that the whole function is
	* "low usage".
	*/
	for (func = 0; func < Nfuncs; func++) {
		if (FUNCNO(Func_info[func].start_block) != func)
			continue;
		if (Func_info[func].flags & FUNC_PLACED) {
			fill_in_rest(func);
			continue;
		}
		else {
/*			if (((func > 0) && (Func_info[func].start_block == Func_info[func - 1].start_block) && (Func_info[func - 1].flags & FUNC_PLACED)) ||*/
/*				((func < Nfuncs) && (Func_info[func].start_block == Func_info[func + 1].start_block) && (Func_info[func + 1].flags & FUNC_PLACED))) {*/
/*				Func_info[func].flags |= FUNC_PLACED;*/
/*				continue;*/
/*			}*/
/*			else*/
			if (Func_info[func].flags & FUNC_GROUPED) {
				func_no_layout(func, 0);
				continue;
			}
		}
		func_no_layout(func, LOW_USAGE);
		Func_order[Nfunc_order] = func;
		Nfunc_order++;
	}

	/*
	** Check for loop alignment by looking for blocks that execute
	** much more frequently than their predecessor
	*/
	if (Blocks_stats) {
		ulong lastfreq;

		fill_up_order(1);
		lastfreq = ULONG_MAX / Loopratio;
		for (i = 0; i < Norder; i++) {
			if ((FREQ(Order[i]) > Loopratio * (lastfreq + 1)) && !(IS_FUNCTION_START(Order[i])))
				ADD_FLAGS(Order[i], LOOP_HEADER);
			lastfreq = FREQ(Order[i]);
		}
	}
	if (Orderfile && Freqfile) {
		for (i = 0; i < Nblocks; i++)
			if ((FLAGS(i) & INLINE_CALL) && !(should_i_inline(i)))
				DEL_FLAGS(i, INLINE_CALL);
		order_to_file();
	}

	inline_proc();

	fill_up_order(0);

	for (i = 0; i < Nblocks; i++)
		if (FLAGS(i) & DATA_BLOCK) {
			if (ORDER(i) != NO_SUCH_ADDR)
				fprintf(stderr, "Internal error, exiting\n");
			SET_ORDER(i, Norder);
			ADD_FLAGS(i, LOW_USAGE);
			Order[Norder++] = i;
		}
	SET_ORDER(Nblocks, Norder);
	Order[Norder] = Nblocks;
	SET_FLAGS(Nblocks, 0);

	for (i = 0; i < Norder; i++)
		if (Order[i] == NO_SUCH_ADDR)
			error("%s is missing blocks\n", Orderfile);
	for (func = 0; func < Nfuncs; func++)
		if (FUNCNO(Func_info[func].start_block) != func)
			Func_info[func].order_start = Func_info[FUNCNO(Func_info[func].start_block)].order_start;
}
