#ident	"@(#)fur:i386/cmd/fur/gencode.c	1.1.2.7"
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
/*#include "uw_eh.h"*/

/*
** Map block addresses back to the symbol table
*/
void
fixup_symbol_table()
{
	int i, j;

	/* First, update the functions in the symbol table */
	for (i = 0; i < Nfuncs; i++) {
		if (Func_info[i].order_start == NO_SUCH_ADDR) {
			if ((i > 0) && (Func_info[i-1].start_block == Func_info[i].start_block)) {
				Funcs[i]->st_value = Funcs[i-1]->st_value;
				Func_info[i] = Func_info[i-1];
			}
			else {
				Funcs[i]->st_value = NO_SUCH_ADDR;
				Funcs[i]->st_size = -1;
			}
		}
		else if (Canonical && strstr(NAME(Funcs[i]), ".low.usage")) {
			Funcs[i]->st_value = NO_SUCH_ADDR;
			Funcs[i]->st_size = -1;
		}
		else if (Canonical && !Funcs[i]->st_value && !Funcs[i]->st_size)
			Funcs[i]->st_value = 0;
		else
			Funcs[i]->st_value = NEW_START_ADDR(Order[Func_info[i].order_start]);
	}
	for (i = 0; i < Nfuncs; i++) {
		if (Funcs[i]->st_size != -1) {
			for (j = Func_info[i].order_start + 1; (j < Norder) && !(IS_FUNCTION_START(Order[j])); j++)
				;
			Funcs[i]->st_size = NEW_END_ADDR(Order[j-1]) - NEW_START_ADDR(Order[Func_info[i].order_start]);
		}
	}
	/* Then, fix the other symbols */
	for (i = 0; i < Nnonfuncs; i++)
		Nonfuncs[i]->st_value = NEW_START_ADDR(Nf_map[i]);
}

