#ident	"@(#)fur:i386/cmd/fur/util.c	1.1.2.6"
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
addto(IntNum *x, ulong y, ulong z)
{
	IntNum tmp1, tmp2;

	num_fromulong(&tmp1, y);
	num_fromulong(&tmp2, z);
	num_umultiply(&tmp1, &tmp2);
	num_uadd(x, &tmp1);
}

#define MULTIPLY_AND_ADDTO(X, Y, Z) addto(&(X), Y, Z)

ulong
retdiv1000(IntNum *num, IntNum *den)
{
	IntNum tmp;
	IntNum zero;
	ulong ul;

	num_fromulong(&tmp, 1000);
	num_fromulong(&zero, 0);
	if (num_ucompare(den, &zero) == 0)
		return(0);
	num_umultiply(num, &tmp);
	num_udivide(num, den);
	num_toulong(num, &ul);
	return(ul);
}

#define RETDIV1000(NUM, DEN) return(retdiv1000(&(NUM), &(DEN)))

ulong
calc_maxfunc()
{
	int i;
	int besti;

	besti = 0;
	for (i = 1; i < Nfuncs; i++) {
		if (Func_info[i].ncalls > Func_info[besti].ncalls)
			besti = i;
	}
	return(besti);
}

ulong
calc_lue(int new)
{
#define LUE_FREQ(BLOCK) (new ? FREQ(Order[BLOCK]) : FREQ(BLOCK))
#define LUE_FLAGS(BLOCK) (new ? FLAGS(Order[BLOCK]) : FLAGS(BLOCK))
#define LUE_ADDR(BLOCK) (new ? NEW_START_ADDR(Order[BLOCK]) : START_ADDR(BLOCK))
#define LUE_ENDADDR(BLOCK) (new ? NEW_END_ADDR(Order[BLOCK]) : END_ADDR(BLOCK))

	ulong i, nexti, j;
	ulong topfreq;
	IntNum use, count, num32;
	int end_test = new ? Norder : Ndecodeblocks;
	Elf32_Addr end_addr = new ? NEW_START_ADDR(Order[Norder]) : Text_data->d_size;

	SET_START_ADDR(Nblocks, Text_data->d_size);
	num_fromulong(&use, 0);
	num_fromulong(&count, 0);
	num_fromulong(&num32, 32);
	for (j = 0, i = 0; i < end_addr; i = nexti) {
		nexti = i + 32;
		while ((j < end_test) && (LUE_ENDADDR(j) < i))
			j++;
		do {
			topfreq = 0;
			while ((j < end_test) && (LUE_ADDR(j) < nexti)) {
				MULTIPLY_AND_ADDTO(use, LUE_FREQ(j), (min(nexti, LUE_ENDADDR(j)) - max(i, LUE_ADDR(j))));
				if (topfreq < LUE_FREQ(j))
					topfreq = LUE_FREQ(j);
				if (LUE_ENDADDR(j) > nexti)
					break;
				j++;
				if (LUE_FLAGS(j) & FUNCTION_START)
					break;
			}
			MULTIPLY_AND_ADDTO(count, topfreq, 32);
			if (num_ucompare(&count, &use) < 0)
				printf("Oops\n");
			if (LUE_ENDADDR(j) > nexti)
				break;
		} while ((j < end_test) && (LUE_ADDR(j) < nexti));
	}
	RETDIV1000(use, count);
}

ulong
calc_calls()
{
	IntNum calls;
	IntNum newcalls;
	int i;

	num_fromulong(&calls, 0);
	num_fromulong(&newcalls, 0);
	for (i = 0; i < Ndecodeblocks; i++) {
		if (END_TYPE(i) == END_CALL) {
			if (FLAGS(i) & INLINE_CALL) {
				MULTIPLY_AND_ADDTO(calls, TFREQ(i), 1);
				if ((ORDER(i) != NO_SUCH_ADDR) && (Order[ORDER(i) + 1] != JUMP_TARGET(i)))
					MULTIPLY_AND_ADDTO(newcalls, TFREQ(i), 1);
			}
			else {
				MULTIPLY_AND_ADDTO(calls, TFREQ(i), 1);
				if (ORDER(i) != NO_SUCH_ADDR)
					MULTIPLY_AND_ADDTO(newcalls, TFREQ(i), 1);
			}
		}
	}
	RETDIV1000(newcalls, calls);
}

