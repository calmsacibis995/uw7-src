#ident	"@(#)fur:i386/cmd/fur/jumps.c	1.4"
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

static int Maxalign;

/* No need to be conservative here.  There is the second loop to grow
* the jumps if necessary.  So, assume that the length of the end
* instruction is that which makes the blocks as close as possible.  This
* means a short jump if the target is earlier and a long jump if the
* target is later.
*/
static int
short_possible(ulong blocksrc, ulong blocktarg, ulong delta)
{
	long jumplen;

	if (ORDER(blocksrc) >= ORDER(blocktarg))
		jumplen = ENDPOS(blocksrc) + 2 - NEW_START_ADDR(blocktarg);
	else
		jumplen = NEW_START_ADDR(blocktarg) + delta - (ENDPOS(blocksrc) + 6);
	return((jumplen < 128) || ((jumplen == 128) && (ORDER(blocksrc) >= ORDER(blocktarg))));
}

/* Calculate whether the jump requires a long jump.  This can be a
* little tricky: there is a borderline case where if the instruction is
* short, the jump length is long, but if the instruction is long, the
* jump length is short.  Conservatively, these should be long
* instructions.  So, assume a short instruction and see if it can
* work.
*
*/
static int
need_long(ulong blocksrc, ulong blocktarg, ulong delta, ulong corrective)
{
	long jumplen;
	ulong srcaddr;

	if (corrective)
		srcaddr = CORRPOS(blocksrc) + 2;
	else
		srcaddr = ENDPOS(blocksrc) + 2;
	if (ORDER(blocksrc) >= ORDER(blocktarg))
		jumplen = srcaddr - NEW_START_ADDR(blocktarg);
	else {
		/* Just because the current block is moving delta bytes forward
		** doesn't mean that a later block is moving that far forward.
		** Alignment may absorb some of the distance.
		*/
		if (delta > Maxalign)
			delta -= Maxalign - 1;
		else
			delta = 0;
		jumplen = NEW_START_ADDR(blocktarg) + delta - srcaddr;
	}
	return((jumplen > 128) || ((jumplen == 128) && (ORDER(blocksrc) < ORDER(blocktarg))));
}

static Elf32_Addr
calc_start_addr(int i, long *pdelta)
{
	int j;

	for (j = 0; j < Ngroups; j++)
		if ((Func_info[Groupinfo[j].start].start_block <= i) && (i < Func_info[Groupinfo[j].end].end_block))
			break;
	if (i == Func_info[Groupinfo[j].start].start_block)
		return(0);
	*pdelta = NEW_START_ADDR(Func_info[Groupinfo[j].start].start_block) - NEW_START_ADDR(i) + (START_ADDR(i) - START_ADDR(Func_info[Groupinfo[j].start].start_block));
	ADDTO_NEW_START_ADDR(i, *pdelta);
	return(1);
}