void
cleanup_symbol_table()
{
	int *delarray;
	int beenhere = 0;
	int i;
	int symno;
	int fill;
	int nsyms = Sym_data->d_size / sizeof(Elf32_Sym);
	Elf32_Sym *symstart;
	Elf32_Sym *newsymstart;
	char *oldbuf;
	char *newstr;
	char *walkstr;
	int lastlocal;

	delarray = CALLOC(nsyms, sizeof(int));
	walkstr = newstr = MALLOC(Str_data->d_size + strlen(".rel.text") + 1);
	symstart = (Elf32_Sym *) Sym_data->d_buf;
	if (!Textrel)
		newsymstart = (Elf32_Sym *) MALLOC(Sym_data->d_size + sizeof(Elf32_Sym));
	else
		newsymstart = symstart;
	for (fill = i = 0; i < nsyms; i++) {
		if ((symstart[i].st_size == -1) && (ELF32_ST_TYPE(symstart[i].st_info) == STT_FUNC)) {
			delarray[i] = -1;
			continue;
		}
		if (ELF32_ST_BIND(symstart[i].st_info) == STB_LOCAL)
			lastlocal = fill;
		else if (!beenhere && !Textrel) {
			beenhere = 1;
			lastlocal++;
			newsymstart[fill].st_value = 0;
			newsymstart[fill].st_size = 0;
			newsymstart[fill].st_info = ELF32_ST_INFO(STB_LOCAL, STT_SECTION);
			newsymstart[fill].st_other = 0;
			newsymstart[fill].st_shndx = Ehdr->e_shnum - 1;
			fill++;
		}
		if ((symstart != newsymstart) || (i != fill))
			newsymstart[fill] = symstart[i];
		if (ELF32_ST_TYPE(symstart[i].st_info) != STT_SECTION) {
			strcpy(walkstr, NAME(newsymstart + fill));
			newsymstart[fill].st_name = walkstr - newstr;
			walkstr += strlen(walkstr) + 1;
		}
		delarray[i] = fill++;
	}
	esections[Sym_index].sec_shdr->sh_info = lastlocal + 1;
	Sym_data->d_size = fill * sizeof(Elf32_Sym);
	Sym_data->d_buf = (void *) newsymstart;
	symstart = newsymstart;
	oldbuf = (char *) Str_data->d_buf;
	Str_data->d_buf = newstr;
	if (esections[Ehdr->e_shstrndx].sec_data == Str_data) {
		char *compname, *sname, *where;
		int j;

		/* Strangely enough, ld chooses to optimize the size of the
		* string table by finding substrings that are workable section
		* names.  If fur did not replicate this functionality, the
		* string table would grow due to fur which would cause even
		* more code complexity.
		*/
		for (i = 1; i < (int) Ehdr->e_shnum; i++) {
			if (esections[i].sec_shdr->sh_name < 0)
				sname = ".rel.text";
			else
				sname = oldbuf + esections[i].sec_shdr->sh_name;
			for (j = 1; j < i; j++) {
				if (esections[j].foundname)
					continue;
				if (esections[j].sec_shdr->sh_name < 0)
					compname = ".rel.text";
				else
					compname = ((char *) Str_data->d_buf) + esections[j].sec_shdr->sh_name;
				if ((where = strstr(compname, sname)) && (strcmp(where, sname) == 0))
					break;
			}
			for (j = i + 1; j < (int) Ehdr->e_shnum; j++) {
				if (esections[j].sec_shdr->sh_name < 0)
					compname = ".rel.text";
				else
					compname = oldbuf + esections[j].sec_shdr->sh_name;
				if ((where = strstr(compname, sname)) && (strcmp(where, sname) == 0))
					break;
			}
			if (j < (int) Ehdr->e_shnum)
				esections[i].foundname = j;
			else {
				esections[i].foundname = 0;
				strcpy(walkstr, sname);
				esections[i].sec_shdr->sh_name = walkstr - newstr;
				walkstr += strlen(walkstr) + 1;
			}
		}
		if (!Textrel) {
			strcpy(walkstr, ".rel.text");
			esections[Ehdr->e_shnum - 1].sec_shdr->sh_name = walkstr - newstr;
			walkstr += strlen(walkstr) + 1;
		}
		for (i = 1; i < (int) Ehdr->e_shnum; i++) {
			if (!esections[i].foundname)
				continue;
			if (esections[i].sec_shdr->sh_name < 0)
				sname = ".rel.text";
			else
				sname = oldbuf + esections[i].sec_shdr->sh_name;
			esections[i].sec_shdr->sh_name = strstr(((char *) Str_data->d_buf) + esections[esections[i].foundname].sec_shdr->sh_name, sname) - (char *) Str_data->d_buf;
		}
		for (i = 0; i < fill; i++) {
			if (ELF32_ST_TYPE(symstart[i].st_info) != STT_SECTION)
				continue;
			symstart[i].st_name = esections[symstart[i].st_shndx].sec_shdr->sh_name;
		}
	}
	Str_data->d_size = walkstr - newstr;
	for (i = 1; i < (int) Ehdr->e_shnum; i++) {
		int rtype = esections[i].sec_shdr->sh_type;
		Elf32_Rel *rel;
		int j, nrel;

		if (rtype != SHT_REL && rtype != SHT_RELA)
			continue;

		rel = (Elf32_Rel *) esections[i].sec_data->d_buf;
		nrel = esections[i].sec_data->d_size / sizeof(Elf32_Rel);
		for (j = 0; j < nrel; j++) {
			symno = ELF32_R_SYM(rel[j].r_info);
			if (delarray[symno] == symno)
				continue;
			rel[j].r_info = ELF32_R_INFO(delarray[symno], ELF32_R_TYPE(rel[j].r_info));
		}
	}
}

void
nopfill(unchar *buf, unchar *end)
{
	int len;

	while (buf != end) {
		len = min(end - buf, 14);
		while (!Noptable[len].len)
			len--;
		memcpy(buf, Noptable[len].nops, len);
		buf += len;
	}
}

