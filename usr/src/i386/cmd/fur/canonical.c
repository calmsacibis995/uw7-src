#ident	"@(#)fur:i386/cmd/fur/canonical.c	1.5"
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

#define START_ADDR1(BLOCK) (Blocks_std1[(BLOCK)].start_addr)
#define FUNCNO1(BLOCK) (Blocks_std1[(BLOCK)].funcno)
#define START_SYM1(BLOCK) (Funcs[Blocks_std1[(BLOCK)].funcno])
#define END_TYPE1(BLOCK) (Blocks_std1[(BLOCK)].end_type)
#define END_INSTLEN1(BLOCK) (Blocks_std1[(BLOCK)].end_instlen)
#define FLAGS1(BLOCK) (Blocks_std1[(BLOCK)].flags)
#define JUMP_TARGET1(BLOCK) (Blocks_std1[(BLOCK)].jump_target)
#define FALLTHROUGH1(BLOCK) (Blocks_change1[BLOCK].fallthrough_target)
#define END_ADDR1(BLOCK) (Blocks_std1[(BLOCK)].end_addr)
#define IS_FUNCTION_START1(BLOCK) (FLAGS1(BLOCK) & FUNCTION_START)
#define CODELEN1(BLOCK) (END_ADDR1(BLOCK) - START_ADDR1(BLOCK) - END_INSTLEN1(BLOCK))
#define NAME1(SYM) (((char *) Str_data1->d_buf) + (SYM)->st_name)
#define FNAME1(SYM) (Fnames1[(SYM)-Symstart1] ? Fnames1[(SYM)-Symstart1] : "")
#define FULLNAME1(SYM) FNAME1(SYM), (Fnames1[(SYM)-Symstart1] ? "@" : ""), NAME1(SYM)
#define JUMP_TYPE1(BLOCK) (((GET1(Text_data1->d_buf, END_ADDR1(BLOCK) - END_INSTLEN1(BLOCK)) == LONGCODE_PREFIX) ? GET1(Text_data1->d_buf, END_ADDR1(BLOCK) - END_INSTLEN1(BLOCK) + 1) : GET1(Text_data1->d_buf, END_ADDR1(BLOCK) - END_INSTLEN1(BLOCK))) & 0xf)

#define START_ADDR2(BLOCK) (Blocks_std2[(BLOCK)].start_addr)
#define FUNCNO2(BLOCK) (Blocks_std2[(BLOCK)].funcno)
#define START_SYM2(BLOCK) (Funcs[Blocks_std2[(BLOCK)].funcno])
#define END_TYPE2(BLOCK) (Blocks_std2[(BLOCK)].end_type)
#define END_INSTLEN2(BLOCK) (Blocks_std2[(BLOCK)].end_instlen)
#define FLAGS2(BLOCK) (Blocks_std2[(BLOCK)].flags)
#define JUMP_TARGET2(BLOCK) (Blocks_std2[(BLOCK)].jump_target)
#define FALLTHROUGH2(BLOCK) (Blocks_change2[BLOCK].fallthrough_target)
#define END_ADDR2(BLOCK) (Blocks_std2[(BLOCK)].end_addr)
#define IS_FUNCTION_START2(BLOCK) (FLAGS2(BLOCK) & FUNCTION_START)
#define CODELEN2(BLOCK) (END_ADDR2(BLOCK) - START_ADDR2(BLOCK) - END_INSTLEN2(BLOCK))
#define NAME2(SYM) (((char *) Str_data2->d_buf) + (SYM)->st_name)
#define FNAME2(SYM) (Fnames2[(SYM)-Symstart2] ? Fnames2[(SYM)-Symstart2] : "")
#define FULLNAME2(SYM) FNAME2(SYM), (Fnames2[(SYM)-Symstart2] ? "@" : ""), NAME2(SYM)
#define JUMP_TYPE2(BLOCK) (((GET1(Text_data2->d_buf, END_ADDR2(BLOCK) - END_INSTLEN2(BLOCK)) == LONGCODE_PREFIX) ? GET1(Text_data2->d_buf, END_ADDR2(BLOCK) - END_INSTLEN2(BLOCK) + 1) : GET1(Text_data2->d_buf, END_ADDR2(BLOCK) - END_INSTLEN2(BLOCK))) & 0xf)

