#ident	"@(#)fur:i386/cmd/fur/insertion.c	1.1.3.5"
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

#ifndef FUR_DIR
#define FUR_DIR "/usr/ccs/lib/fur"
#endif

/*
** Read the elf file for the code to be inserted
*/
void
read_elf(struct codeblock *block, char *filename)
{
	char buf[PATH_MAX];
	int fd, text_index, i, j;
	int str_index;
	ulong *maptable, *mapwalk;
	caddr_t newstrbuf, walkstr;
	Elf32_Sym *newsymbuf;
	int newsymsize;
	int newstrsize;
	char *symname, *str;
	int symlen;
	Elf32_Addr end;
	Elf32_Sym *sym, *newsym, *walksym;
	Elf32_Rel *rel;
	Elf		*elf_file;
	Elf32_Ehdr	*ehdr;
	Elf_Scn		*scn;
	Elf32_Shdr	*shdr;
	char *name;

	if ((fd = open(filename, O_RDONLY)) < 0) {
		sprintf(buf, "%s/%s", FUR_DIR, filename);
		if ((fd = open(buf, O_RDONLY)) < 0)
			error("cannot open %s\n", filename);
	}

	if ((elf_file = elf_begin(fd, ELF_C_READ, (Elf *)0)) == 0)
		error("ELF error in elf_begin of %s: %s\n", filename, elf_errmsg(elf_errno()));

	/*
	 *	get ELF header
	 */
	if ((ehdr = elf32_getehdr(elf_file)) == 0)
		error("problem with ELF header of %s: %s\n", filename, elf_errmsg(elf_errno()));

	/*
	 *	check that it is a relocatable file
	 */
	 if (ehdr->e_type != ET_REL)
		error("%s is not a relocatable file\n", filename);

	scn = 0;

	for (i = 1; (scn =  elf_nextscn(elf_file, scn)) != 0; i++) {
		shdr = elf32_getshdr(scn);
		if (shdr->sh_type == SHT_SYMTAB) {
			block->symtab = myelf_getdata(scn, 0, "symbol table");
			str_index = shdr->sh_link;
		}
		else {
			if ((name = elf_strptr(elf_file, ehdr->e_shstrndx, shdr->sh_name)) == NULL)
				error("cannot get name for section header\n");
			if (strcmp(name, ".text") == 0) {
				text_index = i;
				block->textsec = myelf_getdata(scn, 0, "section to be rearranged\n");
			}
		}
	}

	scn = 0;

	for (i = 1; (scn =  elf_nextscn(elf_file, scn)) != 0; i++) {
		shdr = elf32_getshdr(scn);
		if (i == str_index)
			block->strtab = myelf_getdata(scn, 0, "string table\n");
		else if ((shdr->sh_type == SHT_REL) && (shdr->sh_info == text_index))
			block->relsec = myelf_getdata(scn, 0, "section to be rearranged\n");
	}
	if (!block->symtab || !block->textsec || !block->strtab)
		error("Missing symbol table or text section in %s\n", filename);

	end = ((Elf32_Addr) block->symtab->d_buf + block->symtab->d_size);
	for (sym = (Elf32_Sym *) block->symtab->d_buf + 1; sym < (Elf32_Sym *) end; sym++) {
		symname = ((char *) block->strtab->d_buf) + sym->st_name;
		if (strncmp(symname, "__FILENAME__", 12) == 0)
			sprintf(symname, "%s%s", Shortname, symname + 12);
		if (strstr(symname, "_END_TYPE")) {
			symlen = strlen(symname);
			newsymsize = block->symtab->d_size + 24 * sizeof(Elf32_Sym);
			newstrsize = block->strtab->d_size + 100 + 24 * (symlen + 2);
			newsymbuf = (Elf32_Sym *) MALLOC(newsymsize);
			newstrbuf = MALLOC(newstrsize);
			memcpy(newsymbuf, block->symtab->d_buf, block->symtab->d_size);
			memcpy(newstrbuf, block->strtab->d_buf, block->strtab->d_size);
			sym = newsymbuf + (sym - (Elf32_Sym *) block->symtab->d_buf);
			walksym = newsymbuf + block->symtab->d_size / sizeof(Elf32_Sym);
			walkstr = newstrbuf + block->strtab->d_size;
			sym->st_size = walksym - sym;
			block->symtab->d_buf = newsymbuf;
			end = ((Elf32_Addr) block->symtab->d_buf + block->symtab->d_size);
			block->strtab->d_buf = newstrbuf;

			for (j = i = 0; i < N_END_TYPES; ) {
				if (i == END_CJUMP) {
					if (j != 16)
						str = JCCtable[j++];
					else {
						str = ENDtable[++i];
						i++;
					}
				}
				else
					str = ENDtable[i++];
				walksym[0] = sym[0];
				walksym->st_name = walkstr - newstrbuf;
				walkstr += sprintf(walkstr, "%.*s_%s", symlen - 9, symname, str) + 1;
				walksym++;
			}
			block->symtab->d_size = (caddr_t) walksym - (caddr_t) block->symtab->d_buf;
			block->strtab->d_size = walkstr - (caddr_t) block->strtab->d_buf;
		}
		else
			sym->st_size = 0;
	}
	end = ((Elf32_Addr) block->symtab->d_buf + block->symtab->d_size);
	maptable = (ulong *) MALLOC((block->symtab->d_size/sizeof(Elf32_Sym) + 1) * sizeof(ulong));
	for (mapwalk = maptable + 1, newsym = (Elf32_Sym *) block->symtab->d_buf, sym = (Elf32_Sym *) block->symtab->d_buf + 1; sym < (Elf32_Sym *) end; sym++) {
		symname = ((char *) block->strtab->d_buf) + sym->st_name;
		if (strcmp(symname, "block_number") == 0)
			mapwalk[0] = BLOCKNO_SYMNO;
		else if (strcmp(symname, "function_number") == 0)
			mapwalk[0] = FUNCNO_SYMNO;
		else {
			switch(ELF32_ST_TYPE(sym->st_info)) {
			case STT_OBJECT:
			case STT_NOTYPE:
			case STT_FUNC:
				if (sym->st_shndx != SHN_UNDEF)
					error("%s can not have any defined symbols\n", filename);
				if (ELF32_ST_BIND(sym->st_info) != STB_LOCAL) {
					if (newsym != sym)
						newsym[0] = sym[0];
					mapwalk[0] = (newsym++ - (Elf32_Sym *) block->symtab->d_buf) + 1;
				}
			}
		}
		mapwalk++;
	}
	block->symtab->d_size = ((Elf32_Addr) newsym) - ((Elf32_Addr) block->symtab->d_buf);
	end = ((Elf32_Addr) block->relsec->d_buf + block->relsec->d_size);
	for (rel = (Elf32_Rel *) block->relsec->d_buf; rel < (Elf32_Rel *) end; rel++)
		rel->r_info = ELF32_R_INFO(maptable[ELF32_R_SYM(rel->r_info)], ELF32_R_TYPE(rel->r_info));
	free(maptable);
}