/* Fill in the new text buffer */
void
fill_in_text()
{
	int i;
	int j;

	Newtext = REALLOC(Newtext, NEW_START_ADDR(Nblocks) + 34);
	for (j = 0; j < Norder; j++) {
		unchar *addr;
		int len;

		i = Order[j];
		addr = Newtext + NEW_START_ADDR(i);
		if (len = PRECODELEN(i)) {
			memcpy(addr, (char *) PRECODE(i), len);
			addr += len;
		}

		len = CODELEN(i);
		if (len < 0)
			error("Error encountered while disassembling object\nThis probably means that the object has data in its .text section\n");
		memcpy(addr, TEXTBUF(i), len);
		addr += len;

		if (len = POSTCODELEN(i)) {
			memcpy(addr, (char *) POSTCODE(i), len);
			addr += len;
		}

		if (NEW_END_INSTLEN(i)) {
			if (NEW_END_TYPE(i) != END_TYPE(i)) {
				switch (NEW_END_TYPE(i)) {
				case END_PUSH:
					memcpy(addr, "\150\0\0\0\0", 5);
					break;
				case END_POP:
					memcpy(addr, "\203\304\4", 5);
					break;
				case END_JUMP:
					addr[0] = JMPCODE;
					break;
				default:
					error("Internal error, exiting\n");
				}
			}
			else {
				len = min(END_INSTLEN(i), NEW_END_INSTLEN(i));
				memcpy(addr, TEXTBUF(i) + CODELEN(i), len);
			}
			addr += NEW_END_INSTLEN(i);
		}
		if (CORRECTIVE_JUMP_TARGET(i) != NO_SUCH_ADDR) {
			if (FLAGS(i) & CORRECTIVE_LONG_JUMP)
				addr += sizeof(Elf32_Addr) + 1;
			else
				addr += 1 + 1;
		}

		if (j + 1 < Norder)
			nopfill(addr, Newtext + NEW_START_ADDR(Order[j + 1]));
	}
}

static void
switchjump_to_short(unchar *text_addr)
{
	if (text_addr[0] == LONG_JMPCODE)
		text_addr[0] = JMPCODE;
	else
		text_addr[0] = 0x70 | (text_addr[1] & 0xf);
}

static void
switchjump_to_long(unchar *text_addr)
{
	if (text_addr[0] == JMPCODE)
		text_addr[0] = LONG_JMPCODE;
	else {
		text_addr[1] = 0x80 | (text_addr[0] & 0xf);
		text_addr[0] = LONGCODE_PREFIX;
	}
}

static void
fixup_one_reloc(Elf32_Rel *rel, caddr_t sec_data, Elf32_Addr refto_before)
{
	int high, middle, low;
	int ret;
	Elf32_Addr refto_now;

	if ((refto_now = get_refto(rel, sec_data, NULL, Symstart)) == NO_SUCH_ADDR)
		return;
	high = Ndecodeblocks;
	low = 0;
	while (high > low + 1) {
		middle = (high + low) / 2;
		if ((ret = refto_before - START_ADDR(middle)) == 0)
			break;
		else if (ret > 0)
			low = middle;
		else
			high = middle;
	};
	if (high > low + 1)
		low = middle;
	/*
	* Check to see if the relocation refers to a NOP block in between
	* two real blocks.  If it does, then assume that it points to the
	* next real block
	*/
	if (refto_before >= END_ADDR(low))
		ADD4(sec_data, rel->r_offset, (NEW_START_ADDR(low + 1) - refto_now));
	else {
		if (ORDER(low) == -1)
			ADD4(sec_data, rel->r_offset, (NEW_START_ADDR(JUMP_TARGET(low)) - START_ADDR(low)) - (refto_now - refto_before));
		else if (refto_before != START_ADDR(low))
			ADD4(sec_data, rel->r_offset, (CODEPOS(low) - START_ADDR(low)) - (refto_now - refto_before));
		else
			ADD4(sec_data, rel->r_offset, (NEW_START_ADDR(low) - START_ADDR(low)) - (refto_now - refto_before));
	}
}

