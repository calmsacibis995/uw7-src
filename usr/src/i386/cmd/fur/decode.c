#ident	"@(#)fur:i386/cmd/fur/decode.c	1.1.2.8"
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
#include "dis.h"

extern int Pushdebug;
static int Sends;

typedef struct myint_64 myint_64;

#define LT(X, Y) (num_ucompare(&X, &Y) < 0)

void
push_addr(Elf32_Addr addr)
{
	if (addr > Text_data->d_size)
		error("Jump out of code to address 0x%x\n", addr);
	if (BITMASKN_TEST1(Starts, addr))
		return;
	if (Debug && Pushdebug)
		printf("Pushing address 0x%x\n", addr);
	if (Nstack == Sstack) {
		if (Sstack == 0)
			Sstack = 100;
		else
			Sstack *= 2;
		Stack = REALLOC(Stack, Sstack * sizeof(Elf32_Addr));
	}
	Stack[Nstack++] = addr;
}

static int
comp_num(const void *v1, const void *v2)
{
	return(*((Elf32_Addr *) v1) - *((Elf32_Addr *) v2));
}

void
mark_entry_point(Elf32_Addr start_addr)
{
	long high = Ndecodeblocks, middle, low = 0;

	do {
		middle = (high + low) / 2;
		if (START_ADDR(middle) == start_addr)
			break;
		else if (START_ADDR(middle) < start_addr)
			low = middle + 1;
		else
			high = middle - 1;
	} while (high >= low);
	ADD_FLAGS(middle, ENTRY_POINT);
}

static void
block_start(Elf32_Addr start_addr, int flags)
{
	if (start_addr == Funcs[Nfuncs]->st_value)
		return;
	if (Pass2) {
		if (flags & ENTRY_POINT)
			mark_entry_point(start_addr);
		return;
	}
	if (flags & START_NOP) {
		if (!(Nnops % 100))
			Nops = (Elf32_Addr *) REALLOC(Nops, (Nnops + 100) * sizeof(Elf32_Addr));
		Nops[Nnops++] = start_addr;
	}
	if (start_addr > Text_data->d_size)
		error("Jump out of code to address 0x%x\n", start_addr);
	if (BITMASKN_TEST1(Starts, start_addr))
		return;
	push_addr(start_addr);
	BITMASKN_SET1(Starts, start_addr);
}

static void
block_end(Elf32_Addr end_addr, int end_type, Elf32_Addr jump_addr, int end_instlen)
{
	if ((end_type == END_CALL) && (end_addr == jump_addr)) {
		end_type = END_PSEUDO_CALL;
		Found_pseudo++;
	}
	if (Nends >= Sends - 1) {
		if (!Sends)
			Sends = Text_data->d_size / 20 + 2;
		else
			Sends += Sends / 10 + 2;
		Ends = (struct end *) REALLOC(Ends, Sends * sizeof(struct end));
	}
	Ends[Nends].end_addr = end_addr;
	Ends[Nends].end_type = end_type;
	Ends[Nends].end_instlen = end_instlen;
	Ends[Nends++].jump_addr = jump_addr;
}

/*
* Combine the information about the starts and ends to form a list of
* blocks.  NOP blocks are noted but not removed. Jumps to NOP blocks
* are moved to the successors
*/
static void
mergeblocks()
{
	int end;
	int isdata;
	int i, j;
	int msg = 0;
	uint_t mask, maskword;
	Elf32_Addr addr;
	int data;
	int func;

	if (Nends) {
		qsort(Ends, Nends, sizeof(struct end), comp_num);
		Ends[Nends].end_addr = Text_data->d_size;
	}
	else {
		block_end(Funcs[Nfuncs]->st_value, END_FALL_THROUGH, NULL, NULL);
		Nends--;
	}

	if (Data) {
		qsort(Data, Ndata, sizeof(Elf32_Addr), comp_num);
		Data[Ndata] = NO_SUCH_ADDR;
	}
	for (func = 0, data = 0, end = 0, addr = 0; addr < Funcs[Nfuncs]->st_value; addr++) {
		isdata = 0;
		if (!(addr % NBITPW)) {
			maskword = Starts[addr / NBITPW];
			mask = 1;
		}
		else
			mask <<= 1;
		if (Ndata && (addr == Data[data])) {
			while (addr == Data[++data])
				;
			if (!(mask & maskword)) {
				if ((end == Nends) || (Ends[end].end_addr <= addr)) {
					if (NoDataAllowed)
						exit(61);
					isdata = 1;
					if (Debug)
						printf("Found data at 0x%x\n", addr);
					if (!msg) {
						msg = 1;
						fprintf(stderr, "WARNING: Encountered possible data in the text section, continuing\n");
					}
				}
			}
		}
		else if (!(mask & maskword))
			continue;
		if (Nblocks >= Sblocks - 1) {
			if (!Sblocks)
				Sblocks = Nends + 2;
			else
				Sblocks += Sblocks / 10 + 2;
			Blocks_std = (struct block_std *) REALLOC(Blocks_std, Sblocks * sizeof(struct block_std));
			memset(Blocks_std + Nblocks, '\0', (Sblocks - Nblocks) * sizeof(struct block_std));
		}
		if (isdata)
			SET_FLAGS(Nblocks, DATA_BLOCK);
		SET_START_ADDR(Nblocks, addr);
		if (Nblocks > 0) {
			if (addr < Ends[end].end_addr) {
				SET_END_TYPE(Nblocks - 1, END_FALL_THROUGH);
				SET_JUMP_TARGET(Nblocks - 1, NO_SUCH_ADDR);
				SET_END_ADDR(Nblocks - 1, addr);
				SET_END_INSTLEN(Nblocks - 1, 0);
			}
			else {
				SET_END_INSTLEN(Nblocks - 1, Ends[end].end_instlen);
				SET_END_TYPE(Nblocks - 1, Ends[end].end_type);
				SET_JUMP_TARGET(Nblocks - 1, Ends[end].jump_addr);
				SET_END_ADDR(Nblocks - 1, Ends[end].end_addr);
				while (Ends[++end].end_addr == END_ADDR(Nblocks - 1))
					;
			}
			/*
			* Insert a block to fill the gap between blocks in the Force
			* case.  This should keep the original alignment intact
			*/
			if (Force && (addr > END_ADDR(Nblocks - 1))) {
				for ( ; Funcs[func]->st_value < END_ADDR(Nblocks - 1); func++)
					;
				if (Funcs[func]->st_value != addr) {
					Nblocks++;
					if (Nblocks >= Sblocks) {
						if (!Sblocks)
							Sblocks = Nends + 2;
						else
							Sblocks += Sblocks / 10 + 2;
						Blocks_std = (struct block_std *) REALLOC(Blocks_std, Sblocks * sizeof(struct block_std));
						memset(Blocks_std + Nblocks, '\0', (Sblocks - Nblocks) * sizeof(struct block_std));
					}
					Blocks_std[Nblocks] = Blocks_std[Nblocks - 1];
					SET_START_ADDR(Nblocks - 1, END_ADDR(Nblocks - 2));
					SET_END_TYPE(Nblocks - 1, END_FALL_THROUGH);
					SET_JUMP_TARGET(Nblocks - 1, NO_SUCH_ADDR);
					SET_END_ADDR(Nblocks - 1, addr);
					SET_END_INSTLEN(Nblocks - 1, 0);
				}
			}
		}
		Nblocks++;
	}
	if (end < Nends) {
		SET_END_INSTLEN(Nblocks - 1, Ends[end].end_instlen);
		SET_END_TYPE(Nblocks - 1, Ends[end].end_type);
		SET_JUMP_TARGET(Nblocks - 1, Ends[end].jump_addr);
		SET_END_ADDR(Nblocks - 1, Ends[end].end_addr);
	}
	else {
		SET_END_INSTLEN(Nblocks - 1, 0);
		SET_END_TYPE(Nblocks - 1, END_FALL_THROUGH);
		SET_JUMP_TARGET(Nblocks - 1, NO_SUCH_ADDR);
		SET_END_ADDR(Nblocks - 1, Text_data->d_size);
	}
	SET_END_ADDR(Nblocks, Funcs[Nfuncs]->st_value);
	SET_START_ADDR(Nblocks, Funcs[Nfuncs]->st_value);
	qsort(Nops, Nnops, sizeof(Elf32_Addr), comp_num);
	for (i = 0, j = 0; j < Nnops; j++) {
		while (START_ADDR(i) < Nops[j])
			i++;
		ADD_FLAGS(i, START_NOP);
	}
	if (Nops)
		free(Nops);
	Nnops = 0;
	if (Data) {
		free(Data);
		Data = NULL;
		Ndata = 0;
	}
}