/*
* The code to be inserted generates new symbols in the symbol table as
* well as new entries in the string section.  This adds these to the
* respective sections
*/
void
add_new_code()
{
	char *newsym;
	char *newwalksym;
	char *newwalkstr;
	char *newstr;
	Elf32_Sym *walk, *lastsym, *oldstart;
	int i, j, count;

	if (!New)
		return;
	if (!Textrel) {
		Growsym++;
		Growstr += strlen(".rel.text") + 1;
	}
	newsym = MALLOC(Sym_data->d_size + Growsym * sizeof(Elf32_Sym));
	memcpy(newsym, Sym_data->d_buf, Sym_data->d_size);

	newstr = MALLOC(Str_data->d_size + Growstr);
	memcpy(newstr, Str_data->d_buf, Str_data->d_size);

	newwalksym = newsym + Sym_data->d_size;
	newwalkstr = newstr + Str_data->d_size;
	for (i = 0; i < Nnew; i++) {
		if (New[i]->symtab) {
			memcpy(newwalksym, New[i]->symtab->d_buf, New[i]->symtab->d_size);
			newwalksym += New[i]->symtab->d_size;
		}
		if (New[i]->strtab) {
			memcpy(newwalkstr, New[i]->strtab->d_buf, New[i]->strtab->d_size);
			newwalkstr += New[i]->strtab->d_size;
		}
	}
	Sym_data->d_buf = newsym;
	Sym_data->d_size = newwalksym - newsym;
	Str_data->d_buf = newstr;
	Str_data->d_size = newwalkstr - newstr;
	oldstart = Symstart;
	Symstart = ((Elf32_Sym *) Sym_data->d_buf) + 1;
	for (i = 0; i < Nfuncs; i++)
		Funcs[i] = Symstart + (Funcs[i] - oldstart);
	for (i = 0; i < Nnonfuncs; i++)
		Nonfuncs[i] = Symstart + (Nonfuncs[i] - oldstart);
}