ulong
calc_jumps()
{
	IntNum jumps;
	IntNum newjumps;
	int i;

	num_fromulong(&jumps, 0);
	num_fromulong(&newjumps, 0);
	for (i = 0; i < Ndecodeblocks; i++) {
		switch (END_TYPE(i)) {
		case END_CJUMP:
			MULTIPLY_AND_ADDTO(jumps, TFREQ(i), 1);
			if (ORDER(i) == NO_SUCH_ADDR)
				continue;
			if (Order[ORDER(i) + 1] != FALLTHROUGH(i))
				MULTIPLY_AND_ADDTO(newjumps, FFREQ(i), 1);
			if (Order[ORDER(i) + 1] != JUMP_TARGET(i))
				MULTIPLY_AND_ADDTO(newjumps, TFREQ(i), 1);
			break;
		case END_IJUMP:
			MULTIPLY_AND_ADDTO(jumps, FREQ(i), 1);
			if (ORDER(i) == NO_SUCH_ADDR)
				continue;
			MULTIPLY_AND_ADDTO(newjumps, FREQ(i), 1);
			break;
		case END_JUMP:
			MULTIPLY_AND_ADDTO(jumps, FREQ(i), 1);
			if (ORDER(i) == NO_SUCH_ADDR)
				continue;
			if (Order[ORDER(i) + 1] != JUMP_TARGET(i))
				MULTIPLY_AND_ADDTO(newjumps, FREQ(i), 1);
		}
	}
	RETDIV1000(newjumps, jumps);
}

ulong
checksum(int funcstart, int funcend)
{
	int i;
	Elf32_Addr j;
	Elf32_Rel *rel;
	ulong sum = 0;
	char *symname;

	for (rel = Textrel; (rel->r_offset < START_ADDR(funcstart)) && (rel < Endtextrel); rel++)
		;
	for (i = funcstart; i < funcend; i++) {
		for (j = START_ADDR(i); j < (END_ADDR(i) - END_INSTLEN(i)); j++)
			sum += GET1(Text_data->d_buf, j);
		for ( ; (rel->r_offset < j) && (rel < Endtextrel); rel++) {
			sum += ELF32_R_TYPE(rel->r_info);
			symname = NAME(Symstart + ELF32_R_SYM(rel->r_info) - 1);
			while (*symname)
				sum += *symname++;
		}
		while ((rel->r_offset < END_ADDR(i)) && (rel < Endtextrel))
			rel++;
		switch (END_TYPE(i)) {
		case END_PSEUDO_CALL:
		case END_RET:
		case END_FALL_THROUGH:
			while (j < END_ADDR(i))
				sum += GET1(Text_data->d_buf, j++);
			continue;
		}
		switch (END_INSTLEN(i)) {
		case LONG_CJUMP_LEN:
			sum += GET1(Text_data->d_buf, j + 1);
			/* FALLTHROUGH */
		case SHORT_CJUMP_LEN:
		case JUMP_LEN:
			sum += GET1(Text_data->d_buf, j);
			break;
		}
		if (JUMP_TARGET(i) == NO_SUCH_ADDR)
			sum += -1;
		else {
			if (FLAGS(JUMP_TARGET(i)) & FUNCTION_START) {
				symname = NAME(START_SYM(JUMP_TARGET(i)));
				while (*symname)
					sum += *symname++;
			}
			else
				sum += JUMP_TARGET(i) - funcstart;
		}
	}
	return(sum);
}