/*
* Change the JUMP_TARGET() to be an index, not an address.  Make jumps
* to START_NOP go to the successor.
*/
static void
fixtargets()
{
	int i;

	for (i = 0; i < Nblocks; i++) {
		if (JUMP_TARGET(i) != NO_SUCH_ADDR) {
			SET_JUMP_TARGET(i, block_by_addr(JUMP_TARGET(i)));
			if (JUMP_TARGET(i) >= Nblocks)
				error("Untraceable jump to 0x%x in block %d\n", JUMP_TARGET(i), i);
		}
	}
	for (i = 0; i < Nblocks; i++) {
		if (END_TYPE(i) == END_IJUMP)
			continue;
		while ((JUMP_TARGET(i) != NO_SUCH_ADDR) && (FLAGS(JUMP_TARGET(i)) & START_NOP))
			JUMP_TARGET(i)++;
	}
}

static void
func_entry_point(char *name)
{
	int i, func;

	if ((func = find_name(name)) >= 0) {
		if (Pass2) {
			ADD_FLAGS(Func_info[func].start_block, ENTRY_POINT);
			Func_info[func].flags |= FUNC_ENTRY_POINT;
		}
		else
			block_start(Funcs[func]->st_value, 0);
		return;
	}
	for (i = 0; i < Nnonfuncs; i++)
		if (strcmp(NAME(Nonfuncs[i]), name) == 0) {
			if (Pass2)
				ADD_FLAGS(Nf_map[i], ENTRY_POINT);
			else
				block_start(Nonfuncs[i]->st_value, 0);
			return;
		}
	if (!No_warnings)
		fprintf(stderr, "Cannot find symbol or symbol is unused: %s\n", name);
}


static void
func_entry_points()
{
	FILE *fp;

	if (strncmp(LinkerOption, "export:", 7) == 0) {
		char buf[BUFSIZ];

		if (!(fp = fopen(LinkerOption+7, "r")))
			error("Cannot open %s\n", LinkerOption+7);
		while (fgets(buf, BUFSIZ, fp)) {
			buf[strlen(buf) - 1] = '\0';
			if (*buf == '#')
				continue;
			func_entry_point(buf);
		}
		fclose(fp);
	}
	else if (strncmp(LinkerOption, "export=", 7) == 0) {
		char *name, *buf;

		buf = MALLOC(strlen(LinkerOption) + 1);
		strcpy(buf, LinkerOption);
		for (name = strtok(buf + 7, ","); name; name = strtok(NULL, ","))
			func_entry_point(name);
		free(buf);
	}
}

/*
** Map symbols to block numbers
**
** While computing this, also compute the ENTRY_POINT and FUNCTION_START
** flags
*/
void
get_func_info()
{
	int i, j, k, dup_pend;
	int orig_nfuncs = Nfuncs;

	Func_info = REZALLOC(Func_info, (Nfuncs + 1) * sizeof(struct func_info));
	for (i = 0, j = 0; i < Nfuncs; i++) {
		for ( ; ; ) {
			/*
			** Notice that the test is < rather than !=, this is because the
			** symbol may point to a nop.  In such a case, we can just
			** use the next block
			*/
			while ((START_ADDR(j) < Funcs[i]->st_value) || (FLAGS(j) & START_NOP)) {
				SET_FUNCNO(j, i - 1);
				j++;
			}
			if ((START_ADDR(j) >= Funcs[i + 1]->st_value) && (Funcs[i]->st_value != Funcs[i + 1]->st_value)) {
				Funcs[i]->st_size = -1;
				Funcs[i]->st_value = NO_SUCH_ADDR;
				for (k = i; k < Nfuncs; k++) {
					Funcs[k] = Funcs[k + 1];
					Func_info[k] = Func_info[k + 1];
				}
				Nfuncs--;
				if (i == Nfuncs)
					break;
			}
			else
				break;
		}
		if (((ELF32_ST_BIND(Funcs[i]->st_info) == STB_GLOBAL) || (ELF32_ST_BIND(Funcs[i]->st_info) == STB_WEAK)) && !LinkerOption) {
			Func_info[i].flags |= FUNC_ENTRY_POINT;
			ADD_FLAGS(j, ENTRY_POINT);
		}
		ADD_FLAGS(j, FUNCTION_START);
		SET_FUNCNO(j, i);
		Func_info[i].start_block = j;
		for (k = i - 1; (k >= 0) && (Func_info[k].start_block == Func_info[i - 1].start_block); k--)
			Func_info[k].end_block = j;
	}
	for ( ; j < Nblocks; j++)
		SET_FUNCNO(j, i - 1);
	Func_info[i].start_block = NO_SUCH_ADDR;
	for (k = i - 1; (k >= 0) && (Func_info[k].start_block == Func_info[i - 1].start_block); k--)
		Func_info[k].end_block = Nblocks;
	Nf_map = REALLOC(Nf_map, Nnonfuncs * sizeof(ulong));
	for (i = 0, j = 0; i < Nnonfuncs; i++) {
		while (START_ADDR(j) < Nonfuncs[i]->st_value)
			j++;
		if (START_ADDR(j) != Nonfuncs[i]->st_value)
			Nf_map[i] = Nblocks;
		else {
			while (FLAGS(j) & START_NOP)
				j++;
			if (((ELF32_ST_BIND(Nonfuncs[i]->st_info) == STB_GLOBAL) || (ELF32_ST_BIND(Nonfuncs[i]->st_info) == STB_WEAK)) && !LinkerOption)
				ADD_FLAGS(j, ENTRY_POINT);
			Nf_map[i] = j;
		}
	}

	/* Since we may have deleted functions, the Names array could be
	* tainted
	*/
	if (orig_nfuncs != Nfuncs)
		sort_by_name();

	for (dup_pend = 0, i = 0; i < Nfuncs - 1; i++) {
		if (dup_pend)
			Func_info[Names[i]].flags |= DUP_NAME;
		dup_pend = strcmp(NAME(Funcs[Names[i]]), NAME(Funcs[Names[i + 1]])) == 0;
		if (dup_pend)
			Func_info[Names[i]].flags |= DUP_NAME;
		Func_info[i].group = NO_SUCH_ADDR;
	}
	Func_info[i].group = NO_SUCH_ADDR;
	if (dup_pend)
		Func_info[Names[i]].flags |= DUP_NAME;

	if (LinkerOption)
		func_entry_points();
}