static
growjumps(int order_start, int order_end, int check_funcno, int start_delta)
{
	int i, j, changed;
	Elf32_Addr addr;
	long delta;
	long lastdelta = 0;

	for (changed = 0, delta = start_delta, j = order_start; j < order_end; j++) {
		i = Order[j];
		if (!(FLAGS(i) & GROUPED) || !calc_start_addr(i, &delta)) {
			ADDTO_NEW_START_ADDR(i, delta);
			if (ALIGNMENT(i) && (NEW_START_ADDR(i) % ALIGNMENT(i))) {
				addr = NEW_END_ADDR(Order[j - 1]) + ALIGNMENT(i) - (NEW_END_ADDR(Order[j - 1]) % ALIGNMENT(i));
				delta += addr - NEW_START_ADDR(i);
				SET_NEW_START_ADDR(i, addr);
			}
		}
		switch (NEW_END_TYPE(i)) {
		case END_JUMP:
		case END_CALL:
		case END_CJUMP:
			if ((JUMP_TARGET(i) != NO_SUCH_ADDR) && (NEW_END_INSTLEN(i) < sizeof(Elf32_Addr)) && !(FLAGS(i) & JUMP_GROWN)) {
				if (check_funcno && (FUNCNO(i) != FUNCNO(JUMP_TARGET(i))))
					break;
				if (Blocks_insertion || need_long(i, JUMP_TARGET(i), delta, 0)) {
					if (Func_info[FUNCNO(i)].flags & FUNC_UNTOUCHABLE) {
						fprintf(stderr, "Trying to grow the block of an untouchable function at 0x%x, we are in trouble\n", START_ADDR(i));
						exit(1);
					}
					if (FLAGS(i) & JUMP_SHRUNK)
						DEL_FLAGS(i, JUMP_SHRUNK);
					else
						ADD_FLAGS(i, JUMP_GROWN);
					if (NEW_END_TYPE(i) == END_JUMP) {
						/* 3 = the bigger jump address */
						delta += 3;
						ADDTO_NEW_END_INSTLEN(i, 3);
					}
					else {
						/* 4 = 3 for the bigger jump address + 1 for the instruction code */
						delta += 4;
						ADDTO_NEW_END_INSTLEN(i, 4);
					}
				}
			}
		}
		if ((CORRECTIVE_JUMP_TARGET(i) != NO_SUCH_ADDR) && !(FLAGS(i) & CORRECTIVE_LONG_JUMP)) {
			if (need_long(i, CORRECTIVE_JUMP_TARGET(i), delta, 1)) {
				delta += 3;
				ADD_FLAGS(i, CORRECTIVE_LONG_JUMP);
			}
		}
		ADDTO_NEW_END_ADDR(i, delta);
		if (delta != lastdelta) {
/*			if (Debug)*/
/*				printf("Block %d changed\n", i);*/
			changed++;
			lastdelta = delta;
		}
	}
	return(changed);
}