inscode_rel(int block, Elf32_Rel *rel, Elf32_Addr offset)
{
	Elf32_Sym *newsym;

	if (Nrel == Srel) {
		Srel += Srel / 10 + 1;
		Rel = REALLOC(Rel, Srel * sizeof(Elf32_Rel));
	}
	Rel[Nrel] = rel[0];
	Rel[Nrel].r_offset += offset;
	if (ELF32_R_SYM(rel->r_info) == BLOCKNO_SYMNO)
		PUT4(Newtext, Rel[Nrel].r_offset, Flowblocks ? Flowblockno[block] : block);
	else if (ELF32_R_SYM(rel->r_info) == FUNCNO_SYMNO)
		PUT4(Newtext, Rel[Nrel].r_offset, FUNCNO(block));
	else {
		newsym = Symstart + ELF32_R_SYM(rel->r_info) - 1;
		if (newsym->st_size && (newsym->st_shndx == SHN_UNDEF)) {
			int x;
			Elf32_Sym *end_type_sym = newsym;

			newsym += newsym->st_size;
			/* There may have been symbols deleted, so find the
			* beginning of the list
			*/
			while ((newsym->st_size && (newsym->st_shndx == SHN_UNDEF)) && (newsym > end_type_sym))
				newsym--;
			newsym++;
			newsym += (END_TYPE(block) == END_CJUMP) ? (END_TYPE(block) + JCC_OP(block)) : (END_TYPE(block) + (N_JCC - 1));
			Rel[Nrel++].r_info = ELF32_R_INFO(newsym - Symstart + 1, ELF32_R_TYPE(rel->r_info));
		}
		else
			Nrel++;
	}
}