#define GET_FLAGS(INDEX) (Ends ? 0 : FLAGS(INDEX))
#define GET_END_ADDR(INDEX) (Ends ? Ends[INDEX].end_addr : END_ADDR(INDEX))
#define GET_END_TYPE(INDEX) (Ends ? Ends[INDEX].end_type : END_TYPE(INDEX))
#define GET_END_INSTLEN(INDEX) (Ends ? Ends[INDEX].end_instlen : END_INSTLEN(INDEX))
#define GET_END_JUMP_ADDR(INDEX) (Ends ? Ends[INDEX].jump_addr : ((JUMP_TARGET(INDEX) == NO_SUCH_ADDR) ? NO_SUCH_ADDR : START_ADDR(JUMP_TARGET(INDEX))))
#define GET_END_BEENHERE(INDEX) (Ends ? Ends[INDEX].beenhere : (FLAGS(INDEX) & BEENHERE))
#define SET_END_BEENHERE(INDEX) (Ends ? (Ends[INDEX].beenhere = 1) : (FLAGS(INDEX) |= BEENHERE))
#define UNSET_END_BEENHERE(INDEX) (Ends ? (Ends[INDEX].beenhere = 0) : (FLAGS(INDEX) &= ~(BEENHERE)))
#define GET_START_ADDR(INDEX) (Ends ? decode_get_start_addr(INDEX) : START_ADDR(INDEX))
#define GET_FAR_START_ADDR(INDEX) (Ends ? decode_get_far_start_addr(INDEX) : START_ADDR(INDEX))
#define GET_NENDS() (Ends ? Nends : Nblocks)


static Elf32_Addr
decode_get_far_start_addr(int end)
{
	Elf32_Addr addr;

	for (addr = GET_END_ADDR(end-1); !BITMASKN_TEST1(Starts, addr); addr++)
		;
	return(addr);
}

static Elf32_Addr
decode_get_start_addr(int i)
{
	Elf32_Addr addr;

	for (addr = GET_END_ADDR(i) - GET_END_INSTLEN(i); !BITMASKN_TEST1(Starts, addr); addr--)
		;
	return(addr);
}