/*
* Adding non-contiguous code creates new symbols.  Add them to the
* symbol table
*/
void
add_low_usage()
{
	char *newsym;
	char *newwalksym;
	char *newwalkstr;
	char *newstr;
	Elf32_Sym *walk, *lastsym;
	int i, j, count;

	if (New)
		return;
	if (!Textrel) {
		Growsym++;
		Growstr += strlen(".rel.text") + 1;
	}
	SET_ORDER(Nblocks, Norder);
	if (!Forcecontiguous) {
		for (i = 0; i < Nfuncs; i++) {
			if (!(Func_info[i].flags & FUNC_PLACED))
				continue;
			for (j = Func_info[i].start_block; j < Func_info[i].end_block; j++) {
				if (FLAGS(j) & LOW_USAGE) {
					Growsym++;
					Growstr += strlen(NAME(Funcs[i])) + sizeof(".low_usage.");
					if (Func_info[i].flags & DUP_NAME)
						Growstr += 6;
					break;
				}
			}
		}
	}
	newsym = MALLOC(Sym_data->d_size + Growsym * sizeof(Elf32_Sym));
	memcpy(newsym, Sym_data->d_buf, Sym_data->d_size);

	newstr = MALLOC(Str_data->d_size + Growstr);
	memcpy(newstr, Str_data->d_buf, Str_data->d_size);

	newwalksym = newsym + Sym_data->d_size;
	newwalkstr = newstr + Str_data->d_size;
	if (!Forcecontiguous && Growsym) {
		lastsym = NULL;
		walk = (Elf32_Sym *) newwalksym;
		for (count = 0, j = 0; j < Norder; j++) {
			i = Order[j];
			if (IS_FUNCTION_START(i)) {
				if (IN_LOW_USAGE_FUNC(i)) {
					if (lastsym)
						lastsym->st_size = NEW_END_ADDR(Order[j - 1]) - lastsym->st_value;
				}
				lastsym = ((Elf32_Sym *) newsym) + (START_SYM(i) - (Elf32_Sym *) Sym_data->d_buf);
			}
			else if ((j > 0) && !IN_LOW_USAGE_FUNC(i) && (FLAGS(i) & LOW_USAGE) && (FUNCNO(i) != FUNCNO(Order[j - 1]))) {
				if (lastsym)
					lastsym->st_size = NEW_END_ADDR(Order[j - 1]) - lastsym->st_value;
				lastsym = walk;
				walk->st_name = newwalkstr - newstr;
				walk->st_value = NEW_START_ADDR(i);
				walk->st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC);
				walk->st_other = 0;
				walk->st_shndx = Text_index;
				walk++;
				if (Func_info[FUNCNO(i)].flags & DUP_NAME)
					newwalkstr += sprintf(newwalkstr, "%s.low.usage.%d", NAME(Funcs[FUNCNO(i)]), count++) + 1;
				else
					newwalkstr += sprintf(newwalkstr, "%s.low.usage", NAME(Funcs[FUNCNO(i)])) + 1;
			}
		}
		if (lastsym && (walk != (Elf32_Sym *) newwalksym))
			lastsym->st_size = NEW_START_ADDR(Nblocks) - lastsym->st_value;
		newwalksym = (char *) walk;
	}
	Sym_data->d_buf = newsym;
	Sym_data->d_size = newwalksym - newsym;
	Str_data->d_buf = newstr;
	Str_data->d_size = newwalkstr - newstr;
}