static void
fix_errata(Elf32_Addr addr, int block)
{
	int add, subtract;
	Elf32_Addr oldcodepos, delta = 0;
	int i, j;

	for (j = ORDER(block); (j < Norder) && (NEW_END_ADDR(Order[j]) <= addr + 33); j++) {
		if ((NEW_END_TYPE(Order[j]) == END_JUMP) && (NEW_END_INSTLEN(Order[j]) != JUMP_LEN)) {
			if (FLAGS(Order[j]) & JUMP_SHRUNK)
				DEL_FLAGS(Order[j], JUMP_SHRUNK);
			else
				ADD_FLAGS(Order[j], JUMP_GROWN);
			delta = 3;
			ADDTO_NEW_END_INSTLEN(Order[j], delta);
			ADDTO_NEW_END_ADDR(Order[j], delta);
/*			printf("Using recourse 1 at address 0x%x\n", addr);*/
			break;
		}
	}
	if (!delta) {
		for (j = ORDER(block); (j < Norder) && (NEW_END_ADDR(Order[j]) <= addr + 33); j++) {
			if ((NEW_END_TYPE(Order[j]) == END_CJUMP) && (NEW_END_INSTLEN(Order[j]) < sizeof(Elf32_Addr))) {
				if (FLAGS(Order[j]) & JUMP_SHRUNK)
					DEL_FLAGS(Order[j], JUMP_SHRUNK);
				else
					ADD_FLAGS(Order[j], JUMP_GROWN);
				delta = 4;
/*				printf("Using recourse 2 at address 0x%x\n", addr);*/
				ADDTO_NEW_END_INSTLEN(Order[j], delta);
				ADDTO_NEW_END_ADDR(Order[j], delta);
				break;
			}
		}
	}
	if (!delta) {
		if (!Blocks_insertion)
			Blocks_insertion = (struct block_insertion *) REZALLOC(Blocks_insertion, (Nblocks + 1) * sizeof(struct block_insertion));
		if (NEW_END_ADDR(block) < addr + 33) {
/*			printf("Using recourse 3 at address 0x%x\n", addr);*/
			for (j = ORDER(block); (j < Norder) && (NEW_END_ADDR(Order[j]) <= addr + 33); j++)
				;
			add = 1;
			subtract = 1000;
		}
		else {
/*			printf("Using recourse 4 at address 0x%x\n", addr);*/
			j = ORDER(block);
			if (addr % 32 == 0x1f) {
				subtract = 1;
				add = 15;
			}
			else {
				subtract = addr % 32 + 2;
				add = 15 - addr % 32;
			}
		}
		oldcodepos = CODEPOS(Order[j]);
		if (PRECODELEN(Order[j])) {
			if (PRECODELEN(Order[j]) >= subtract)
				Blocks_insertion[Order[j]].beg->textsec->d_size -= subtract;
			else
				Blocks_insertion[Order[j]].beg->textsec->d_size += add;
		}
		else {
			Blocks_insertion[Order[j]].beg = MALLOC(sizeof(struct codeblock));
			Blocks_insertion[Order[j]].beg->relsec = &Nulldata;
			Blocks_insertion[Order[j]].beg->symtab = &Nulldata;
			Blocks_insertion[Order[j]].beg->strtab = &Nulldata;
			Blocks_insertion[Order[j]].beg->textsec = MALLOC(sizeof(Elf_Data));
			Blocks_insertion[Order[j]].beg->textsec->d_buf = MALLOC(16);
			Blocks_insertion[Order[j]].beg->textsec->d_size = add;
		}
		nopfill(PRECODE(Order[j]), PRECODE(Order[j]) + PRECODELEN(Order[j]));
		delta = CODEPOS(Order[j]) - oldcodepos;
		ADDTO_NEW_END_ADDR(Order[j], delta);
	}
	for (j++; j < Norder; j++) {
		i = Order[j];
		ADDTO_NEW_START_ADDR(i, delta);
		if (ALIGNMENT(i) && (NEW_START_ADDR(i) % ALIGNMENT(i))) {
			addr = NEW_END_ADDR(Order[j - 1]) + ALIGNMENT(i) - (NEW_END_ADDR(Order[j - 1]) % ALIGNMENT(i));
			delta += addr - NEW_START_ADDR(i);
			SET_NEW_START_ADDR(i, addr);
		}
		ADDTO_NEW_END_ADDR(i, delta);
	}
	ADDTO_NEW_START_ADDR(Nblocks, delta);
}

static int
comp_0f(const void *v1, const void *v2)
{
	return(ORDER(((struct found_0f *) v1)->block) - ORDER(((struct found_0f *) v2)->block));
}