/* Fix up the jump instructions and the relocations */
void
fixup_jumps()
{
	Elf32_Addr refto;
	int i, j;
	Elf32_Rel *walk, *lastrel, *endinstrel;

	Nrel = 0;
	Srel = Endtextrel - Textrel;
	Rel = REALLOC(Rel, Srel * sizeof(Elf32_Rel));
	for (j = 0; j < Norder; j++) {
		i = Order[j];
		lastrel = NULL;
		if (PRECODELEN(i))
			for (walk = (Elf32_Rel *) PRERELSEC(i); walk < (Elf32_Rel *) ((Elf32_Addr) PRERELSEC(i) + PRERELSIZE(i)); walk++)
				inscode_rel(i, walk, PREPOS(i));

		for (walk = REL(i); (walk < ENDREL(i)) && (walk->r_offset < ORIGENDPOS(i)); walk++, Nrel++) {
			if (Nrel == Srel) {
				Srel += Srel / 10 + 1;
				Rel = REALLOC(Rel, Srel * sizeof(Elf32_Rel));
			}
			Rel[Nrel] = walk[0];
			Rel[Nrel].r_offset = CODEPOS(i) + (Rel[Nrel].r_offset - START_ADDR(i));
			switch (ELF32_R_TYPE(Rel[Nrel].r_info)) {
			case R_386_GOTPC:
				{
					int k;
					
					for (k = i - 1; !IS_FUNCTION_START(k); k--)
						if (END_TYPE(k) == END_PSEUDO_CALL)
							break;

					PUT4(Newtext, Rel[Nrel].r_offset, Rel[Nrel].r_offset - CORRPOS(k));
				}
/*				ADD4(Newtext, Rel[Nrel].r_offset, PRECODELEN(i));*/
/*				if (CORRECTIVE_JUMP_TARGET(i) != NO_SUCH_ADDR)*/
/*					ADD4(Newtext, Rel[Nrel].r_offset, NEW_START_ADDR(i) - CORRPOS(i - 1));*/
				break;
			case R_386_GOTOFF:
				if (GET4(Newtext, Rel[Nrel].r_offset) != 0) {
					if ((refto = get_refto(Rel + Nrel, (caddr_t) Newtext, NULL, Origsymtab + 1)) != NO_SUCH_ADDR)
						fixup_one_reloc(Rel + Nrel, (caddr_t) Newtext, refto);
					break;
				}
			default:
				if (Origsymtab[ELF32_R_SYM(walk->r_info)].st_shndx == Text_index)
					if ((refto = get_refto(Rel + Nrel, (caddr_t) Newtext, NULL, Origsymtab + 1)) != NO_SUCH_ADDR)
						fixup_one_reloc(Rel + Nrel, (caddr_t) Newtext, refto);
			}
		}
		endinstrel = walk;
		if (POSTCODELEN(i))
			for (walk = (Elf32_Rel *) POSTRELSEC(i); walk < (Elf32_Rel *) ((Elf32_Addr) POSTRELSEC(i) + POSTRELSIZE(i)); walk++)
				inscode_rel(i, walk, POSTPOS(i));

		/* If we got rid of the jump, don't look at the relocation */
		if ((END_TYPE(i) == NEW_END_TYPE(i)) && (END_INSTLEN(i) == NEW_END_INSTLEN(i))) {
			Elf32_Rel *rel;

			for (rel = endinstrel; rel < ENDREL(i); rel++) {
				if (Nrel == Srel) {
					Srel += Srel / 10 + 1;
					Rel = REALLOC(Rel, Srel * sizeof(Elf32_Rel));
				}
				Rel[Nrel] = rel[0];
				lastrel = Rel + Nrel;
				Rel[Nrel].r_offset = ENDPOS(i) + (Rel[Nrel].r_offset - ORIGENDPOS(i));
				if (ELF32_R_TYPE(Rel[Nrel].r_info) == R_386_GOTPC) {
					ADD4(Newtext, Rel[Nrel].r_offset, PRECODELEN(i));
					if (CORRECTIVE_JUMP_TARGET(i) != NO_SUCH_ADDR)
						ADD4(Newtext, Rel[Nrel].r_offset, NEW_START_ADDR(i) - CORRPOS(i - 1));
				}
				Nrel++;
			}
		}
		if (FLAGS(i) & REVERSE_END_CJUMP) {
			if (Newtext[ENDPOS(i)] == LONGCODE_PREFIX)
				REVERSE(Newtext[ENDPOS(i) + 1]);
			else
				REVERSE(Newtext[ENDPOS(i)]);
		}
		if (CORRECTIVE_JUMP_TARGET(i) != NO_SUCH_ADDR) {
			if (FLAGS(i) & CORRECTIVE_LONG_JUMP) {
				PUT1(Newtext, CORRPOS(i), LONG_JMPCODE);
				PUT4(Newtext, CORRPOS(i) + 1, PREPOS(CORRECTIVE_JUMP_TARGET(i)) - NEW_END_ADDR(i));
			}
			else {
				PUT1(Newtext, CORRPOS(i), JMPCODE);
				PUT1(Newtext, CORRPOS(i) + 1, PREPOS(CORRECTIVE_JUMP_TARGET(i)) - NEW_END_ADDR(i));
			}
		}
		if (NEW_END_TYPE(i) == END_PUSH) {
			if ((Nrel > 0) && (Rel[Nrel - 1].r_offset == ENDPOS(i) + 1))
				Nrel--;
			else if (Nrel == Srel) {
				Srel += Srel / 10 + 1;
				Rel = REALLOC(Rel, Srel * sizeof(Elf32_Rel));
			}
			Rel[Nrel].r_offset = ENDPOS(i) + 1;
			Rel[Nrel].r_info = ELF32_R_INFO(Text_sym, R_386_32);
			PUT4(Newtext, ENDPOS(i) + 1, PREPOS(JUMP_TARGET(i)));
			Nrel++;
		}
		if (JUMP_TARGET(i) == NO_SUCH_ADDR)
			continue;
		/* Always generate the op code in the future */

		if (NEW_END_TYPE(i) == END_PSEUDO_CALL) {
			PUT4(Newtext, CORRPOS(i) - sizeof(Elf32_Addr), 0);
			if (lastrel)
				Nrel--;
			continue;
		}
		if ((NEW_END_TYPE(i) != END_CJUMP) && (NEW_END_TYPE(i) != END_CALL) && (NEW_END_TYPE(i) != END_JUMP))
			continue;
		if (NEW_END_INSTLEN(i) > sizeof(Elf32_Addr)) {
			if (!lastrel) {
				if (FLAGS(i) & JUMP_GROWN)
					switchjump_to_long(Newtext + ENDPOS(i));
				PUT4(Newtext, CORRPOS(i) - sizeof(Elf32_Addr), PREPOS(JUMP_TARGET(i)) - CORRPOS(i));
			}
			else {
				if (ELF32_R_TYPE(lastrel->r_info) != R_386_PLT32) {
					PUT4(Newtext, CORRPOS(i) - sizeof(Elf32_Addr), PREPOS(JUMP_TARGET(i)) - CORRPOS(i));
					Nrel--;
				}
			}
		}
		else {
			int x;

			if (FLAGS(i) & JUMP_SHRUNK) {
				switchjump_to_short(Newtext + ENDPOS(i));
				if ((Nrel > 0) && (Rel[Nrel - 1].r_offset <= NEW_END_ADDR(i)) && (Rel[Nrel - 1].r_offset >= ENDPOS(i)))
					Nrel--;
			}
/*			if ((PREPOS(JUMP_TARGET(i)) - CORRPOS(i) >= 128) || (PREPOS(JUMP_TARGET(i)) - CORRPOS(i) < -128))*/
			x = PREPOS(JUMP_TARGET(i)) - CORRPOS(i);
			if ((x >= 128) || (x < -128))
				fprintf(stderr, "Jump is too far for a short jump\n");
			PUT1(Newtext, CORRPOS(i) - sizeof(char), x);
		}
	}
}