/*
** The relocations in the new code must be adjusted to the new locations
*/
void
fixup_new_code_reloc(int symstart, int strstart)
{
	Elf32_Sym *sym;
	Elf32_Rel *rel;
	Elf32_Addr end;
	int nextsymstart = symstart;
	int nextstrstart = strstart;
	int i;
	int symno;

	for (i = 0; i < Nnew; i++) {
		if (nextsymstart) {
			end = ((Elf32_Addr) New[i]->relsec->d_buf + New[i]->relsec->d_size);
			for (rel = (Elf32_Rel *) New[i]->relsec->d_buf; rel < (Elf32_Rel *) end; rel++) {
				symno = ELF32_R_SYM(rel->r_info);
				if ((symno != BLOCKNO_SYMNO) && (symno != FUNCNO_SYMNO))
					rel->r_info = ELF32_R_INFO(symno + nextsymstart, ELF32_R_TYPE(rel->r_info));
			}
		}
		nextsymstart += New[i]->symtab->d_size / sizeof(Elf32_Sym);
		if (nextstrstart) {
			end = ((Elf32_Addr) New[i]->symtab->d_buf + New[i]->symtab->d_size);
			for (sym = (Elf32_Sym *) New[i]->symtab->d_buf; sym < (Elf32_Sym *) end; sym++)
				sym->st_name += nextstrstart;
		}
		nextstrstart += New[i]->strtab->d_size;
	}
	Growsym = nextsymstart - symstart;
	Growstr = nextstrstart - strstart;
}

/*
** Put the new code into the New data structure
*/
void
post_new_code(struct codeblock *block)
{
	New = (struct codeblock **) REALLOC(New, (Nnew + 1) * sizeof(struct codeblock *));
	New[Nnew++] = block;
}