static int
jump_table_info(int end, int *pfunc, int *pfuncstart, int *pfuncend)
{
	int i, j, closest, partial_opcode;
	int func, funcstart, funcend, tablen;
	Elf32_Addr newtarget, target;
	Elf32_Addr addr, next_addr;

	func = *pfunc;
	for (j = 0; j < NDecodeJumpTables; j++)
		if (DecodeJumpTables[j].ijump_end_addr == GET_END_ADDR(end))
			return(0);
	for ( ; Funcs[func]->st_value < GET_END_ADDR(end); func++)
		;
	func--;

	for (funcstart = end; (funcstart > 0) && (GET_END_ADDR(funcstart) > Funcs[func]->st_value); funcstart--)
		;
	funcstart++;

	for (funcend = end; (funcend < GET_NENDS()) && (GET_END_ADDR(funcend) <= Funcs[func + 1]->st_value); funcend++)
		;

	/*
	** Find a protective conditional jump
	*/
	for (j = funcstart; j < funcend; j++)
		UNSET_END_BEENHERE(j);
	SET_END_BEENHERE(end);
	newtarget = GET_START_ADDR(end);
	do {
		if (newtarget == GET_START_ADDR(funcstart)) {
			j = Nblocks;
			break;
		}
		target = newtarget;
		closest = -1;
		for (j = funcstart; j < funcend; j++)
			if ((GET_END_ADDR(j) <= target) && ((closest < 0) || (GET_END_ADDR(closest) < GET_END_ADDR(j))))
				closest = j;
		switch (GET_END_TYPE(closest)) {
		case END_CJUMP:
			j = closest;
			continue; /* breaks the loop */
		case END_CALL:
		case END_PSEUDO_CALL:
		case END_FALL_THROUGH:
			break;
		default:
			closest = -1;
		}
		for (j = funcstart; j < funcend; j++)
			if ((GET_END_TYPE(j) == END_CJUMP) && (GET_END_JUMP_ADDR(j) == target))
				break;

		if (j < funcend)
			break;
		if (closest >= 0) {
			newtarget = GET_START_ADDR(closest);
			SET_END_BEENHERE(closest);
			continue;
		}
		for (j = funcstart; j < funcend; j++) {
			if (GET_END_BEENHERE(j))
				continue;
			switch (GET_END_TYPE(j)) {
			case END_JUMP:
				if (GET_END_JUMP_ADDR(j) != target)
					continue;
				newtarget = GET_START_ADDR(j);
				SET_END_BEENHERE(j);
				break;
			default:
				continue;
			}
			break;
		}
	} while (target != newtarget);
	for (i = funcstart; i < funcend; i++)
		UNSET_END_BEENHERE(i);
	if ((j >= funcend) || (GET_END_TYPE(j) != END_CJUMP)) {
		if (Debug)
			fprintf(stderr, "INFO: Indirect Jump Encountered in function %s%s%s\n", FULLNAME(Funcs[func_by_addr(GET_END_ADDR(end) - GET_END_INSTLEN(end))]));
		return(0);
	}
	if (GET_END_JUMP_ADDR(j) != target)
		partial_opcode = 0x7; /* ja */
	else
		partial_opcode = 0x6; /* jbe */

	if (GET_END_INSTLEN(j) == LONG_CJUMP_LEN) {
		if (GET1(Text_data->d_buf, GET_END_ADDR(j) - GET_END_INSTLEN(j) + 1) != (0x80 + partial_opcode)) {
			if (Debug)
				fprintf(stderr, "INFO: Indirect Jump Encountered in function %s%s%s\n", FULLNAME(Funcs[func_by_addr(GET_END_ADDR(end) - GET_END_INSTLEN(end))]));
			return(0);
		}
	}
	else {
		if (GET1(Text_data->d_buf, GET_END_ADDR(j) - GET_END_INSTLEN(j)) != (0x70 + partial_opcode)) {
			if (Debug)
				fprintf(stderr, "INFO: Indirect Jump Encountered in function %s%s%s\n", FULLNAME(Funcs[func_by_addr(GET_END_ADDR(end) - GET_END_INSTLEN(end))]));
			return(0);
		}
	}
	tablen = 0;
	i = j;
	/*
	* Strange code.  We are looking for the comparison statement that
	* determines the length of the jump table.  So, what we do is look
	* for the last block that might contain the comparison statement. 
	* This could be any block that might fall through to the
	* conditional jump that we are examining.
	*/
	while ((i > funcstart) && (GET_END_TYPE(i - 1) != END_RET) && (GET_END_TYPE(i - 1) != END_JUMP) && (GET_END_TYPE(i - 1) != END_IJUMP) && !(GET_FLAGS(i - 1) & DATA_BLOCK))
		i--;
	if (i == 0)
		addr = 0;
	else
		addr = GET_FAR_START_ADDR(i);
	do {
		Shdr.sh_addr = addr;
		p_data = (char *) Text_data->d_buf + addr;
		dis_text(&Shdr);
		next_addr = loc;
		if (GET1(Text_data->d_buf, addr) == 0x66)
			addr++;
		switch(GET1(Text_data->d_buf, addr)) {
		case 0x3c:
			tablen = GET1(Text_data->d_buf, next_addr - 1) + 1;
			break;
		case 0x3d:
			switch (next_addr - addr - 1) {
			case 1:
				tablen = GET1(Text_data->d_buf, next_addr - 1) + 1;
				break;
			case 2:
				tablen = GET1(Text_data->d_buf, next_addr - 1) << 8;
				tablen += GET1(Text_data->d_buf, next_addr - 2) + 1;
				break;
			case 4:
				tablen = GET4(Text_data->d_buf, next_addr - 4) + 1;
				break;
			}
			break;
		case 0x80:
		case 0x83:
		case 0x81:
			if (stripop(GET1(Text_data->d_buf, addr + 1)) == 7) {
				switch (next_addr - addr - 2) {
				case 1:
					tablen = GET1(Text_data->d_buf, next_addr - 1) + 1;
					break;
				case 2:
					tablen = GET1(Text_data->d_buf, next_addr - 1) << 8;
					tablen += GET1(Text_data->d_buf, next_addr - 2) + 1;
					break;
				case 4:
					tablen = GET4(Text_data->d_buf, next_addr - 4) + 1;
					break;
				}
			}
			break;
		}
		addr = next_addr;
	} while (addr < GET_END_ADDR(j) - GET_END_INSTLEN(j));
	if (!tablen) {
		if (!No_warnings)
			fprintf(stderr, "Warning: can't interpret protective test for jump table\n");
		tablen = NO_SUCH_ADDR;
	}
	*pfuncstart = funcstart;
	*pfuncend = funcend;
	*pfunc = func;
	return(tablen);
}

/*
* PIC code presents a particular problem for jump tables: it uses
* relative offsets (from the PC that was computed during the function
* prologue) rather than full addresses.  To deal with this, we make a
* pass through the relocation tables looking for GOT relocations against
* the read-only data section.  If we see that these relocations lead to
* indirect jumps, then we suspect that we are looking at a jump table.
* From there, we assume that there must be a protective test to make
* sure that we don't use too large an index into the jump table.  This
* gives us the length of the jump table.  Each entry in this table is
* put in the PICJumpTables data structure.
*/
static void
decode_pic_jump_tables()
{
	int end, i, pseudo, funcstart, funcend, tablen;
	int func;
	int ncr;
	Elf32_Addr funcstart_addr;
	Elf32_Rel *rel;
	Elf32_Addr jump_addr, tabstart;

	if (Nends) {
		qsort(Ends, Nends, sizeof(struct end), comp_num);
		Ends[Nends].end_addr = Text_data->d_size;
	}
	else {
		block_end(Funcs[Nfuncs]->st_value, END_FALL_THROUGH, NULL, NULL);
		Nends--;
	}
	for (func = 0, rel = Textrel, end = 0; end < Nends; end++) {
		if (Ends[end].end_type != END_IJUMP)
			continue;
		tablen = jump_table_info(end, &func, &funcstart, &funcend);
		if (!tablen)
			continue;
		if (rel < Textrel)
			rel = Textrel;
		for ( ; rel < Endtextrel; rel++)
			if (rel->r_offset > Ends[end].end_addr)
				break;
		for (rel--; rel >= Textrel; rel--)
			if (ELF32_R_TYPE(rel->r_info) == R_386_GOTOFF)
				break;
		if ((rel < Textrel) || ((end > 0) && (rel->r_offset < Ends[end - 1].end_addr)))
			continue;
		ncr = ELF32_R_SYM(rel->r_info) == Text_sym;
		if (!ncr) {
			for (pseudo = funcstart; pseudo < funcend; pseudo++)
				if (Ends[pseudo].end_type == END_PSEUDO_CALL)
					break;
			if (pseudo == funcend)
				continue;
		}
		if (Debug)
			printf("Pic Jump table at 0x%x\n", Ends[end].end_addr - 2);
		if (!ncr && !esections[Rodata_sec].sec_data)
			esections[Rodata_sec].sec_data = myelf_getdata(esections[Rodata_sec].sec_scn, 0, "read_only data section\n");
		tabstart = GET4(Text_data->d_buf, rel->r_offset);
		funcstart_addr = decode_get_start_addr(funcstart);
		if (!(NDecodeJumpTables % 100))
			DecodeJumpTables = REALLOC(DecodeJumpTables, (NDecodeJumpTables + 100) * sizeof(struct jumptable));
		DecodeJumpTables[NDecodeJumpTables].ijump_end_addr = Ends[end].end_addr;
		DecodeJumpTables[NDecodeJumpTables].tabstart = tabstart;
		DecodeJumpTables[NDecodeJumpTables].base_address = ncr ? tabstart : Ends[pseudo].end_addr;
		if (tablen != NO_SUCH_ADDR) {
			DecodeJumpTables[NDecodeJumpTables].dsts = MALLOC(tablen * sizeof(Elf32_Addr));
			DecodeJumpTables[NDecodeJumpTables].tablen = tablen;
		}
		else
			DecodeJumpTables[NDecodeJumpTables].dsts = NULL;
		for (i = 0; i < tablen; i++) {
			if (ncr)
				jump_addr = tabstart + GET4(Text_data->d_buf, tabstart + (i * sizeof(Elf32_Addr)));
			else
				jump_addr = Ends[pseudo].end_addr + GET4(esections[Rodata_sec].sec_data->d_buf, tabstart + (i * sizeof(Elf32_Addr)));
			if ((funcstart_addr > jump_addr) || (Ends[funcend].end_addr <= jump_addr)) {
				if (tablen != NO_SUCH_ADDR) {
					if (!No_warnings)
						fprintf(stderr, "Warning: Encountered interprocedural jump table\n");
				}
				else
					break;
			}
			block_start(jump_addr, 0);
			if ((tablen == NO_SUCH_ADDR) && !(i % 5))
				DecodeJumpTables[NDecodeJumpTables].dsts = REALLOC(DecodeJumpTables[NDecodeJumpTables].dsts, (i + 100) * sizeof(Elf32_Addr));

			DecodeJumpTables[NDecodeJumpTables].dsts[i] = jump_addr;
		}
		NDecodeJumpTables++;
	}
}