static struct jumptable *JumpTables1;
static int NJumpTables1;
static Elf_Data 	*Text_data1;
static ulong		*Names1;
static char			**Fnames1;
static Elf32_Sym	*Symstart1;
static Elf32_Sym	**Funcs1;
static struct func_info *Func_info1;
static int			Nfuncs1;
static ulong		*Nf_map1;
static Elf32_Sym	**Nonfuncs1;
static int			Nnonfuncs1;
static Elf_Data		*Str_data1;
static struct block_std			*Blocks_std1;
static struct block_change		*Blocks_change1;
static int			Nblocks1;
static int			Ndecodeblocks1;
static int			Sblocks1;
static struct section *esections1;
static Elf32_Rel	*Textrel1;
static Elf32_Rel	*Endtextrel1;
static int	 		Text_index1;

static struct jumptable *JumpTables2;
static int NJumpTables2;
static Elf_Data 	*Text_data2;
static ulong		*Names2;
static char			**Fnames2;
static Elf32_Sym	*Symstart2;
static Elf32_Sym	**Funcs2;
static struct func_info *Func_info2;
static int			Nfuncs2;
static ulong		*Nf_map2;
static Elf32_Sym	**Nonfuncs2;
static int			Nnonfuncs2;
static Elf_Data		*Str_data2;
static struct block_std			*Blocks_std2;
static struct block_change		*Blocks_change2;
static int			Nblocks2;
static int			Ndecodeblocks2;
static int			Sblocks2;
static struct section *esections2;
static Elf32_Rel	*Textrel2;
static Elf32_Rel	*Endtextrel2;
static int	 		Text_index2;

#undef START_ADDR
#undef FUNCNO
#undef START_SYM
#undef END_TYPE
#undef END_INSTLEN
#undef FLAGS
#undef JUMP_TARGET
#undef END_ADDR
#undef IS_FUNCTION_START
#undef NAME
#undef FNAME
#undef FULLNAME
#undef JUMP_TYPE
#undef CODELEN

void
clearglob()
{
	Found_pseudo = 0;
	Ndata = 0;
	Pass2 = 0;
	JumpTables = NULL;
	NJumpTables = 0;
	Nsizes = 0;
	Textrel = NULL;
	Endtextrel = NULL;
	Eh_data = NULL;
	Eh_reldata = NULL;
	Text_data = NULL;
	Text_index = -1;
	Ehdr = NULL;
	Names = NULL;
	Fnames = NULL;
	Symstart = NULL;
	Sym_data = NULL;
	Funcs = NULL;
	Nfuncs = 0;
	Func_info = NULL;
	Nf_map = NULL;
	Nonfuncs = NULL;
	Nnonfuncs = 0;
	Blocks_std = NULL;
	Blocks_insertion = NULL;
	Blocks_change = NULL;
	Blocks_stats = NULL;
	Nblocks = 0;
	Sblocks = 0;
	Ends = NULL;
	Nends = 0;
	Nops = NULL;
	Nnops = 0;
	esections = NULL;
}

int
store1()
{
	if (Blocks_std1)
		return(0);
	JumpTables1 = JumpTables;
	NJumpTables1 = NJumpTables;
	Text_data1 = Text_data;
	Names1 = Names;
	Fnames1 = Fnames;
	Symstart1 = Symstart;
	Funcs1 = Funcs;
	Func_info1 = Func_info;
	Nfuncs1 = Nfuncs;
	Nf_map1 = Nf_map;
	Nonfuncs1 = Nonfuncs;
	Nnonfuncs1 = Nnonfuncs;
	Str_data1 = Str_data;
	Blocks_std1 = Blocks_std;
	Blocks_change1 = Blocks_change;
	Nblocks1 = Nblocks;
	Ndecodeblocks1 = Ndecodeblocks;
	Sblocks1 = Sblocks;
	esections1 = esections;
	Textrel1 = Textrel;
	Endtextrel1 = Endtextrel;
	Text_index1 = Text_index;

	clearglob();
	return(1);
}