/*
** Figure out which code to insert and where
*/
void
setup_insertion()
{
	int i, j;

	if (!(Allblocks || Allepilogues || Allprologues || Someblocks || Someprologues || Someepilogues || Flowblocks))
		return;

	Blocks_insertion = (struct block_insertion *) REZALLOC(Blocks_insertion, (Nblocks + 1) * sizeof(struct block_insertion));

	if (Allepilogues || Allprologues) {
		for (i = 0; i < Nblocks; i++) {
			if (Func_info[FUNCNO(i)].flags & FUNC_GROUPED)
				continue;
			if (FLAGS(i) & DATA_BLOCK)
				continue;
			if (Allepilogues && (END_TYPE(i) == END_RET))
				SET_INSERTION_AT_END(i, &Epilogue);
			if (IS_FUNCTION_START(i)) {
				if (Allprologues)
					SET_INSERTION_AT_BEGINNING(i, &Prologue);
			}
		}
	}
	if (Allblocks || Flowblocks) {
		for (i = 0; i < Nblocks; i++) {
			if (Func_info[FUNCNO(i)].flags & FUNC_GROUPED)
				continue;
			if (FLAGS(i) & (START_NOP|DATA_BLOCK))
				continue;
			if (Flowblocks) {
				if (END_TYPE(i) == END_CJUMP)
					SET_INSERTION_AT_END(i, &Perblock);
				else {
					if (FLAGS(i) & ENTRY_POINT)
						SET_INSERTION_AT_BEGINNING(i, &Perblock);
					if (END_TYPE(i) == END_IJUMP) {
						Elf32_Addr *succs;
						int nsuccs;

						if (nsuccs = get_succs(i, &succs))
							for (j = 0; j < nsuccs; j++)
								if (END_TYPE(succs[j]) != END_CJUMP)
									SET_INSERTION_AT_BEGINNING(succs[j], &Perblock);
					}
				}
			}
			else {
				if (!TEST_INSERTION_AT_BEGINNING(i))
					SET_INSERTION_AT_BEGINNING(i, &Perblock);
				else if (!TEST_INSERTION_AT_END(i))
					SET_INSERTION_AT_END(i, &Perblock);
			}
		}
		if (Flowblocks) {
			Flowblockno = CALLOC(Nblocks, sizeof(int));
			Nflow = 0;
			for (i = 0; i < Nblocks; i++)
				if (TEST_INSERTION_AT_BEGINNING(i) || TEST_INSERTION_AT_END(i))
					Flowblockno[i] = Nflow++;
		}
	}
	if (Someprologues) {
		FILE *fp;
		char buf[BUFSIZ];

		if (!(fp = fopen(Someprologues, "r")))
			error("Cannot open %s\n", Someprologues);
		while (fgets(buf, BUFSIZ, fp)) {
			buf[strlen(buf) - 1] = '\0';
			if (*buf == '#')
				continue;
			if ((i = find_name(buf)) < 0) {
				if (!No_warnings)
					fprintf(stderr, "Cannot find symbol or symbol is unused: %s\n", buf);
				continue;
			}
			i = Func_info[i].start_block;
			if (Func_info[FUNCNO(i)].flags & FUNC_GROUPED)
				continue;
			if (!TEST_INSERTION_AT_BEGINNING(i))
				SET_INSERTION_AT_BEGINNING(i, &Prologue);
		}
		fclose(fp);
	}
	if (Someepilogues) {
		FILE *fp;
		char buf[BUFSIZ];

		if (!(fp = fopen(Someepilogues, "r")))
			error("Cannot open %s\n", Someepilogues);
		while (fgets(buf, BUFSIZ, fp)) {
			buf[strlen(buf) - 1] = '\0';
			if (*buf == '#')
				continue;
			if ((i = find_name(buf)) < 0) {
				if (!No_warnings)
					fprintf(stderr, "Cannot find symbol or symbol is unused: %s\n", buf);
				continue;
			}
			i = Func_info[i].start_block;
			do {
				if (!(Func_info[FUNCNO(i)].flags & FUNC_GROUPED)) {
					if (END_TYPE(i) == END_RET) {
						if (!TEST_INSERTION_AT_END(i))
							SET_INSERTION_AT_END(i, &Epilogue);
					}
				}
				i++;
			} while ((i < Nblocks) && !(IS_FUNCTION_START(i)));
		}
		fclose(fp);
	}
	if (Someblocks) {
		FILE *fp;
		char buf[BUFSIZ];

		if (!(fp = fopen(Someblocks, "r")))
			error("Cannot open %s\n", Someblocks);
		while (fgets(buf, BUFSIZ, fp)) {
			buf[strlen(buf) - 1] = '\0';
			if (*buf == '#')
				continue;
			if ((i = find_name(buf)) < 0) {
				if (!No_warnings)
					fprintf(stderr, "Cannot find symbol or symbol is unused: %s\n", buf);
				continue;
			}
			i = Func_info[i].start_block;
			do {
				if (!(Func_info[FUNCNO(i)].flags & FUNC_GROUPED)) {
					if (!(FLAGS(i) & (START_NOP|DATA_BLOCK))) {
						if (!TEST_INSERTION_AT_BEGINNING(i))
							SET_INSERTION_AT_BEGINNING(i, &Perblock);
						else if (!TEST_INSERTION_AT_END(i))
							SET_INSERTION_AT_END(i, &Perblock);
					}
				}
				i++;
			} while ((i < Nblocks) && !(IS_FUNCTION_START(i)));
		}
		fclose(fp);
	}
#ifdef SPECIAL
	for (i = 0; i < Nspecial; i++) {
				for (j = 0; j < Nspecial; j++) {
					if (!Special[j].found && (strcmp(NAME(START_SYM(i)), Special[j].func) == 0)) {
						Special[j].found = 1;
						if (!Special[j].epilogue)
							Special[j].block = i;
						else for (k = i + 1; k < Nblocks; k++) {
							if (FLAGS(k) & FUNCTION_START)
								break;
							if (END_TYPE(k) == END_RET)
								Special[j].block = k;
						}
						break;
					}
				}
		if (Special[j].epilogue)
			SET_INSERTION_AT_END(Special[i].block, &Special[i].code);
		else
			SET_INSERTION_AT_BEGINNING(Special[i].block, &Special[i].code);
	}
#endif
}

void
find_0f()
{
	int i;
	Elf32_Addr next_addr, addr;
	ulong inst, inst2;

	for (i = 0; i < Nblocks; i++) {
		if (FLAGS(i) & DATA_BLOCK)
			continue;
		for (addr = START_ADDR(i); addr < END_ADDR(i); addr = next_addr) {
			inst = GET1(Text_data->d_buf, addr);
			Shdr.sh_addr = addr;
			p_data = (char *) Text_data->d_buf + addr;
			dis_text(&Shdr);
			next_addr = loc;
			if ((inst == 0xf) && (((inst2 = GET1(Text_data->d_buf, addr + 1)) > 0x8f) || (inst2 < 0x80))) {
				if (!(Nfound_0f % 100))
					Found_0f = REALLOC(Found_0f, (Nfound_0f + 101) * sizeof(struct found_0f));
				Found_0f[Nfound_0f].block = i;
				Found_0f[Nfound_0f++].offset = addr - START_ADDR(i);
			}
		}
	}
}