int
find_name(char *name)
{
	long high = Nfuncs, middle, low = 0;
	int ret;
	static int lastfound = -1;
	int i;
	char *basename;

	if (basename = strchr(name, '@'))
		*basename++ = '\0';
	else
		basename = name;
	if ((lastfound >= 0) && (strcmp(basename, NAME(Funcs[Names[lastfound]])) == 0))
		high = low = lastfound;
	do {
		middle = (high + low) / 2;
		if (middle == Nfuncs)
			break;
		if ((ret = strcmp(basename, NAME(Funcs[Names[middle]]))) == 0) {
			lastfound = middle;
			if (basename != name) {
				i = middle;
				do {
					if (strcmp(name, FNAME(Funcs[Names[i]])) == 0) {
						basename[-1] = '@';
						return(Names[i]);
					}
					i--;
				} while ((i >= 0) && (strcmp(basename, NAME(Funcs[Names[i]])) == 0));
				for (i = middle + 1; (i < Nfuncs) && (strcmp(basename, NAME(Funcs[Names[i]])) == 0); i++) {
					if (strcmp(name, FNAME(Funcs[Names[i]])) == 0) {
						basename[-1] = '@';
						return(Names[i]);
					}
				}
				basename[-1] = '@';
				return(-1);
			}
			else
				return(Names[middle]);
		}
		else if (ret > 0)
			low = middle + 1;
		else
			high = middle - 1;
	} while (high >= low);
	if (basename != name)
		basename[-1] = '@';
	return(-1);
}

int
find_dominant_name(char *name)
{
	int func;

	if ((func = find_name(name)) < 0)
		return(func);
	if (FUNCNO(Func_info[func].start_block) != func)
		return(FUNCNO(Func_info[func].start_block));
	return(func);
}

/* Compute the address that a given relocatable reference refers to. */
Elf32_Addr
get_refto(Elf32_Rel *rel, caddr_t sec_data, Elf32_Sym **ppsym, Elf32_Sym *symstart)
{
	Elf32_Sym *sym;

	sym = symstart + ELF32_R_SYM(rel->r_info) - 1;
	if (ppsym)
		*ppsym = sym;
	if (sym->st_shndx != Text_index)
		return(NO_SUCH_ADDR);

	switch (ELF32_R_TYPE(rel->r_info)) {
	case R_386_GOT32:
		return(sym->st_value);
	case R_386_GOTOFF:
		return(sym->st_value + GET4(sec_data, rel->r_offset));
	case R_386_PLT32:
		if (GET4(sec_data, rel->r_offset) != 0xfffffffc)
			error("I do not understand the PLT entry at 0x%x\n", rel->r_offset);
		return(sym->st_value);
	case R_386_BASEOFF:
	case R_386_32:
		return(sym->st_value + GET4(sec_data, rel->r_offset));
	case R_386_PC32:
		/* Since this is sym + ref - loc and since we are looking at
		* a PC-relative instruction (j??/call), the whole calculation
		* will be sym + ref - loc + "end of instruction".  However,
		* "end of instruction" - loc is simply the size of an
		* address.
		*/
		return(sym->st_value + GET4(sec_data, rel->r_offset) + sizeof(Elf32_Addr));
	}
	return(NO_SUCH_ADDR);
}

int
comp_relocations(const void *v1, const void *v2)
{
	return(((Elf32_Rel *) v1)->r_offset - ((Elf32_Rel *) v2)->r_offset);
}

int
comp_orig_addr(const void *v1, const void *v2)
{
	return(CURADDR(*((Elf32_Sym **) v1)) - CURADDR(*((Elf32_Sym **) v2)));
}

int
func_by_addr(Elf32_Addr addr)
{
	long high = Nfuncs, middle, low = 0;
	int ret;

	for ( ; ; ) {
		middle = (high + low) / 2;
		if ((middle == Nfuncs) || (middle <= low))
			break;
		if ((ret = Funcs[middle]->st_value - addr) == 0) {
			low = middle;
			break;
		}
		else if (ret < 0)
			low = middle;
		else
			high = middle;
	}
	return(low);
}

int
func_by_newaddr(Elf32_Addr addr)
{
	long high = Nfunc_order, middle, low = 0;
	int ret;

	for ( ; ; ) {
		middle = (high + low) / 2;
		if (middle == Nfuncs)
			return(Nfuncs);
		if (middle <= low)
			break;
		if ((ret = NEW_START_ADDR(Order[Func_info[Func_order[middle]].order_start]) - addr) == 0) {
			low = middle;
			break;
		}
		else if (ret < 0)
			low = middle;
		else
			high = middle;
	}
	return(Func_order[low]);
}