void
store2()
{
	JumpTables2 = JumpTables;
	NJumpTables2 = NJumpTables;
	Text_data2 = Text_data;
	Names2 = Names;
	Fnames2 = Fnames;
	Symstart2 = Symstart;
	Funcs2 = Funcs;
	Func_info2 = Func_info;
	Nfuncs2 = Nfuncs;
	Nf_map2 = Nf_map;
	Nonfuncs2 = Nonfuncs;
	Nnonfuncs2 = Nnonfuncs;
	Str_data2 = Str_data;
	Blocks_std2 = Blocks_std;
	Blocks_change2 = Blocks_change;
	Nblocks2 = Nblocks;
	Ndecodeblocks2 = Ndecodeblocks;
	Sblocks2 = Sblocks;
	esections2 = esections;
	Textrel2 = Textrel;
	Endtextrel2 = Endtextrel;
	Text_index2 = Text_index;

	clearglob();
}

static int
get_succs1(int block1, Elf32_Addr **psuccs)
{
	int i;

	for (i = 0; i < NJumpTables1; i++)
		if (JumpTables1[i].ijump_end_addr == END_ADDR1(block1)) {
			*psuccs = JumpTables1[i].dsts;
			return(JumpTables1[i].tablen);
		}
	return(0);
}

static int
get_succs2(int block2, Elf32_Addr **psuccs)
{
	int i;

	for (i = 0; i < NJumpTables2; i++)
		if (JumpTables2[i].ijump_end_addr == END_ADDR2(block2)) {
			*psuccs = JumpTables2[i].dsts;
			return(JumpTables2[i].tablen);
		}
	return(0);
}

struct eq {
	int block1;
	int block2;
};

static struct eq *Hashtable;

static int
find_hash(int block1, int block2)
{
	ulong i, starti;

	starti = i = ((ulong) ((block1 * block2) + block1 + block2)) % (Nblocks2 * 2);
	if (!Hashtable) {
		Hashtable = (struct eq *) MALLOC(Nblocks2 * 2 * sizeof(struct eq));
		memset(Hashtable, 0xff, Nblocks2 * 2 * sizeof(struct eq));
	}
	for ( ; ; ) {
		if ((Hashtable[i].block1 == block1) && (Hashtable[i].block2 == block2))
			return(i);
		if (Hashtable[i].block1 == NO_SUCH_ADDR)
			return(i);
		i = (i + 1) % (Nblocks2 * 2);
		if (starti == i)
			error("Hash table blown\n");
	}
}

static int
check_equal(int block1, int block2)
{
	int i = find_hash(block1, block2);

	return((Hashtable[i].block1 == block1) && (Hashtable[i].block2 == block2));
}

static void
mark_equal(int block1, int block2)
{
	int i = find_hash(block1, block2);

	if ((Hashtable[i].block1 == block1) && (Hashtable[i].block2 == block2))
		error("oops\n");
	Hashtable[i].block1 = block1;
	Hashtable[i].block2 = block2;
	return;
}

#define COMPARE(BLOCK1, BLOCK2) do_compare(BLOCK1, BLOCK2)

static void block_compare(int block1, int block2);

static void
do_compare(int block1, int block2)
{
	if ((block1 == NO_SUCH_ADDR) && (block2 != NO_SUCH_ADDR))
		error("Block in 1 is undefined and Block %d is not\n", block2);
	if ((block1 != NO_SUCH_ADDR) && (block2 == NO_SUCH_ADDR))
		error("Block %d is not undefined and Block in 2 is\n", block1);
	if (!check_equal(block1, block2)) {
		mark_equal(block1, block2);
		if (Debug)
			fprintf(stderr, "Comparing 0x%x and 0x%x\n", START_ADDR1(block1), START_ADDR2(block2));
		block_compare(block1, block2);
	}
}

