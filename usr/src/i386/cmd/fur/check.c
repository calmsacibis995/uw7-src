#ident	"@(#)fur:i386/cmd/fur/check.c	1.1.1.2"
#ident	"$Header:"

#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#define _KERNEL
#include <sys/bitmasks.h>
#undef _KERNEL
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

void
checkinstructions()
{
#ifdef COMMENTED_OUT
	Elf32_Addr addr;
	int k;
	int i, j;
	int ifunc;
	char *p, *q;
	Elf32_Rel *rel;

	if (Sizes) {
		for (addr = 0, j = 0, i = 0; (j < Nblocks) && (i < Nsizes); j++) {
			while (addr < END_ADDR(j)) {
				if (addr + Sizes[i] > END_ADDR(j))
					fprintf(stderr, "Error: jump into middle of instruction beginning at 0x%x\n", addr);
				addr += Sizes[i++];
			}
		}
		for (rel = Textrel, i = 0, addr = 0; rel < Endtextrel; rel++) {
			while ((addr + Sizes[i]) < rel->r_offset)
				addr += Sizes[i++];
			if ((rel->r_offset <= addr) || ((rel->r_offset + 4) > (addr + Sizes[i])))
				fprintf(stderr, "Bad relocation: addr 0x%x\n", rel->r_offset);
		}
		free(Sizes);
	}
	for (i = 0; i < Nblocks; i++) {
		if (((END_TYPE(i) != END_JUMP) && (END_TYPE(i) != END_CJUMP)) || (JUMP_TARGET(i) == NO_SUCH_ADDR))
			continue;
		if (i >= JUMP_TARGET(i)) {
			for (j = i; (j >= 0) && !IS_FUNCTION_START(j) && (j != JUMP_TARGET(i)); j--)
				;
		}
		else
			for (j = i + 1; (j < Nblocks) && !IS_FUNCTION_START(j) && (j != JUMP_TARGET(i)); j++)
				;
		if ((j == JUMP_TARGET(i)) && !IS_FUNCTION_START(j))
			continue;
		if (j < i)
			ifunc = j;
		else {
			for (j = i; (j >= 0) && !IS_FUNCTION_START(j) && (j != JUMP_TARGET(i)); j--)
				;
			ifunc = j;
		}
		for (j = JUMP_TARGET(i); (j >= 0) && !IS_FUNCTION_START(j); j--)
				;
		if ((START_SYM(ifunc)->st_size == 0) && (START_SYM(ifunc)->st_value == 0)) {
			for (k = 0; k < Nfuncs; k++)
				if ((Funcs[k]->st_value == 0) && Funcs[k]->st_size)
					break;
			if (k < Nfuncs)
				SET_START_SYM(ifunc, Funcs[k]);
		}
		if ((START_SYM(j)->st_size == 0) && (START_SYM(j)->st_value == 0)) {
			for (k = 0; k < Nfuncs; k++)
				if ((Funcs[k]->st_value == 0) && Funcs[k]->st_size)
					break;
			if (k < Nfuncs)
				SET_START_SYM(j, Funcs[k]);
		}
		for (p = NAME(START_SYM(ifunc)), q = NAME(START_SYM(j)); *p && *q; p++, q++)
			if (*p != *q)
				break;
		if (*p == *q)
			continue;
		if (!*p && (strncmp(q, ".low.usage", sizeof(".low.usage") - 1) == 0))
			continue;
		if (!*q && (strncmp(p, ".low.usage", sizeof(".low.usage") - 1) == 0))
			continue;
/*		fprintf(stderr, "Interprocedural jump from %s%s%s to %s%s%s\n", FULLNAME(START_SYM(ifunc)), FULLNAME(START_SYM(j)));*/
		fprintf(stderr, "Interprocedural jump from %s to %s\n", NAME(START_SYM(ifunc)), NAME(START_SYM(j)));
	}
#endif
}