static int
comp_ijump(const void *v1, const void *v2)
{
	return(((struct jumptable *) v1)->ijump_end_addr - ((struct jumptable *) v2)->ijump_end_addr);
}

void
find_jump_tables()
{
	int i, j, funcstart, funcend, tablen;
	int warned;
	int pseudo;
	int pic, ncr;
	int curdecode;
	int func;
	int altfunc;
	Elf32_Rel *rel;
	Elf32_Addr jump_addr, tabstart;

	if (NDecodeJumpTables)
		qsort(DecodeJumpTables, NDecodeJumpTables, sizeof(struct jumptable), comp_ijump);
	for (curdecode = 0, func = 0, rel = Textrel, i = 0; i < Nblocks; i++) {
		if (END_TYPE(i) != END_IJUMP)
			continue;
		tablen = jump_table_info(i, &func, &funcstart, &funcend);
		if (!tablen)
			continue;
		while ((rel < Endtextrel) && (rel->r_offset < END_ADDR(i)))
			rel++;
		rel--;
		if (pic = (rel->r_offset < (END_ADDR(i) - END_INSTLEN(i)))) {
			for ( ; rel >= Textrel; rel--)
				if (ELF32_R_TYPE(rel->r_info) == R_386_GOTOFF)
					break;
			if (rel < Textrel)
				continue;
			if (rel->r_offset < START_ADDR(i)) {
				if ((i < 2) || !(FLAGS(i - 1) & START_NOP))
					continue;
				if (rel->r_offset < START_ADDR(i - 2))
					continue;
			}
			ncr = ELF32_R_SYM(rel->r_info) == Text_sym;
			if (!ncr) {
				for (pseudo = funcstart; pseudo < funcend; pseudo++)
					if (END_TYPE(pseudo) == END_PSEUDO_CALL)
						break;
				if (pseudo == funcend)
					continue;
			}
			if (Debug)
				printf("Pic Jump table at 0x%x\n", END_ADDR(i) - 2);
			if (!ncr && !esections[Rodata_sec].sec_data)
				esections[Rodata_sec].sec_data = myelf_getdata(esections[Rodata_sec].sec_scn, 0, "read_only data section\n");
		}
		else if (!esections[Rodata_sec].sec_data)
			esections[Rodata_sec].sec_data = myelf_getdata(esections[Rodata_sec].sec_scn, 0, "read_only data section\n");
		tabstart = GET4(Text_data->d_buf, rel->r_offset);
		while ((curdecode < NDecodeJumpTables) && (END_ADDR(i) < DecodeJumpTables[curdecode].ijump_end_addr)) {
			if (!(NJumpTables % 100))
				JumpTables = REALLOC(JumpTables, (NJumpTables + 100) * sizeof(struct jumptable));
			JumpTables[NJumpTables++] = DecodeJumpTables[curdecode++];
		}
		if (!(NJumpTables % 100))
			JumpTables = REALLOC(JumpTables, (NJumpTables + 100) * sizeof(struct jumptable));
		JumpTables[NJumpTables].ijump_end_addr = END_ADDR(i);
		JumpTables[NJumpTables].base_address = pic ? (ncr ? tabstart : END_ADDR(pseudo)) : NO_SUCH_ADDR;
		JumpTables[NJumpTables].tabstart = tabstart;
		if (tablen != NO_SUCH_ADDR) {
			JumpTables[NJumpTables].dsts = MALLOC(tablen * sizeof(Elf32_Addr));
			JumpTables[NJumpTables].tablen = tablen;
		}
		else {
			JumpTables[NJumpTables].dsts = NULL;
			JumpTables[NJumpTables].tablen = 0;
		}
/*		refsec = ELF32_SYM(rel->r_info);*/
/*		if (!esections[refsec].sec_data)*/
/*			esections[refsec].sec_data = myelf_getdata(esections[refsec].sec_scn, 0, "relocation section");*/
		for (warned = 0, j = 0; j < tablen; j++) {
			if (pic) {
				if (ncr)
					jump_addr = tabstart + GET4(Text_data->d_buf, tabstart + (j * sizeof(Elf32_Addr)));
				else
					jump_addr = END_ADDR(pseudo) + GET4(esections[Rodata_sec].sec_data->d_buf, tabstart + (j * sizeof(Elf32_Addr)));
			}
			else
				jump_addr = GET4(esections[Rodata_sec].sec_data->d_buf, tabstart + (j * sizeof(Elf32_Addr)));
			if ((START_ADDR(funcstart) > jump_addr) || (END_ADDR(funcend) <= jump_addr)) {
				if (tablen != NO_SUCH_ADDR) {
					if ((j == 0) && (tablen != 1))
						altfunc = FUNCNO(block_by_addr(jump_addr));
					else {
						if (!No_warnings && !warned && ((tablen == 1) || (altfunc != FUNCNO(block_by_addr(jump_addr))))) {
							fprintf(stderr, "Warning: Encountered interprocedural jump table at address 0x%x\n", JumpTables[NJumpTables].ijump_end_addr);
							warned = 1;
						}
					}
				}
				else
					break;
			}
			if ((tablen != NO_SUCH_ADDR) && !(j % 10))
				JumpTables[NJumpTables].dsts = REALLOC(JumpTables[NJumpTables].dsts, (j + 10) * sizeof(Elf32_Addr));

			JumpTables[NJumpTables].dsts[j] = jump_addr;
		}
		JumpTables[NJumpTables].tablen = j;
		NJumpTables++;
	}
	while (curdecode < NDecodeJumpTables) {
		if (!(NJumpTables % 100))
			JumpTables = REALLOC(JumpTables, (NJumpTables + 100) * sizeof(struct jumptable));
		JumpTables[NJumpTables++] = DecodeJumpTables[curdecode++];
	}
	free(DecodeJumpTables);
	NDecodeJumpTables = 0;
	DecodeJumpTables = NULL;
	for (i = 0; i < NJumpTables; i++)
		for (j = 0; j < JumpTables[i].tablen; j++)
			JumpTables[i].dsts[j] = block_by_addr(JumpTables[i].dsts[j]);
}