/* Fix-up the entries that correspond to PIC jump tables */
void
fixup_pic_jump_tables()
{
	int i, j, dst, base;

	for (i = 0; i < NJumpTables; i++) {
		if (JumpTables[i].base_address == NO_SUCH_ADDR)
			continue;
		for (j = 0; j < JumpTables[i].tablen; j++) {
			base = block_by_addr(JumpTables[i].base_address);
			if (base >= Ndecodeblocks)
				error("Internal error, cannot find jump table\n");
			if ((base > 0) && !(FLAGS(base) & DATA_BLOCK) && (END_ADDR(base - 1) == JumpTables[i].base_address))
				base--;
			dst = JumpTables[i].dsts[j];
			while (FLAGS(dst) & START_NOP)
				dst++;
			if (FLAGS(base) & DATA_BLOCK)
				PUT4(Newtext, NEW_START_ADDR(base) + (j * sizeof(Elf32_Addr)), PREPOS(dst) - NEW_START_ADDR(base));
			else
				PUT4(esections[Rodata_sec].sec_data->d_buf, JumpTables[i].tabstart + (j * sizeof(Elf32_Addr)), PREPOS(dst) - CORRPOS(base));
		}
	}
}

/* Fix up relocations in other sections */
void
fixup_other_relocs()
{
	Elf32_Addr	refto;
	Elf32_Rel	*rel;
	Elf32_Sym	*sym;
	int i;

	for (i = 1; i < (int) Ehdr->e_shnum; i++) {
		int rtype = esections[i].sec_shdr->sh_type;
		int refsec;
		Elf32_Rel *endrel;

		if (rtype != SHT_REL && rtype != SHT_RELA)
			continue;

		/* We already did the text section as we were scanning
		** through it, let's just put in the new one
		*/
		if (esections[i].sec_shdr->sh_info == Text_index) {
			esections[i].sec_data->d_buf = (void *) Rel;
			esections[i].sec_data->d_size = Nrel * sizeof(Elf32_Rel);
			continue;
		}

		if (!esections[i].sec_data)
			esections[i].sec_data = myelf_getdata(esections[i].sec_scn, 0, "relocation section");

		refsec = esections[i].sec_shdr->sh_info;

		if (!esections[refsec].sec_data)
			esections[refsec].sec_data = myelf_getdata(esections[refsec].sec_scn, 0, "relocation section");

		endrel = ((Elf32_Rel *) esections[i].sec_data->d_buf) + esections[i].sec_data->d_size / sizeof(Elf32_Rel);

		for (rel = (Elf32_Rel *) esections[i].sec_data->d_buf; rel < endrel; rel++) {
			if (((refto = get_refto(rel, esections[refsec].sec_data->d_buf, &sym, Origsymtab + 1)) == NO_SUCH_ADDR) || (refto == sym->st_value))
				continue;
			fixup_one_reloc(rel, esections[refsec].sec_data->d_buf, refto);
		}
	}
}

