#ident	"@(#)ktool:i386at/ktool/scodb/local.c	1.1"
#include <stdio.h>
#include <libelf.h>

#define MIN_VALUE	0xc0000000

extern int errno;

Elf *elf;
Elf_Scn *symtab_scn, *strtab_scn;
Elf32_Ehdr *ehdr;
Elf_Data *symtab_data;
int strtab_index;
int strtab_bytes = 0, symtab_bytes, symtab_entries;
char *local_strtab;

/*
 * Compare two symbols for qsort()
 */

int
sym_compar(Elf32_Sym *sp_a, Elf32_Sym *sp_b)
{
	if (sp_a->st_value < sp_b->st_value)
		return(-1);
	else
	if (sp_a->st_value > sp_b->st_value)
		return(1);
	else
		return(0);
}

int
write_local_symbols(char *kernel_name, FILE *f_out)
{
	Elf_Scn *scn;
	Elf32_Shdr *shdr;
	int n, fd, index, bind, type;
	char *name, *strtab_ptr;
	Elf32_Sym *sp, *first_sp = NULL, *first_invalid_sp;

	if ((fd = open(kernel_name, 0)) < 0) {
		fprintf(stderr, "Cannot open kernel file %s (error %d)\n", kernel_name, errno);
		return(0);
	}

	if (elf_version(EV_CURRENT) == EV_NONE) {
		fprintf(stderr, "Elf library out of date\n");
		return(0);
	}

	if ((elf = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
		fprintf(stderr, "Cannot execute elf_begin()\n");
		return(0);
	}

	if ((ehdr = elf32_getehdr(elf)) == NULL) {
		fprintf(stderr, "Cannot get Elf extended header\n");
		return(0);
	}

	/*
	 * Scan section table looking for symbol table and string table.
	 */

	index = 1;
	scn = NULL;
	while ((scn = elf_nextscn(elf, scn)) != NULL) {
		if ((shdr = elf32_getshdr(scn)) == NULL) {
			fprintf(stderr, "Cannot get Elf section header\n");
			return(0);
		}
		if (shdr->sh_type != SHT_NULL) {
			name = elf_strptr(elf, ehdr->e_shstrndx, shdr->sh_name);
			if (shdr->sh_type == SHT_SYMTAB && !strcmp(name, ".symtab")) {
				symtab_scn = scn;
			}
			if (shdr->sh_type == SHT_STRTAB && !strcmp(name, ".strtab")) {
				strtab_scn = scn;
				strtab_index = index;
			}
		}
		index++;
	}

	/*
	 * Get the symbol table data.
	 */

	if ((symtab_data = elf_getdata(symtab_scn, NULL)) == NULL) {
		fprintf(stderr, "Cannot get Elf symbol table data\n");
		return(0);
	}

	/*
	 * Skip through the symbol table marking entries that we are
	 * not interested in.  This is done by setting the st_value
	 * field to zero.  The first non-local symbol halts the search.
	 * This is because the local symbols preceed the global
	 * symbols in the symbol table.  Keep a pointer to the first
	 * global symbol (one after the last local symbol).
	 */

	sp = symtab_data->d_buf;
	while ((char *)sp < (char *)symtab_data->d_buf + symtab_data->d_size) {
		bind = ELF32_ST_BIND(sp->st_info);
		type = ELF32_ST_TYPE(sp->st_info);
		name = elf_strptr(elf, strtab_index, sp->st_name);
		if (bind == STB_LOCAL) {
			if ((type == STT_OBJECT || type == STT_FUNC) &&
			    strncmp(name, ".X", 2) != 0) {
				if (first_sp == NULL)
					first_sp = sp;
			} else
				sp->st_value = 0;
		} else
			break;
		sp++;
	}

	first_invalid_sp = sp;	/* first global symbol */

	/*
	 * Sort the local symbols (at the beginning of the symbol table)
	 * by value.
	 */

	qsort(first_sp, sp - first_sp, sizeof(Elf32_Sym), sym_compar);

	/*
	 * Find the first local symbol with a value above the kernel
	 * minimum address.  This skips the un-needed symbols that we
	 * marked with a zero value above, and those symbols which
	 * represent real mode addresses.
	 */

	sp = first_sp;
	while (sp < first_invalid_sp && sp->st_value < MIN_VALUE) {
		sp++;
	}

	/*
	 * Find out how much space is needed for the string table of
	 * for the remaining symbols.
	 */

	first_sp = sp;
	while (sp < first_invalid_sp) {
		name = elf_strptr(elf, strtab_index, sp->st_name);
		strtab_bytes += strlen(name) + 1;
		sp++;
	}

	symtab_entries = sp - first_sp;
	symtab_bytes = symtab_entries * sizeof(Elf32_Sym);

	printf("symbol table has %d bytes and %d entries, string table has %d bytes\n", symtab_bytes, symtab_entries, strtab_bytes);

	/*
	 * Allocate space for new string table.
	 */

	if ((local_strtab = (char *)malloc(strtab_bytes)) == NULL) {
		fprintf(stderr, "Cannot allocate %d bytes for new string table\n", strtab_bytes);
		return(0);
	}

	/*
	 * Go through copying symbol strings to new string table, resetting
	 * each st_name field to the offset within the new string table.
	 */

	strtab_ptr = local_strtab;

	sp = first_sp;
	while (sp < first_invalid_sp) {
		name = elf_strptr(elf, strtab_index, sp->st_name);
		n = strlen(name);
		strcpy(strtab_ptr, name);
		sp->st_name = strtab_ptr - local_strtab;
		strtab_ptr += n + 1;
		sp++;
	}

	/*
	 * Write out the local symbol file in the following format:
	 *
	 *	int		number_of_entries;
	 *	Elf32_Sym	entries[number_of_entries];
	 *	char		string_table[];
	 */

	do_write(f_out, &symtab_entries, sizeof(symtab_entries));
	do_write(f_out, first_sp, symtab_bytes);
	do_write(f_out, local_strtab, strtab_bytes);

	elf_end(elf);

	free(local_strtab);

	return(sizeof(symtab_entries) + symtab_bytes + strtab_bytes);
}

do_write(FILE *f_out, char *addr, int nbytes)
{
	int n;

	if ((n = fwrite(addr, 1, nbytes, f_out)) != nbytes)
		fprintf(stderr, "Cannot write to output file (error %d)\n", errno);
}