/*
** Scan up to a given address for more relocation entries
*/
static Elf32_Addr
findmore(Elf32_Rel **prel, Elf32_Rel *end, caddr_t sec_data, Elf32_Addr nextaddr, int inst_is_jump)
{
	Elf32_Rel *rel = *prel;
	Elf32_Addr refto;

	for ( ; rel < end; rel++) {
		if (rel->r_offset + sizeof(Elf32_Addr) > nextaddr) {
			refto = NO_SUCH_ADDR;
			break;
		}
		if ((refto = get_refto(rel, sec_data, NULL, Symstart)) != NO_SUCH_ADDR) {
			Elf32_Sym *sym = Symstart + ELF32_R_SYM(rel->r_info) - 1;

			if ((sec_data != Text_data->d_buf) || (Funcistext && (ELF32_ST_TYPE(sym->st_info) == STT_FUNC) && (refto == sym->st_value))) {
				if (refto < Text_data->d_size)
					block_start(refto, ENTRY_POINT);
			}
			else if (inst_is_jump && (rel->r_offset + sizeof(Elf32_Addr) == nextaddr))
				block_start(refto, 0);
			else {
				int i;

				for (i = 0; i < Ndata; i++)
					if (Data[i] == refto)
						break;
				if (i >= Ndata) {
					if (!(Ndata % 100))
						Data = (Elf32_Addr *) REALLOC(Data, (Ndata + 101) * sizeof(Elf32_Addr));
					Data[Ndata++] = refto;
				}
			}
		}
		if (rel->r_offset + sizeof(Elf32_Addr) == nextaddr)
			break;
	}
	*prel = rel;
	return((rel < end) ? refto : NO_SUCH_ADDR);
}

/*
** Test whether the statement is one of the designated No-op's from the
** Noptable
*/
static int
isnop(Elf32_Addr addr, int len)
{
	unchar buf[14];
	int i;

	if (len > 14)
		error("Problem decoding at addr 0x%x\n", addr);
	for (i = 0; i < len; i++)
		buf[i] = GET1(Text_data->d_buf, addr + i);
	for (i = 0; i < Noptable[len].len; i += len) {
		if ((buf[0] == Noptable[len].nops[i]) && (memcmp(buf, Noptable[len].nops + i, len) == 0))
			return(1);
	}
	return(0);
}

static void
groupinfo(int i)
{
	Elf32_Addr addr, next_addr;
	Elf32_Rel *rel = NULL;
	ulong inst, inst2;

	Groupinfo = REALLOC(Groupinfo, (Ngroups + 1) * sizeof(struct groupinfo));
	Groupinfo[Ngroups].block = i;
	for (rel = Textrel; rel < Endtextrel; rel++)
		if (rel->r_offset >= START_ADDR(i))
			break;
	for (addr = START_ADDR(i); addr < END_ADDR(i); addr = next_addr) {
		Shdr.sh_addr = addr;
		p_data = (char *) Text_data->d_buf + addr;
		dis_text(&Shdr);
		next_addr = loc;
		inst = GET1(Text_data->d_buf, addr);
		inst2 = GET1(Text_data->d_buf, addr + 1);
		if (issub(inst, inst2)) {
			while ((rel < Endtextrel) && (rel->r_offset < addr))
				rel++;
			if ((rel >= Endtextrel) && !(rel->r_offset < next_addr))
				continue;
			if ((ELF32_R_SYM(rel->r_info) + Symstart - 1)->st_shndx == Text_index) {
				Groupinfo[Ngroups].rel = rel;
				Groupinfo[Ngroups].addr = addr;
			}
		}
	}
	Ngroups++;
}

/*
** Read in Blocks_std data structure from file
*/
int
getblocks()
{
	int fd;
	int i;
	caddr_t p;
	struct stat stat;

	if ((fd = open(Keepblocks, O_RDWR)) < 0)
		error("Cannot open %s\n", Keepblocks);

	if (fstat(fd, &stat) < 0)
		error("Cannot stat %s\n", Keepblocks);

	if ((p = mmap(NULL, stat.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0)) == (caddr_t) -1)
		error("Cannot mmap %s\n", Keepblocks);

	Blocks_std = MALLOC(stat.st_size);
	Ndecodeblocks = Nblocks = stat.st_size / sizeof(struct block_std) - 1;
	Sblocks = Nblocks + 1;
	memcpy(Blocks_std, p, stat.st_size);
	munmap(p, stat.st_size);
	if (START_ADDR(Nblocks) != Text_data->d_size) {
		free(Blocks_std);
		Sblocks = Nblocks = 0;
		return(0);
	}
	for (i = 0; i < Nblocks; i++) {
		if (START_ADDR(i) >= START_ADDR(i + 1)) {
			free(Blocks_std);
			Sblocks = Nblocks = 0;
			Groupinfo = NULL;
			Ngroups = 0;
			return(0);
		}
		if (FLAGS(i) & ARITH_ON_ADDRESS)
			groupinfo(i);
	}
	return(1);
}

static int
jumptable_map(int sec, struct jumptable ***ptable)
{
	int i;
	struct jumptable **table;
	int len = 0;

	if (sec != Rodata_sec)
		return(0);
	table = (struct jumptable **) MALLOC(NJumpTables * sizeof(struct jumptable *));
	for (i = 0; i < NJumpTables; i++) {
		if (JumpTables[i].base_address == JumpTables[i].tabstart)
			continue;
		table[len++] = JumpTables + i;
	}
	if (!len) {
		free(table);
		table = NULL;
	}
	*ptable = table;
	return(len);
}