struct ehfuncmap {
	int funcno;
	int start;
	Elf32_Rel *rel;
	Elf32_Rel *endrel;
};

static int
comp_ehfuncs(const void *v1, const void *v2)
{
	struct ehfuncmap *func1 = ((struct ehfuncmap *) v1);
	struct ehfuncmap *func2 = ((struct ehfuncmap *) v2);

	return(Func_info[func1->funcno].order_start - Func_info[func2->funcno].order_start);
}

/*
* Resort the eh structures such that the addresses that they refer to
* are in ascending order.
*/
void
eh_gencode()
{
	int i, fill, ehfunc, funcno, start_block, end_block;
	Elf32_Addr oldaddr1, newaddr1, newaddr2, addr, oldaddr2;
	Elf32_Rel *startrel, *rel, *endrel;
	struct eheither *eh = (struct eheither *) (Eh_data->d_buf);
	struct eheither *neweh;
	struct ehregion *regtable;
	int reglen;
	struct ehfuncmap *ehfuncs;
	int nehfuncs;
	int neh;

	neh = Eh_data->d_size / sizeof(struct eheither);
	neweh = (struct eheither *) MALLOC(Eh_data->d_size);
	startrel = (Elf32_Rel *) (Eh_reldata->d_buf);
	endrel = (Elf32_Rel *) ((Elf32_Addr) Eh_reldata->d_buf + Eh_reldata->d_size);
	qsort(startrel, endrel - startrel, sizeof(Elf32_Rel), comp_relocations);
	ehfuncs = MALLOC((neh + 1) * sizeof(struct ehfuncmap));

	nehfuncs = 0;
	i = 0;
	rel = startrel;
	do {
		ehfuncs[nehfuncs].start = i;
		addr = get_refto(rel, Eh_data->d_buf, NULL, Symstart);
		ehfuncs[nehfuncs].funcno = func_by_newaddr(addr);
		do {
			i++;
		} while ((i < neh) && (eh[i].type == EH_TRYBLOCK));
		ehfuncs[nehfuncs].rel = rel;
		while ((rel < endrel) && (rel->r_offset < i * sizeof(struct eheither)))
			rel++;
		ehfuncs[nehfuncs].endrel = rel;
		nehfuncs++;
	} while (i < neh);
	ehfuncs[nehfuncs].start = i;
	ehfuncs[nehfuncs].funcno = Nfuncs;
	ehfuncs[nehfuncs].rel = endrel;
	qsort(ehfuncs, nehfuncs, sizeof(struct ehfuncmap), comp_ehfuncs);
	for (fill = 0, ehfunc = 0; ehfunc < nehfuncs; ehfunc++) {
		/* First, fix up the function and try blocks */
		i = ehfuncs[ehfunc].start;
		rel = ehfuncs[ehfunc].rel;
		funcno = ehfuncs[ehfunc].funcno;
		start_block = Func_info[funcno].start_block;
		do {
			addr = get_refto(rel, Eh_data->d_buf, NULL, Symstart);
			for ( ; start_block < Func_info[funcno].end_block; start_block++)
				if ((NEW_START_ADDR(start_block) <= addr) && (NEW_END_ADDR(start_block) > addr))
					break;
			if (addr == NEW_START_ADDR(start_block))
				oldaddr1 = START_ADDR(start_block);
			else
				oldaddr1 = START_ADDR(start_block) + (addr - CODEPOS(start_block));
			oldaddr2 = oldaddr1 + eh[i].len;
			for (end_block = start_block; end_block < Func_info[funcno].end_block; end_block++)
				if (START_ADDR(end_block) >= oldaddr2)
					break;
			if (end_block == start_block)
				newaddr2 = NEW_START_ADDR(end_block);
			else {
				do {
					end_block--;
				} while (ORDER(end_block) == NO_SUCH_ADDR);
				if (END_ADDR(end_block) > oldaddr2) {
					if (oldaddr2 == START_ADDR(end_block))
						newaddr2 = NEW_START_ADDR(end_block);
					else
						newaddr2 = CODEPOS(end_block) + (oldaddr2 - START_ADDR(end_block));
				}
				else
					newaddr2 = CORRPOS(end_block);
			}
			neweh[fill] = eh[i];
			neweh[fill].len = newaddr2 - addr;
			do {
				Elf32_Addr fill5_off;

				fill5_off = ((Elf32_Addr) &eh[i].fill5) - (Elf32_Addr) eh;
				/*
				* This is a bit hard-coded here.  If field5 is a
				* pointer to itself then move the pointer to point to
				* its new location.
				*/
				if ((ELF32_R_SYM(rel->r_info) == Eh_sym) && (rel->r_offset == fill5_off) && (eh[i].fill5 == fill5_off))
					neweh[fill].fill5 += (fill - i) * sizeof(struct eheither);
				rel->r_offset += (fill - i) * sizeof(struct eheither);
				rel++;
			} while ((rel < ehfuncs[ehfunc].endrel) && (rel->r_offset < (i + 1) * sizeof(struct eheither)));
			fill++;
			i++;
		} while ((i < neh) && (eh[i].type != EH_FUNCTION));

		/* Then fix up the associated region table */
		regtable = (struct ehregion *) ((caddr_t) Eh_other->d_buf + ((struct ehfunc_entry *) eh)[ehfuncs[ehfunc].start].rangetable);
		reglen = *((ulong *) regtable);
		regtable = (struct ehregion *) ((Elf32_Addr) regtable + sizeof(Elf32_Addr));
		newaddr1 = NEW_START_ADDR(Order[Func_info[ehfuncs[ehfunc].funcno].order_start]);
		end_block = Func_info[ehfuncs[ehfunc].funcno].start_block;
		for (i = 0; i < reglen; i++) {
			oldaddr2 = START_ADDR(Func_info[ehfuncs[ehfunc].funcno].start_block) + regtable[i].offset;
			for ( ; end_block < Func_info[ehfuncs[ehfunc].funcno].end_block; end_block++)
				if (START_ADDR(end_block) >= oldaddr2)
					break;
			if (regtable[i].offset == 0)
				newaddr2 = NEW_START_ADDR(Func_info[ehfuncs[ehfunc].funcno].start_block);
			else {
				do {
					end_block--;
				} while (ORDER(end_block) == NO_SUCH_ADDR);
				if (END_ADDR(end_block) > oldaddr2) {
					if (oldaddr2 == START_ADDR(end_block))
						newaddr2 = NEW_START_ADDR(end_block);
					else
						newaddr2 = CODEPOS(end_block) + (oldaddr2 - START_ADDR(end_block));
				}
				else
					newaddr2 = CORRPOS(end_block);
			}
			regtable[i].offset = newaddr2 - newaddr1;
		}
	}
	free(ehfuncs);
	Eh_data->d_buf = (Elf_Void *) neweh;
}