void
param_set(int i, int val, int precedence)
{
	if (Params[i].precedence < precedence) {
		*(Params[i].val) = val;
		Params[i].precedence = precedence;
	}
}

void
param_set_env()
{
	int i;
	char buf[100];

	for (i = 0; i < Nparams; i++) {
		sprintf(buf, "_FUR_%s", Params[i].name);
		if (getenv(buf))
			param_set(i, atoi(getenv(buf)), 2);
	}
}

void
param_set_var_val(const char *str, int precedence)
{
	int i;

	if (!strchr(str, '='))
		error("Illegal variable: %s\n", str);
	for (i = 0; i < Nparams; i++)
		if (strncmp(str, Params[i].name, strlen(Params[i].name)) == 0) {
			param_set(i, strtol(strchr(str, '=') + 1, NULL, 0), precedence);
			return;
		}

	error("Illegal variable: %s\n", str);
}

/*
** Get a section, abort if it fails
*/
Elf_Data *
#ifdef __STDC__
myelf_getdata(Elf_Scn *scn, Elf_Data *data, const char *errmsg)
#else
myelf_getdata(scn, data, errmsg)
Elf_Scn *scn;
Elf_Data *data;
const char *errmsg;
#endif
{
	Elf_Data *td;
	if ((td = elf_getdata(scn, data)) == NULL)
		error("cannot get data for %s\n", errmsg);
	return(td);
}

void
#ifdef __STDC__
usage(char *msg)
#else
usage(msg)
char *msg;
#endif
{
	fprintf(stderr, "%s: %s\n", prog, msg);
	fprintf(stderr, "Usage:\n    %s [ -v ] [ -f frequency-file ] [ -l function-file ] [ -o order-file ] reloc_file\n", prog);
	fprintf(stderr, "\t-v - view frequency-file\n\t-f frequency-file - created by logging blocks\n\t-l function-file - function ordering created by lrt_scan\n\t-o order-file - to be created if frequency or function-file is supplied\n\t-o order-file - to be used to order reloc_file\n\t\n");
	fprintf(stderr, "    %s [ -c compile-line ] [ -B block-logging-binary ] [ -b all ] [ -P prologue-logging-binary ] [ -p all ] [ -E epilogue-logging-binary ] [ -e all ] reloc_file\n", prog);
	exit(-1);
}

void
#ifdef __STDC__
error(const char *fmt, ...)
#else
error(va_alist)
va_dcl
#endif
{
	va_list ap;
#ifdef __STDC__
	va_start(ap, fmt);
#else
	char *fmt;
	va_start(ap);
	fmt = va_arg(ap, char *);
#endif
	fprintf(stderr, "%s: ", prog);
	vfprintf(stderr, fmt, ap);
	exit(-1);
}

/* This is used by the dis code to tell fur that there has been an
** error while disassembling
*/
void
bad_opcode()
{
	error("Error disassembling at address 0x%x\n", loc);
}

comp_new_addr(const void *v1, const void *v2)
{
	return(Func_info[*((ulong *) v1)].order_start - Func_info[*((ulong *) v2)].order_start);
}

int
comp_names(const void *v1, const void *v2)
{
	int ret;

	if (ret = strcmp(NAME(Funcs[*((ulong *) v1)]), NAME(Funcs[*((ulong *) v2)])))
		return(ret);
	return(strcmp(FNAME(Funcs[*((ulong *) v1)]), FNAME(Funcs[*((ulong *) v2)])));
}

void
sort_by_name()
{
	int i;

	Names = (ulong *) REALLOC(Names, Nfuncs * sizeof(ulong));
	for (i = 0; i < Nfuncs; i++)
		Names[i] = i;
	qsort(Names, Nfuncs, sizeof(ulong), comp_names);
}