static void
comp_bodies(int *pblock1, int *pblock2)
{
	int len1, len2;
	int block1 = *pblock1, block2 = *pblock2;
	int off1 = 0, off2 = 0;
	int len;

	len1 = CODELEN1(block1);
	len2 = CODELEN2(block2);
	if (!(len1 || len2))
		return;
	for ( ; ; ) {
		len = min(len1, len2);
		if (memcmp((unchar *) Text_data1->d_buf + START_ADDR1(block1) + off1, (unchar *) Text_data2->d_buf + START_ADDR2(block2) + off2, len) != 0) {
			if ((END_TYPE1(block1) == END_IJUMP) && (END_TYPE2(block2) == END_IJUMP))
				fprintf(stderr, "IJUMP: Block at 0x%x does not match Block at 0x%x\n", START_ADDR1(block1), START_ADDR2(block2));
			else
				fprintf(stderr, "Block at 0x%x does not match Block at 0x%x\n", START_ADDR1(block1), START_ADDR2(block2));
/*				error("Block at 0x%x does not match Block at 0x%x\n", START_ADDR1(block1), START_ADDR2(block2));*/
			return;
		}
		len1 -= len;
		len2 -= len;
		off1 += len;
		off2 += len;
		while (!len1) {
			if ((END_TYPE1(block1) == END_FALL_THROUGH) && (FALLTHROUGH1(block1) != NO_SUCH_ADDR))
				block1 = FALLTHROUGH1(block1);
			else if ((END_TYPE1(block1) == END_JUMP) && (block1 != JUMP_TARGET1(block1)) && (JUMP_TARGET1(block1) != NO_SUCH_ADDR))
				block1 = JUMP_TARGET1(block1);
			else
				break;
			off1 = 0;
			len1 = CODELEN1(block1);
		}
		while (!len2) {
			if ((END_TYPE2(block2) == END_FALL_THROUGH) && (FALLTHROUGH2(block2) != NO_SUCH_ADDR))
				block2 = FALLTHROUGH2(block2);
			else if ((END_TYPE2(block2) == END_JUMP) && (block2 != JUMP_TARGET2(block2)) && (JUMP_TARGET2(block2) != NO_SUCH_ADDR))
				block2 = JUMP_TARGET2(block2);
			else
				break;
			off2 = 0;
			len2 = CODELEN2(block2);
		}
		if (!len1 && !len2)
			break;
		if ((!len1 && len2) || (len1 && !len2))
			error("Block at 0x%x does not match Block at 0x%x\n", START_ADDR1(block1), START_ADDR2(block2));
	}
	*pblock1 = block1;
	*pblock2 = block2;
}

static void
block_compare(int block1, int block2)
{
	comp_bodies(&block1, &block2);
	if (END_TYPE1(block1) != END_TYPE2(block2)) {
		if ((END_TYPE1(block1) == END_RET) && (END_TYPE2(block2) == END_POP))
			return;
		if ((END_TYPE1(block1) == END_POP) && (END_TYPE2(block2) == END_RET))
			return;
		if ((END_TYPE1(block1) == END_CALL) && (END_TYPE2(block2) == END_PUSH)) {
			COMPARE(FALLTHROUGH1(block1), JUMP_TARGET2(block2));
			COMPARE(JUMP_TARGET1(block1), FALLTHROUGH2(block2));
			return;
		}
		if ((END_TYPE1(block1) == END_PUSH) && (END_TYPE2(block2) == END_CALL)) {
			COMPARE(JUMP_TARGET1(block1), FALLTHROUGH2(block2));
			COMPARE(FALLTHROUGH1(block1), JUMP_TARGET2(block2));
			return;
		}
		if ((END_TYPE1(block1) == END_FALL_THROUGH) && (END_TYPE2(block2) == END_JUMP)) {
			COMPARE(FALLTHROUGH1(block1), JUMP_TARGET2(block2));
			return;
		}
		if ((END_TYPE1(block1) == END_JUMP) && (END_TYPE2(block2) == END_FALL_THROUGH)) {
			COMPARE(JUMP_TARGET1(block1), FALLTHROUGH2(block2));
			return;
		}
		error("Block at 0x%x has a different END_TYPE than does Block at 0x%x\n", START_ADDR1(block1), START_ADDR2(block2));
	}
	switch(END_TYPE1(block1)) {
	case END_CJUMP:
		{
			int jump_type1 = JUMP_TYPE1(block1);
			int jump_type2 = JUMP_TYPE2(block2);

			if (jump_type1 != jump_type2) {
				if (jump_type1 == REVERSE(jump_type2)) {
					COMPARE(FALLTHROUGH1(block1), JUMP_TARGET2(block2));
					COMPARE(JUMP_TARGET1(block1), FALLTHROUGH2(block2));
				}
				else
					error("Block at 0x%x has a different conditional jump than does Block at 0x%x\n", START_ADDR1(block1), START_ADDR2(block2));
			}
			else  {
				COMPARE(FALLTHROUGH1(block1), FALLTHROUGH2(block2));
				COMPARE(JUMP_TARGET1(block1), JUMP_TARGET2(block2));
			}
			break;
		}
	case END_PUSH:
	case END_CALL:
		COMPARE(FALLTHROUGH1(block1), FALLTHROUGH2(block2));
		COMPARE(JUMP_TARGET1(block1), JUMP_TARGET2(block2));
		break;
	case END_JUMP:
		COMPARE(JUMP_TARGET1(block1), JUMP_TARGET2(block2));
		break;
	case END_FALL_THROUGH:
	case END_PSEUDO_CALL:
	case END_POP:
		COMPARE(FALLTHROUGH1(block1), FALLTHROUGH2(block2));
		break;
	case END_RET:
		break;
	case END_IJUMP:
		{
			Elf32_Addr *succs1;
			int nsuccs1;
			Elf32_Addr *succs2;
			int nsuccs2;
			int j;

			nsuccs1 = get_succs1(block1, &succs1);
			nsuccs2 = get_succs2(block2, &succs2);
			if (nsuccs1 != nsuccs2)
				error("Block at 0x%x has a different number of successors than does Block at 0x%x\n", START_ADDR1(block1), START_ADDR2(block2));
			for (j = 0; j < nsuccs1; j++)
				COMPARE(succs1[j], succs2[j]);
		}
	}
}