/*
* Figure out where the blocks go.

* First, find what jumps still make sense or need to be added:
	1. an unconditional jump to the next block is unnecessary
	2. a corrective jump must be added if both targets of a conditional
		are moved away
	3. a conditional jump may need to be reversed
	4. a former fall through may now require a corrective jump if it has
		a new successor
*
* The final two steps merely figure out how big a jump instruction to
* use. The first shrinks the jumps where possible and then the second
* grows the jumps where necessary.  This is a very simple approach to
* the problem of sizing jump instructions (if you are not careful, you
* can have an infinite loop).  There is much literature about how to do
* this but this simple approach seems sufficient.
*/
void
setup_jumps(int readonly)
{
	int i, j, k, changed;
	int last_block;
	int count = 1;
	int func;
	Elf32_Addr addr;
	long delta;

	for (i = 0; i < Nblocks; i++) {
		DEL_FLAGS(i, (JUMP_GROWN|JUMP_SHRUNK|REVERSE_END_CJUMP|CORRECTIVE_LONG_JUMP|GROUPED));
		if (Func_info[FUNCNO(i)].flags & FUNC_GROUPED)
			ADD_FLAGS(i, GROUPED);
	}

	Maxalign = max(Funcalign, Loopalign);
	for (addr = 0, j = 0; j < Norder; j++) {
		i = Order[j];
		SET_NEW_START_ADDR(i, addr);
		addr += CODELEN(i) + PRECODELEN(i) + POSTCODELEN(i) + NEW_END_INSTLEN(i);
		/* setup_canonical has already decided to reverse a jump
		** even though it requires a corrective jump
		*/
		SET_CORRECTIVE_JUMP_TARGET(i, NO_SUCH_ADDR);
		switch(NEW_END_TYPE(i)) {
		case END_JUMP:
			if ((j < Norder - 1) && (JUMP_TARGET(i) == Order[j + 1])) {
				SET_NEW_END_TYPE(i, END_FALL_THROUGH);
				addr -= NEW_END_INSTLEN(i);
				SET_NEW_END_INSTLEN(i, 0);
			}
			break;
		case END_CJUMP:
			if ((j == Norder - 1) || ((Order[j + 1] != FALLTHROUGH(i)) && (Order[j + 1] != JUMP_TARGET(i)))) {
				/* Start out by assuming that corrective jumps are short */
				addr += 2;
				if (FLAGS(i) & REVERSE_END_CJUMP) {
					SET_CORRECTIVE_JUMP_TARGET(i, JUMP_TARGET(i));
					SET_JUMP_TARGET(i, FALLTHROUGH(i));
					SET_FALLTHROUGH(i, CORRECTIVE_JUMP_TARGET(i));
				}
				else
					SET_CORRECTIVE_JUMP_TARGET(i, FALLTHROUGH(i));
			}
			else if ((FLAGS(i) & REVERSE_END_CJUMP) || (Order[j + 1] == JUMP_TARGET(i))) {
				int hold = FALLTHROUGH(i);

				ADD_FLAGS(i, REVERSE_END_CJUMP);
				SET_FALLTHROUGH(i, JUMP_TARGET(i));
				SET_JUMP_TARGET(i, hold);
			}
			break;
		case END_PUSH:
		case END_POP:
		case END_CALL:
		case END_FALL_THROUGH:
			if (Order[j + 1] != FALLTHROUGH(i)) {
				/* Start out by assuming that corrective jumps are short */
				SET_CORRECTIVE_JUMP_TARGET(i, FALLTHROUGH(i));
				addr += 2;
			}
			break;
		case END_IJUMP:
		case END_PSEUDO_CALL:
			/* do nothing, no corrective jump needed */
			break;
		}
		SET_NEW_END_ADDR(i, addr);
	};
	SET_NEW_START_ADDR(Nblocks, addr);

	/* First, shrink as much as you can */

	/*
	* We could shrink in the "force" case, but that would merely have
	* the effect of trying to "correct" the assembler's attempts at
	* shrinking; why bother?  However, if this were done, then
	* find_pic_jump_tables would also need to be enabled for the
	* "force" case.
	*/
	if (!Force && !Blocks_insertion) {
		do {
			for (delta = 0, j = 0; j < Norder; j++) {
				i = Order[j];
				ADDTO_NEW_START_ADDR(i, delta);
				if (((NEW_END_TYPE(i) == END_JUMP) || (NEW_END_TYPE(i) == END_CJUMP)) && (JUMP_TARGET(i) != NO_SUCH_ADDR) && (NEW_END_INSTLEN(i) > sizeof(Elf32_Addr))) {
					if (short_possible(i, JUMP_TARGET(i), delta)) {
						if (Func_info[FUNCNO(i)].flags & FUNC_UNTOUCHABLE)
							continue;
						ADD_FLAGS(i, JUMP_SHRUNK);
						if (NEW_END_TYPE(i) == END_JUMP) {
							/* 3 = the bigger jump address */
							delta -= 3;
							SUBTRACT_FROM_NEW_END_INSTLEN(i, 3);
						}
						else {
							/* 4 = 3 for the bigger jump address + 1 for the instruction code */
							delta -= 4;
							SUBTRACT_FROM_NEW_END_INSTLEN(i, 4);
						}
					}
				}
				ADDTO_NEW_END_ADDR(i, delta);
			}
			ADDTO_NEW_START_ADDR(Nblocks, delta);
		} while(delta);
	}
#ifndef SLOWGROW
	func = -1;
	Func_order[Nfunc_order] = Nfuncs;
	Func_info[Nfuncs].order_start = Norder;
	for (k = 0; k < Nfunc_order; k++) {
		func = Func_order[k];
		if (Func_info[func].order_start != 0)
			delta = NEW_END_ADDR(Order[Func_info[func].order_start - 1]) - NEW_START_ADDR(Order[Func_info[func].order_start]);
		else
			delta = 0;
		last_block = Func_info[func].order_start + Func_info[func].clen;
		if (growjumps(Func_info[func].order_start, last_block, 1, delta))
			while (growjumps(Func_info[func].order_start, last_block, 1, 0))
				;
		if (last_block < Func_info[Func_order[k + 1]].order_start) {
			delta = NEW_END_ADDR(Order[last_block - 1]) - NEW_START_ADDR(Order[last_block]);
			if (growjumps(last_block, Func_info[Func_order[k + 1]].order_start, 0, delta))
				while (growjumps(last_block, Func_info[Func_order[k + 1]].order_start, 0, 0))
					;
		}
	}
#endif
	SET_NEW_START_ADDR(Order[Norder], NEW_END_ADDR(Order[Norder - 1]));
	while ((count = growjumps(0, Norder, 0, 0)))
		if (Debug)
			printf("growjumps: %d changes\n", count);
	SET_NEW_START_ADDR(Order[Norder], NEW_END_ADDR(Order[Norder - 1]));

	/* Do not fix the errata in the insertion case */
	if (!SafetyCheck || Blocks_insertion || readonly || Canonical)
		return;
	qsort(Found_0f, Nfound_0f, sizeof(struct found_0f), comp_0f);
	do {
		int i, found1st, found2nd;
		Elf32_Addr addr;
		int rel = 0;

		fixup_symbol_table();
		fill_in_text();
		find_block_reloc();
		fixup_jumps();

		changed = 0;
		for (i = 0; i < Nfound_0f; i++) {
			addr = CODEPOS(Found_0f[i].block) + Found_0f[i].offset;
			if ((addr % 32 > 0xe) && (addr % 32 < 0x1f))
				continue;
			addr += 33;
			if (addr > NEW_START_ADDR(Nblocks)) {
				nopfill(Newtext + NEW_START_ADDR(Nblocks), Newtext + addr);
				SET_NEW_START_ADDR(Nblocks, addr);
				continue;
			}
			for (j = ORDER(Found_0f[i].block); j < Norder; j++)
				if (NEW_START_ADDR(Order[j]) > addr)
					break;
			j--;
			if (NEW_END_ADDR(Order[j]) < addr)
				continue;
			found1st = GET1(Newtext, addr) == 0xf;
			if (!found1st) {
				for (rel = 0; (rel < Nrel) && ((Rel[rel].r_offset + sizeof(Elf32_Addr)) <= addr); rel++)
					;
				if ((rel < Nrel) && (Rel[rel].r_offset <= addr))
					found1st = 2;
			}
			if (found1st) {
				addr++;
				if (addr > NEW_START_ADDR(Nblocks)) {
					nopfill(Newtext + NEW_START_ADDR(Nblocks), Newtext + addr);
					SET_NEW_START_ADDR(Nblocks, addr);
					continue;
				}
				found2nd = (GET1(Newtext, addr) & 0xf0) == 0x80;
				if (!found2nd) {
					for (rel = 0; ((rel < Nrel) && ((Rel[rel].r_offset + sizeof(Elf32_Addr)) <= addr)); rel++)
						;
					if ((rel < Nrel) && (Rel[rel].r_offset <= addr))
						found2nd = 2;
				}
			}
			if (found1st && found2nd) {
				addr = CODEPOS(Found_0f[i].block) + Found_0f[i].offset;
				if (target_erratum_condition(addr, Found_0f[i].block, 1, Textalign)) {
					fix_errata(addr, Found_0f[i].block);
					changed = 1;
					break;
				}
			}
		}
		changed += growjumps(0, Norder, 0, 0);
	} while(changed);
	Text_ready = 1;
}