/*
** One way to find data in the text section is to look for relocations
** from the text section to the text section which do not appear
** in jump statements.
*/
void
checkdata()
{
	int i;
	Elf32_Rel *rel;

	if (No_warnings)
		return;
	for (i = 0, rel = Textrel; rel < Endtextrel; rel++) {
		while (END_ADDR(i) < rel->r_offset)
			i++;
		if (((Symstart + ELF32_R_SYM(rel->r_info) - 1)->st_shndx == Text_index) && (rel->r_offset != END_ADDR(i) - sizeof(Elf32_Addr))) {
			Elf32_Addr addr;
			int func;

			if (GET4(Text_data->d_buf, rel->r_offset) == 0)
				continue;
			/*
			** We now know that a statement that is not a "jump"
			** statement has a relocation to a text address.  We
			** are already pretty sure that it is not the start of
			** a function, but we will make sure here.
			*/
			addr = get_refto(rel, Text_data->d_buf, NULL, Symstart);
			func = func_by_addr(addr);
			if (Funcs[func]->st_value != addr) {
				int i, found;

				/*
				** So, now we know that this is not a function address.
				** Let's make sure that there is no way of getting
				** to this address by jumping or falling into it.
				*/
				for (i = 0, found = 0; (i < Nblocks) && !found; i++) {
					switch (END_TYPE(i)) {
					case END_CJUMP:
					case END_CALL:
						if ((START_ADDR(i + 1) <= addr) && (END_ADDR(i + 1) > addr)) {
							found = 1;
							break;
						}
						/* FALLTHROUGH */
					case END_JUMP:
						if ((JUMP_TARGET(i) != NO_SUCH_ADDR) && (START_ADDR(JUMP_TARGET(i)) <= addr) && (END_ADDR(JUMP_TARGET(i)) > addr)) {
							found = 1;
							break;
						}
					case END_PSEUDO_CALL:
					case END_FALL_THROUGH:
						if ((START_ADDR(i + 1) <= addr) && (END_ADDR(i + 1) > addr)) {
							found = 1;
							break;
						}
					}
				}
				if (!found)
					fprintf(stderr, "Warning: possible data in text segment at address 0x%x in function %s\n", addr, NAME(Funcs[func]));
			}
		}
	}
}

void
check_for_erratum()
{
	int i, found1st, found2nd;
	Elf32_Addr addr;
	Elf32_Rel *rel = Textrel;

	for (i = 0; i < Nfound_0f; i++) {
		addr = START_ADDR(Found_0f[i].block) + Found_0f[i].offset;
		if ((Text_data->d_align >= 32) && (addr % 32 > 0xe) && (addr % 32 < 0x1f))
			continue;
		found2nd = 0;
		if (addr + 33 > Text_data->d_size)
			found2nd = found1st = 1;
		else
			found1st = GET1(Text_data->d_buf, addr + 33) == 0xf;
		if (!found1st) {
			for ( ; (rel < Endtextrel) && ((rel->r_offset + sizeof(Elf32_Addr)) <= (addr + 33)); rel++)
				;
			if ((rel < Endtextrel) && (rel->r_offset <= addr + 33))
				found1st = 1;
		}
		if (found1st && !found2nd) {
			if (addr + 34 > Text_data->d_size)
				found2nd = 1;
			else
				found2nd = (GET1(Text_data->d_buf, addr + 34) & 0xf0) == 0x80;
			if (!found2nd) {
				for ( ; (rel < Endtextrel) && ((rel->r_offset + sizeof(Elf32_Addr)) <= (addr + 34)); rel++)
					;
				if ((rel < Endtextrel) && (rel->r_offset <= addr + 34))
					found2nd = 1;
			}
		}
		if (found1st && found2nd) {
			if (target_erratum_condition(addr, Found_0f[i].block, 0, Text_data->d_align))
				printf("Unsafe code found at address 0x%x\n", addr);
		}
	}
}

target_erratum_condition(Elf32_Addr addr, int block, int gen, int align)
{
	int i, j, k, prev;
	Elf32_Addr highend, lowend;

	if (align >= 16) {
		if (addr % 16 == 0xf)
			highend = addr + 1;
		else
			highend = addr & ~15;
		if (highend <= 16)
			return(1);
		lowend = highend - 16;
	}
	else {
		highend = addr + 1;
		if (highend <= 30)
			return(1);
		lowend = addr - 30;
	}
	if (gen) {
		for (j = ORDER(block); NEW_START_ADDR(Order[j]) > highend; j--)
			;
	}
	else {
		for (j = block; START_ADDR(j) > highend; j--)
			;
	}
	/* The start address of the first block is 0 which is never > lowend */
	for ( ; ; j--) {
		if (gen) {
			i = Order[j];
			prev = Order[j - 1];
			if (NEW_START_ADDR(i) < lowend)
				break;
		}
		else {
			i = j;
			prev = i - 1;
			if (START_ADDR(i) < lowend)
				break;
		}
		if (FLAGS(i) & ENTRY_POINT)
			return(1);
		switch(END_TYPE(prev)) {
		case END_FALL_THROUGH:
			if (END_INSTLEN(prev))
				return(1);
			/* FALLTHROUGH */
		case END_CJUMP:
			for (k = 0; k < Nblocks; k++)
				if ((JUMP_TARGET(k) == i) || (gen && (CORRECTIVE_JUMP_TARGET(k) == i)))
					return(1);
			break;
		default:
			return(1);
		}
	}
	return(0);
}