void
errexit(const char *fmt, ...)
{
	va_list ap;
	ulong arg1, arg2;

	va_start(ap, fmt);
	arg1 = va_arg(ap, ulong);
	arg2 = va_arg(ap, ulong);
	fprintf(stderr, fmt, arg1, arg2);
	va_end(ap);
	exit(1);
}

void *
out_of_memory()
{
	fprintf(stderr, "Program ran out of memory\n");
	exit(1);
	return(0);
}

int
block_by_addr(Elf32_Addr addr)
{
	long high = Ndecodeblocks, middle, low = 0;
	int ret;

	for ( ; ; ) {
		middle = (high + low) / 2;
		if (middle == low)
			break;
		if ((ret = START_ADDR(middle) - addr) == 0)
			break;
		else if (ret < 0)
			low = middle;
		else
			high = middle;
	}
	return(middle);
}

static char *
callee_name(int block)
{
	static int last_block;
	static char *last_callee;
	Elf32_Rel *high = Endtextrel, *low = Textrel, *rel;

	if (last_callee && (last_block == block))
		return(last_callee);

	do {
		rel = low + (high - low) / 2;
		if (END_ADDR(block) > rel->r_offset)
			low = rel;
		else
			high = rel;
	} while (high > low + 1);
	if (low->r_offset == END_ADDR(block) - sizeof(ulong)) {
		last_block = block;
		last_callee = NAME(Symstart + ELF32_R_SYM(low->r_info) - 1);
		return(last_callee);
	}
	return(NULL);
}

static void
viewfreq()
{
	int i, namei;

	for (i = 0; i < Nblocks; i++) {
		if (IS_FUNCTION_START(i))
			namei = i;
		if (!FREQ(i))
			continue;
		printf("%s%s%s+%d(0x%x) -> %d\n", FULLNAME(START_SYM(namei)), i - namei, START_ADDR(i), FREQ(i));
		if (END_TYPE(i) == END_JUMP) {
			if (JUMP_TARGET(i) == NO_SUCH_ADDR)
				printf("%s%s%s+%d(0x%x) -> unknown\n", FULLNAME(START_SYM(namei)), i - namei, START_ADDR(i));
			else
				printf("%s%s%s+%d(0x%x) -> %-.11s+%d(0x%x)\n", FULLNAME(START_SYM(namei)), i - namei, START_ADDR(i), NAME(START_SYM(namei)), JUMP_TARGET(i) - namei, START_ADDR(JUMP_TARGET(i)));
		}
		else if (END_TYPE(i) == END_IJUMP)
			printf("%s%s%s+%d(0x%x) -> unknown\n", FULLNAME(START_SYM(namei)), i - namei, START_ADDR(i));
		else if ((END_TYPE(i) == END_CJUMP) && (TFREQ(i) || FFREQ(i)))
			printf("%s%s%s+%d(0x%x) -> fallthrough: %d, %-.11s+%d(0x%x): %d\n", FULLNAME(START_SYM(namei)), i - namei, START_ADDR(i), FFREQ(i), NAME(START_SYM(namei)), JUMP_TARGET(i) - namei, START_ADDR(JUMP_TARGET(i)), TFREQ(i));
		else if (END_TYPE(i) == END_CALL) {
			if (JUMP_TARGET(i) != NO_SUCH_ADDR)
				printf("%s%s%s+%d(0x%x) CALLS %s%s%s\n", FULLNAME(START_SYM(namei)), i - namei, START_ADDR(i), FULLNAME(START_SYM(JUMP_TARGET(i))));
			else if (callee_name(i))
				printf("%s%s%s+%d(0x%x) CALLS %s\n", FULLNAME(START_SYM(namei)), i - namei, START_ADDR(i), callee_name(i));
			else
				printf("%s%s%s+%d(0x%x) CALLS unknown\n", FULLNAME(START_SYM(namei)), i - namei, START_ADDR(i));
		}
	}
}

static
comp_callfreq(const void *v1, const void *v2)
{
	int block1, block2;

	block1 = *((int *) v1);
	block2 = *((int *) v2);
	return(FREQ(block2) - FREQ(block1));
}