void
data_entry_points()
{
	Elf32_Rel *rel, *endrel;
	int i, len;
	struct jumptable **table;

	/* Check for relocatable references to blocks in other
	** relocation sections
	*/
	for (i = 1; i < (int) Ehdr->e_shnum; i++) {
		int rtype = esections[i].sec_shdr->sh_type;
		int refsec;

		if (rtype != SHT_REL && rtype != SHT_RELA)
			continue;

		if (esections[i].sec_shdr->sh_info == Text_index) {
			if (Pass2) {
				int j;
				int inst_is_jump;

				rel = Textrel;
				for (j = 0; j < Nblocks; j++) {
					while ((rel < Endtextrel) && (rel->r_offset < START_ADDR(j)))
						rel++;
					if (rel == Endtextrel)
						break;
					switch(END_TYPE(j)) {
					case END_JUMP:
					case END_CALL:
					case END_PSEUDO_CALL:
					case END_CJUMP:
						inst_is_jump = 1;
						break;
					default:
						inst_is_jump = 0;
					}
					findmore(&rel, Endtextrel, Text_data->d_buf, END_ADDR(j), inst_is_jump);
				}
			}
			continue;
		}

		if (!esections[i].sec_data)
			esections[i].sec_data = myelf_getdata(esections[i].sec_scn, 0, "relocation section");

		refsec = esections[i].sec_shdr->sh_info;

		if (!esections[refsec].sec_data)
			esections[refsec].sec_data = myelf_getdata(esections[refsec].sec_scn, 0, "relocation section");
		rel = (Elf32_Rel *) esections[i].sec_data->d_buf;
		endrel = rel + esections[i].sec_data->d_size / sizeof(Elf32_Rel);
		/*
		* Block out parts of sections that correspond to jump tables. 
		* Since we understand them, we don't have to treat them like entry
		* points.
		*/
		len = jumptable_map(refsec, &table);

		if (len) {
			Elf32_Addr curaddr, endaddr = MAXADDR;
			int j;

			for (curaddr = 0, j = 0; j < len; j++) {
				endaddr = table[j]->tabstart;
				findmore(&rel, endrel, esections[refsec].sec_data->d_buf, endaddr, 0);
				curaddr = endaddr + sizeof(Elf32_Addr) * table[j]->tablen;
				while ((rel < endrel) && (rel->r_offset < curaddr))
					rel++;
				if (rel >= endrel)
					break;
			}
		}
		findmore(&rel, endrel, esections[refsec].sec_data->d_buf, MAXADDR, 0);
	}
}

/*
* Read the elf file to scan for blocks.

* Use the disassembler to get the length of each instruction.
* Determine if the instruction designates the end of a block (i.e.  a
* return statement, a jump or a call).  Also, read the relocations
* along the way to determine where the jumps go.

* If a series of nops is recognized, separate them into a separate
* block so they can be left out of the final code.

* Then, read the other relocation sections: any reference into the text
* section designates the beginning of a block.

* Then, look for text symbols.  Any text symbol refers to the beginning
* of a block.

* Last, if the user wants the blocks stored, write them to the file.
*/
void
genblocks()
{
	char *p;
	int nop_pending, nop;
	ulong inst, inst2;
	int uncond_jump;
	Elf32_Rel *rel = NULL;
	char buf[PATH_MAX];
	Elf32_Addr next_addr, addr, offset, refto;
	int notjump;
	unsigned int 	i;

	/* These two may have been set by a failed getblocks() */
	Nblocks = 0;
	Ngroups = 0;
	Starts = MALLOC(BITMASK_NWORDS(Text_data->d_size) * sizeof(uint_t));
	BITMASKN_CLRALL(Starts, BITMASK_NWORDS(Text_data->d_size));

	data_entry_points();
	if (LinkerOption)
		func_entry_points();
	else {
		for (i = 0; i < Nfuncs; i++)
			if ((ELF32_ST_BIND(Funcs[i]->st_info) == STB_GLOBAL) || (ELF32_ST_BIND(Funcs[i]->st_info) == STB_WEAK))
				block_start(CURADDR(Funcs[i]), 0);

		for (i = 0; i < Nnonfuncs; i++)
			if (((ELF32_ST_BIND(Nonfuncs[i]->st_info) == STB_GLOBAL) || (ELF32_ST_BIND(Nonfuncs[i]->st_info) == STB_WEAK)) && (ELF32_ST_TYPE(Nonfuncs[i]->st_info) == STT_NOTYPE))
				block_start(CURADDR(Nonfuncs[i]), 0);
	}

	if (!Nstack)
		return;
	while (Nstack) {
		next_addr = 0;
		buf[0] = '\0';
		nop_pending = 0;
		while ((addr = pop_addr()) != NO_SUCH_ADDR) {
			if (Debug && getenv("PUSHDEBUG"))
				printf("Processing address 0x%x\n", addr);
			if (addr != next_addr) {
				nop_pending = 0;
				rel = NULL;
			}
			inst = GET1(Text_data->d_buf, addr);
			inst2 = GET1(Text_data->d_buf, addr + 1);
			Shdr.sh_addr = addr;
			p_data = (char *) Text_data->d_buf + addr;
			dis_text(&Shdr);
			next_addr = loc;
			if (Checkinstructions) {
				if (!(Nsizes % 1024))
					Sizes = REALLOC(Sizes, Nsizes + 1024);
				Sizes[Nsizes++] = next_addr - addr;
			}
			if (!rel) {
				Elf32_Rel *high = Endtextrel, *low = Textrel;

				do {
					rel = low + (high - low) / 2;
					if (addr > rel->r_offset)
						low = rel + 1;
					else
						high = rel;
				} while (high > low);
				if (addr > rel->r_offset)
					rel++;
			}
			else
				while ((rel < Endtextrel) && (rel->r_offset < addr))
					rel++;
			nop = 0;
			uncond_jump = 0;
			refto = NO_SUCH_ADDR;
			notjump = 0;
			if (iscjump(inst, inst2) || (uncond_jump = isjump(inst, inst2)) || (notjump = iscall(inst, inst2))) {
				if (!uncond_jump)
					block_start(next_addr, 0);
				/* Save the function call if we know the offset is past */
				if ((rel < Endtextrel) && (rel->r_offset < next_addr))
					refto = findmore(&rel, Endtextrel, Text_data->d_buf, next_addr, 1);
				if ((rel < Endtextrel) && (rel->r_offset < next_addr))
					block_end(next_addr, notjump ? END_CALL : uncond_jump ? END_JUMP : END_CJUMP, refto, next_addr - addr);
				else {
					if (islonginst(inst, inst2))
						offset = GET4(Text_data->d_buf, next_addr - sizeof(Elf32_Addr));
					else {
						offset = GET1(Text_data->d_buf, next_addr - sizeof(unchar));
						if (offset >= 128)
							offset = offset - 256;
					}
					block_end(next_addr, notjump ? END_CALL : uncond_jump ? END_JUMP : END_CJUMP, next_addr + offset, next_addr - addr);
					block_start(next_addr + offset, 0);
				}
			}
			else {
				if ((rel < Endtextrel) && (rel->r_offset < next_addr)) {
					if (issub(inst, inst2) && (ELF32_R_SYM(rel->r_info) + Symstart - 1)->st_shndx == Text_index) {
						Groupinfo = REALLOC(Groupinfo, (Ngroups + 1) * sizeof(struct groupinfo));
						Groupinfo[Ngroups].rel = rel;
						Groupinfo[Ngroups].addr = addr;
						Ngroups++;
/*						printf("Found arithmetic on a text address at 0x%x\n", addr);*/
					}
					refto = findmore(&rel, Endtextrel, Text_data->d_buf, next_addr, 0);
				}
				if (isret(inst, inst2)) {
					block_end(next_addr, END_RET, NO_SUCH_ADDR, next_addr - addr);
				}
				else if (isicall(inst, inst2)) {
					block_end(next_addr, END_CALL, NO_SUCH_ADDR, next_addr - addr);
					block_start(next_addr, 0);
				}
				else if (isijump(inst, inst2)) {
					block_end(next_addr, END_IJUMP, NO_SUCH_ADDR, next_addr - addr);
				}
				else if (iscallgate(inst, inst2))
					block_start(next_addr, 0);
				else if (/* TestPushPop && */ isimmpush(inst, inst2) && (refto != NO_SUCH_ADDR)) {
					block_end(next_addr, END_PUSH, refto, next_addr - addr);
					block_start(next_addr, 0);
					block_start(refto, 0);
				}
				else if (TestPushPop && TESTPOP(inst, inst2, addr)) {
					block_end(next_addr, END_POP, NULL, next_addr - addr);
					block_start(next_addr, 0);
				}
				else {
					if (isnop(addr, next_addr - addr) && !((rel < Endtextrel) && (rel->r_offset < next_addr)))
						nop = 1;
					if (!nop || (next_addr < Text_data->d_size))
						push_addr(next_addr);
				}
			}
			if (nop_pending != nop) {
				block_start(addr, nop ? START_NOP : 0);
				/* A little odd here:  we don't want to process
				* the same address twice in a row (it screws up 
				* nop_pending calculations).  So, check to see
				* if block_start forced a push and, if so, remove
				* it
				*/
				if (Nstack && (Stack[Nstack - 1] == addr))
					Nstack--;
			}
			nop_pending = nop;
		}
		if (Found_pseudo)
			decode_pic_jump_tables();
	}
	free(Stack);
	Stack = NULL;
	Nstack = 0;
	Sstack = 0;

	mergeblocks();

	free(Starts);
	Starts = NULL;
	free(Ends);
	Ends = NULL;
	Nends = 0;
	Sends = 0;

	Ndecodeblocks = Nblocks;

	fixtargets();
	for (i = 0; i < Ngroups; i++) {
		Groupinfo[i].block = block_by_addr(Groupinfo[i].addr);
		ADD_FLAGS(Groupinfo[i].block, ARITH_ON_ADDRESS);
	}
/*	checkdata();*/
	if (Keepblocks) {
		int fd;
		char *end;
		int len;

		if ((fd = open(Keepblocks, O_CREAT|O_RDWR|O_TRUNC, 0666)) < 0)
			error("Cannot open %s\n", Keepblocks);
		p = (char *) Blocks_std;
		end = p + (Nblocks + 1) * sizeof(struct block_std);
		for ( ; p < end; p += len) {
			if ((len = write(fd, p, end - p)) < 0)
				error("Cannot write to %s\n", Keepblocks);
		}
		close(fd);
	}
/*	for (i = 0; i < Nblocks; i++)*/
/*		printf("Start 0x%x, End 0x%x, end_type %d\n", START_ADDR(i), END_ADDR(i), END_TYPE(i));*/
}