/*
* Data references in the text section must be "scrubbed" - the addresses
* referred to by them must be cleared so that they are not considered
* mismatches.
*/
static void
scrub_data_ref()
{
	Elf32_Rel *rel;
	Elf32_Addr refto;

	Text_index = Text_index1;
	Ndecodeblocks = Ndecodeblocks1;
	Blocks_std = Blocks_std1;
	for (rel = Textrel1; rel < Endtextrel1; rel++)
		if ((refto = get_refto(rel, Text_data1->d_buf, NULL, Symstart1)) != NO_SUCH_ADDR) {
/*			i = blockstart_by_addr(refto);*/
/*			if (!(FLAGS1(i) & DATA_BLOCK))*/
/*				TODO: Mark this as needing comparison */
			PUT4(Text_data1->d_buf, rel->r_offset, 0xffeeffee);
		}

	Text_index = Text_index2;
	Ndecodeblocks = Ndecodeblocks2;
	Blocks_std = Blocks_std2;
	for (rel = Textrel2; rel < Endtextrel2; rel++)
		if ((refto = get_refto(rel, Text_data2->d_buf, NULL, Symstart2)) != NO_SUCH_ADDR) {
/*			i = blockstart_by_addr(refto);*/
/*			if (FLAGS2(i) & DATA_BLOCK)*/
/*				TODO: Mark this as needing comparison */
			PUT4(Text_data2->d_buf, rel->r_offset, 0xffeeffee);
		}

	Text_index = -1;
	Ndecodeblocks = 0;
	Blocks_std = NULL;
}

void
compare_files()
{
	int i1, i2;
	Elf32_Sym *func1, *func2;

	scrub_data_ref();
	for (i1 = 0, i2 = 0; (i1 < Nfuncs1) && (i2 < Nfuncs2); i1++, i2++) {
		while (i1 < Nfuncs1) {
			if (Func_info1[Names1[i1]].flags & FUNC_ENTRY_POINT)
				break;
			i1++;
		}
		while (i2 < Nfuncs2) {
			if (Func_info2[Names2[i2]].flags & FUNC_ENTRY_POINT)
				break;
			i2++;
		}
		if ((i1 >= Nfuncs1) || (i2 >= Nfuncs2)) {
			if (i1 < Nfuncs1)
				error("More Functions left in file 1\n");
			if (i2 < Nfuncs2)
				error("More Functions left in file 2\n");
			return;
		}
		func1 = Funcs1[Names1[i1]];
		func2 = Funcs2[Names2[i2]];
		if ((strcmp(NAME1(func1), NAME2(func2)) != 0) || (strcmp(FNAME1(func1), FNAME2(func2)) != 0))
			error("Functions %s%s%s and %s%s%s are not the same\n", FULLNAME1(func1), FULLNAME2(func2));
		COMPARE(Func_info1[Names1[i1]].start_block, Func_info2[Names2[i2]].start_block);
	}
}