/*
** Uses three different ways of guessing the number of times that a
** function was really "called"; the maximum of the three will be used.
** 1)  The number of times the first block of the routine is executed,
** 2)  The number of times the routine exits
** 3)  The number of times another routine calls this routine
*/
static void
count_func_calls()
{
	int i;
	ulong *jumps_in;
	int check_fallthrough;
	int jumpin_check_jump_target, jumpout_check_jump_target;

	jumps_in = (ulong *) CALLOC(Nfuncs, sizeof(ulong));
	for (i = 0; i < Nblocks; i++) {
		check_fallthrough = jumpin_check_jump_target = jumpout_check_jump_target = 0;
		if (!HAS_EFFECT(i))
			continue;
		switch(END_TYPE(i)) {
		case END_PUSH:
		case END_POP:
			break;
		case END_PSEUDO_CALL:
		case END_FALL_THROUGH:
			check_fallthrough = 1;
			break;
		case END_CALL:
			jumpin_check_jump_target = 1;
			check_fallthrough = 1;
			break;
		case END_CJUMP:
			check_fallthrough = 1;
			jumpin_check_jump_target = 1;
			jumpout_check_jump_target = 1;
			break;
		case END_JUMP:
			jumpin_check_jump_target = 1;
			jumpout_check_jump_target = 1;
			break;
		case END_RET:
			Func_info[FUNCNO(i)].ncalls += FREQ(i);
			break;
		case END_IJUMP:
			{
				Elf32_Addr *succs;
				int nsuccs;
				int j;

				nsuccs = get_succs(i, &succs);
				for (j = 0; j < nsuccs; j++) {
					if (FUNCNO(succs[j]) != FUNCNO(i)) {
						Func_info[FUNCNO(i)].ncalls += FREQ(succs[j]);
						jumps_in[FUNCNO(succs[j])] += FREQ(succs[j]);
					}
				}
			}
		}
		if (jumpin_check_jump_target && (JUMP_TARGET(i) != NO_SUCH_ADDR) && (FUNCNO(i) != FUNCNO(JUMP_TARGET(i))))
			jumps_in[FUNCNO(JUMP_TARGET(i))] += TFREQ(i);
		if (jumpout_check_jump_target && (JUMP_TARGET(i) != NO_SUCH_ADDR) && (FUNCNO(i) != FUNCNO(JUMP_TARGET(i))))
			Func_info[FUNCNO(i)].ncalls += TFREQ(i);
		if (check_fallthrough && (FALLTHROUGH(i) != NO_SUCH_ADDR) && (FUNCNO(i) != FUNCNO(FALLTHROUGH(i)))) {
			Func_info[FUNCNO(i)].ncalls += FFREQ(i);
			jumps_in[FUNCNO(FALLTHROUGH(i))] += FFREQ(i);
		}
	}
	for (i = 0; i < Nfuncs; i++) {
		if (Func_info[i].ncalls < FREQ(Func_info[i].start_block))
			Func_info[i].ncalls = FREQ(Func_info[i].start_block);
		if (Func_info[i].ncalls < jumps_in[i])
			Func_info[i].ncalls = jumps_in[i];
/*		printf("%s%s%s called %d times\n", FULLNAME(Funcs[i]), Func_info[i].ncalls);*/
	}
	free(jumps_in);
}