/* Find the relocations for each block */
void
find_block_reloc()
{
	Elf32_Rel *walk;
	int i;
	static int beenhere;

	if (beenhere)
		return;
	beenhere = 1;
	for (walk = Textrel, i = 0; i < Nblocks; i++) {
		if (walk >= Endtextrel) {
			SET_REL(i, Endtextrel);
			SET_ENDREL(i, Endtextrel);
			continue;
		}
		while ((walk < Endtextrel) && (walk->r_offset < START_ADDR(i)))
			walk++;
		SET_REL(i, walk);
		while ((walk < Endtextrel) && (walk->r_offset < END_ADDR(i)))
			walk++;
		SET_ENDREL(i, walk);
	}
}

/*
* Because of the presence of "nasty" assembly code, fur has to do a
* little detective work to find what functions need to stay together. 
* This happens when arithmetic is being performed on a text address.
*/
void
find_func_groups()
{
	int i, j;

	for (i = 0; i < Ngroups; i++) {
		Groupinfo[i].start = Groupinfo[i].end = FUNCNO(block_by_addr(get_refto(Groupinfo[i].rel, Text_data->d_buf, NULL, Symstart)));
		Groupinfo[i].end = Groupinfo[i].start + 1;
		for (j = 0; j < Nblocks; j++) {
			if ((END_TYPE(j) == END_CALL) && (JUMP_TARGET(j) != NO_SUCH_ADDR) && (FUNCNO(JUMP_TARGET(j)) == FUNCNO(Groupinfo[i].block)) ||
				((END_TYPE(j) == END_PSEUDO_CALL) && (FUNCNO(j) == FUNCNO(Groupinfo[i].block)))) {
				if (FUNCNO(j) < Groupinfo[i].start)
					Groupinfo[i].start = FUNCNO(j);
				else if (FUNCNO(j) >= Groupinfo[i].end)
					Groupinfo[i].end = FUNCNO(j) + 1;
			}
		}
/*		printf("Function group starts at 0x%x and ends at 0x%x\n", START_ADDR(Func_info[Groupinfo[i].start].start_block), START_ADDR(Func_info[Groupinfo[i].end].start_block));*/
		for (j = Groupinfo[i].start; j < Groupinfo[i].end; j++) {
			Func_info[j].flags |= FUNC_GROUPED;
			Func_info[j].group = i;
		}
	}
}

/*
* Look for functions that are referenced by an eh structure.  Mark each
* function to say that the order should remain unchanged by fur.
*/
void
eh_decode()
{
	Elf32_Rel *rel;
	Elf32_Rel *endrel;
	Elf32_Addr addr;

	rel = (Elf32_Rel *) (Eh_reldata->d_buf);
	endrel = (Elf32_Rel *) ((Elf32_Addr) Eh_reldata->d_buf + Eh_reldata->d_size);
	for ( ; rel < endrel; rel++)
		if ((addr = get_refto(rel, Eh_data->d_buf, NULL, Symstart)) != NO_SUCH_ADDR)
			Func_info[func_by_addr(addr)].flags |= FUNC_SAVE_ORDER;
	/*
	* We decided that we will not decode the ranges table for the
	* purpose of recognizing new blocks.  This decision may need to be
	* revisited at a later point.
	*/
}