/*
** Calculate direct calls to each function, order them and make
** inlining decisions based on direct calls.
*/
static void
count_direct_calls()
{
	int print_count_calls;
	int *order;
	int i;
	ulong ncalls;
	IntNum totalcalls, work;
	long workcalls;

	print_count_calls = BOOL(getenv("COUNT_CALLS"));
	order = (int *) MALLOC(Nblocks * sizeof(int));
	num_fromulong(&totalcalls, 0);
	for (ncalls = i = 0; i < Nblocks; i++) {
		if ((END_TYPE(i) != END_CALL) || (FREQ(i) == 0) || (JUMP_TARGET(i) == NO_SUCH_ADDR))
			continue;
		order[ncalls] = i;
		ncalls++;
		num_fromulong(&work, (ulong) FREQ(i));
		num_uadd(&totalcalls, &work);
	}
	num_fromulong(&work, InlineCriteria);
	num_umultiply(&totalcalls, &work);
	num_fromulong(&work, 100);
	num_udivide(&totalcalls, &work);
	num_toslong(&totalcalls, &workcalls);
	qsort(order, ncalls, sizeof(int), comp_callfreq);
	for (i = 0; i < ncalls; i++) {
		if (print_count_calls)
			printf("%s%s%s(0x%x) CALLS %s%s%s %d times\n", FULLNAME(START_SYM(order[i])), START_ADDR(order[i]), FULLNAME(Funcs[FUNCNO(JUMP_TARGET(order[i]))]), FREQ(order[i]));
		SET_CALL_ORDER(order[i], i);
		if ((workcalls > 0) && (JUMP_TARGET(order[i]) != NO_SUCH_ADDR)) {
			if ((FUNCNO(JUMP_TARGET(order[i])) != FUNCNO(order[i])) &&
				(InlineCallRatio * Func_info[FUNCNO(JUMP_TARGET(order[i]))].ncalls <= 100 * FREQ(order[i]))) {
/*				printf("%s%s%s(0x%x) CALLS %s%s%s %d times which executes %d times total\n", FULLNAME(START_SYM(order[i])), START_ADDR(order[i]), FULLNAME(Funcs[FUNCNO(JUMP_TARGET(order[i]))]), FREQ(order[i]), Func_info[FUNCNO(JUMP_TARGET(order[i]))].ncalls);*/
				ADD_FLAGS(order[i], INLINE_CALL);
			}
			workcalls -= FREQ(order[i]);
		}
	}
	free(order);
}

check_and_propagate(struct flowcount *flowcount, int block, int freq)
{
	if (block == NO_SUCH_ADDR)
		return;
	if (!freq)
		return;
	if (FLAGS(block) & FLOW_BLOCK)
		return;
	propagate(flowcount, block, freq);
}

int
propagate(struct flowcount *flowcount, int block, int freq)
{
	if (FLAGS(block) & BEENHERE)
		return;
	ADD_FLAGS(block, BEENHERE);
	ADDTO_FREQ(block, freq);
	switch (END_TYPE(block)) {
	case END_IJUMP:
	case END_CJUMP:
		break;
	case END_JUMP:
		check_and_propagate(flowcount, JUMP_TARGET(block), freq);
		ADDTO_TFREQ(block, freq);
		break;
	case END_CALL:
		ADDTO_TFREQ(block, freq);
		ADDTO_FFREQ(block, freq);
		check_and_propagate(flowcount, JUMP_TARGET(block), freq);
		check_and_propagate(flowcount, FALLTHROUGH(block), freq);
		break;
	case END_RET:
		break;
	case END_FALL_THROUGH:
	case END_PSEUDO_CALL:
	case END_POP:
	case END_PUSH:
		ADDTO_FFREQ(block, freq);
		check_and_propagate(flowcount, FALLTHROUGH(block), freq);
	}
	DEL_FLAGS(block, BEENHERE);
}

void
proc_freq()
{
	int i, j;
	char *freqfile;
	struct stat stat;
	int ffd;
	int flowblocks;
	struct blockcount *blockcount;
	struct flowcount *flowcount;

	if (!Freqfile || ((Freqfile[0] == '.') && (Freqfile[1] == '\0')))
		return;
	for (freqfile = strtok(Freqfile, ", "); freqfile; freqfile = strtok(NULL, ", ")) {
		char buf[8];
		void *mapbuf;

		if ((ffd = open(freqfile, O_RDONLY)) < 0)
			error("Cannot open %s\n", freqfile);

		if (fstat(ffd, &stat) < 0)
			error("Cannot stat %s\n", freqfile);

/*		if ((stat.st_size != Nblocks * sizeof(struct blockcount)) && (stat.st_size != ((Nblocks * sizeof(struct blockcount) + 4095) & ~4095)))*/
		if (stat.st_size != Nblocks * sizeof(struct blockcount))
			flowblocks = 1;
		else
			flowblocks = 0;
		if ((mapbuf = mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, ffd, 0)) == (struct blockcount *) -1)
			error("mmap failed: %s\n", strerror(errno));

		if (flowblocks) {
			int expected_count = stat.st_size / sizeof(struct flowcount);
			flowcount = (struct flowcount *) mapbuf;
			for (i = 0; i < Nblocks; i++)
				DEL_FLAGS(i, (BEENHERE|FLOW_BLOCK));
			for (i = 0; i < Nblocks; i++) {
				if (Func_info[FUNCNO(i)].flags & FUNC_GROUPED)
					continue;
				if (FLAGS(i) & (START_NOP|DATA_BLOCK))
					continue;
				if (END_TYPE(i) == END_CJUMP)
					ADD_FLAGS(i, FLOW_BLOCK);
				else  {
					if (FLAGS(i) & ENTRY_POINT)
						ADD_FLAGS(i, FLOW_BLOCK);
					if (END_TYPE(i) == END_IJUMP) {
						Elf32_Addr *succs;
						int nsuccs;

						if (nsuccs = get_succs(i, &succs))
							for (j = 0; j < nsuccs; j++)
								if (END_TYPE(succs[j]) != END_CJUMP)
									ADD_FLAGS(succs[j], FLOW_BLOCK);
					}
				}
			}
			for (j = 0, i = 0; i < Nblocks; i++) {
				if (!(FLAGS(i) & FLOW_BLOCK))
					continue;
				if (j >= expected_count)
					error("%s does not match number of blocks in file\n", freqfile);
				if (flowcount[j].callcount) {
					switch (END_TYPE(i)) {
					case END_CJUMP:
						ADDTO_FREQ(i, flowcount[j].callcount);
						ADDTO_FFREQ(i, flowcount[j].firstcount);
						ADDTO_TFREQ(i, flowcount[j].secondcount);
						ADD_FLAGS(i, BEENHERE);
						check_and_propagate(flowcount, i + 1, flowcount[j].firstcount);
						check_and_propagate(flowcount, JUMP_TARGET(i), flowcount[j].secondcount);
						DEL_FLAGS(i, BEENHERE);
						break;
					default:
						propagate(flowcount, i, flowcount[j].callcount);
					}
				}
				j++;
			}
			if (expected_count != j)
				error("%s does not match number of blocks in file\n", freqfile);
		}
		else {
			blockcount = (struct blockcount *) mapbuf;
			for (i = 0; i < Nblocks; i++) {
				ADDTO_FREQ(i, blockcount[i].callcount);
				if (blockcount[i].firstcount) {
					if (blockcount[i].firstblock == i + 1) {
						ADDTO_FFREQ(i, blockcount[i].firstcount);
						ADDTO_TFREQ(i, blockcount[i].secondcount);
					}
					else {
						ADDTO_FFREQ(i, blockcount[i].secondcount);
						ADDTO_TFREQ(i, blockcount[i].firstcount);
					}
				}
			}
		}
		munmap(mapbuf, stat.st_size);
		close(ffd);
	}

	if (Mergefile) {
		FILE *mfp;
		struct blockcount blockcount;

		if (!(mfp = fopen(Mergefile, "w")))
			error("Cannot open %s\n", Mergefile);
		for (i = 0; i < Nblocks; i++) {
			blockcount.callcount = FREQ(i);
			if (FFREQ(i)) {
				blockcount.firstblock = i + 1;
				blockcount.firstcount = FFREQ(i);
				blockcount.secondcount = TFREQ(i);
			}
			else {
				blockcount.firstblock = JUMP_TARGET(i);
				blockcount.firstcount = TFREQ(i);
				blockcount.secondcount = FFREQ(i);
			}
			if (!fwrite(&blockcount, sizeof(struct blockcount), 1, mfp))
				error("Cannot complete write to %s\n", Mergefile);
		}
		fclose(mfp);
	}
	if (Viewfreq)
		viewfreq();
	count_func_calls();
	if (InlineCriteria || getenv("COUNT_CALLS"))
		count_direct_calls();
}
