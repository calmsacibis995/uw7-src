#ident	"@(#)fur:common/cmd/fur/fur.c	1.6.3.8"
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
#define DECLARE_GLOBALS
#include "fur.h"
#include "op.h"
#include <macros.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>

#ifndef FUR_DIR
#define FUR_DIR "/usr/ccs/lib/fur"
#endif

int Pushdebug;

do_compile(int argc, char **argv, char *compile)
{
	char compilebuf[PATH_MAX];
	int j;
	char buf[256];

	sprintf(buf, "ARGC=%d", argc);
	putenv(strdup(buf));
	for (j = 0; j < argc; j++) {
		sprintf(buf, "ARGV%d=%s", j, argv[j]);
		putenv(strdup(buf));
	}
	sprintf(compilebuf, "%s %d %s %d", compile, Flowblocks ? Nflow : Nblocks, argv[optind], Nfuncs);
	execl("/bin/sh", "sh", "-c", compilebuf, 0);
}

int
#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
int argc;
char *argv[];
#endif
{
	char *prologue_arg = NULL, *epilogue_arg = NULL, *block_arg = NULL;
	struct rlimit rlim;
	int dump_funcs = 0;
	int compare = 0;
	int readonly = 0;
	int call_pct, jump_pct;
	char *compile = NULL;
	Elf32_Sym extrasym;
	Elf32_Rel *rel;
	char *p;
	unsigned int 	i;
	char 		*name;
	Elf32_Sym	*esym;
	Elf_Scn		*scn;
	char		*curfile;	/* current file in symbol table */
	char		*putbuf;

	/* file information for target file */
	int 		fd;
	Elf		*elf_file;

	/* info on symbol table section */
	Elf32_Sym	*endsym;	/* pointer to end of symbol table */

	/* info on string table for symbol table */
	int		str_index;

	int		c;
	extern char	*optarg;
	extern int	optind;
	extern int Silent_mode;

	Shdr.sh_size = 1;
	if (getenv("VERBOSE"))
		Verbose_mode = atoi(getenv("VERBOSE"));
	if (getenv("DIS_MODE"))
		Silent_mode = 0;
	else
		Silent_mode = 1;
	rlim.rlim_cur = RLIM_INFINITY;
	rlim.rlim_max = RLIM_INFINITY;
	if (setrlimit(RLIMIT_DATA, &rlim) != 0) {
		if (getrlimit(RLIMIT_DATA, &rlim) == 0) {
			rlim.rlim_cur = rlim.rlim_max;
			setrlimit(RLIMIT_DATA, &rlim);
		}
	}
	rlim.rlim_cur = RLIM_INFINITY;
	rlim.rlim_max = RLIM_INFINITY;
	if (setrlimit(RLIMIT_VMEM, &rlim) != 0) {
		if (getrlimit(RLIMIT_VMEM, &rlim) == 0) {
			rlim.rlim_cur = rlim.rlim_max;
			setrlimit(RLIMIT_VMEM, &rlim);
		}
	}
		
	Pushdebug = !!getenv("PUSHDEBUG");
	Debug = 0;
	LowUsageRatio = 9;
	InlineCriteria = 0;
	InlineCallRatio = 90;
	NoDataAllowed = 0;
	Loopratio = 50;
	Loopalign = 16;
	Funcalign = 16;
	Textalign = 32;
	SafetyCheck = 0;
	Funcistext = 1;
	Numfuncalign = ULONG_MAX;
	Forcecontiguous = 0;
	Checkinstructions = 0;
	Existwarnings = 1;
	Forcedis = NO_SUCH_ADDR;

	putbuf = MALLOC(sizeof("PATH=") + sizeof(":"FUR_DIR) + strlen(getenv("PATH")));
	strcpy(putbuf, "PATH=");
	strcat(putbuf, getenv("PATH"));
	strcat(putbuf, ":"FUR_DIR);
	putenv(putbuf);

	param_set_env();
	num_init(errexit, realloc);

	prog = argv[0];

	if (elf_version(EV_CURRENT) == EV_NONE)
		error("ELF library is out of date\n");

	No_warnings = 0;
	while ((c = getopt(argc, argv, "?XWO:mK:k:a:vc:o:M:f:rP:p:E:e:B:b:l:L:")) != EOF) {
		switch (c) {
		case 'L':
			LinkerOption = optarg;
			break;
		case 'W':
			No_warnings = 1;
			break;
		case 'O':
			param_set_var_val(optarg, 4);
			break;
		case 'K':
			Keeprebuild = 1;
		/* FALLTHROUGH */
		case 'k':
			Keepblocks = optarg;
			break;
		case 'l':
			Functionfile = optarg;
			break;
		case 'a':
			Special = (struct special *) REALLOC(Special, (Nspecial + 1) * sizeof(struct special));
			if (optarg[0] == 'e')
				Special[Nspecial].epilogue = 1;
			Special[Nspecial].func = strtok(strdup(optarg + 2), ":");
			Special[Nspecial++].file = strtok(NULL, ":");
			break;
		case 'v':
			Viewfreq = 1;
			break;
		case 'm':
			Metrics = 1;
			break;
		case 'c':
			compile = optarg;
			break;
		case 'o':
			Orderfile = optarg;
			break;
		case 'M':
			Mergefile = optarg;
			break;
		case 'f':
			Freqfile = optarg;
			break;
		case 'r':
			param_set_var_val("SAFETY_CHECK=0", 1);
			readonly = 1;
			break;
		case 'b':
			if (strcmp(optarg, "all") == 0)
				Allblocks = 1;
			else if (strcmp(optarg, "flow") == 0)
				Flowblocks = 1;
			else
				Someblocks = optarg;
			break;
		case 'e':
			if (strcmp(optarg, "all"))
				Someepilogues = optarg;
			else
				Allepilogues = 1;
			break;
		case 'p':
			if (strcmp(optarg, "all"))
				Someprologues = optarg;
			else
				Allprologues = 1;
			break;
		case 'P':
			prologue_arg = optarg;
			break;
		case 'E':
			epilogue_arg = optarg;
			break;
		case 'B':
			block_arg = optarg;
			break;
		case 'X':
			dump_funcs = 1;
			break;
		default:
			usage("illegal option");
		}
	}

	if (!Freqfile && (Mergefile || Metrics || Viewfreq))
		usage("-m or -v option may not be used without the -f option");

	if (Functionfile && Orderfile && !Freqfile)
		usage("-l and -f options may not be used without the -f option");

	/* Emulate original fur interface by using force flag.
	** This will make it such that all blocks are "live" and
	** no blocks are NOP's
	*/
	if (Functionfile && !Freqfile && !Orderfile && !Allblocks && !Flowblocks && !Someblocks && !Someprologues && !Someepilogues)
		Force = 1;

	if (p = strrchr(argv[optind], '/'))
		p++;
	else
		p = argv[optind];
	sprintf(Shortname, "%.12s", p);
	if (strchr(Shortname, '.'))
		*strchr(Shortname, '.') = '\0';

	for (i = 0; i < Nspecial; i++) {
		read_elf(&Special[i].code, Special[i].file);
		post_new_code(&Special[i].code);
	}
	if (Allblocks || Someblocks || Flowblocks) {
		if (block_arg)
			read_elf(&Perblock, block_arg);
		else if (Flowblocks)
			read_elf(&Perblock, FUR_DIR"/flow.o");
		else
			read_elf(&Perblock, FUR_DIR"/block.o");
		post_new_code(&Perblock);
	}
	if (Allprologues || Someprologues) {
		if (prologue_arg)
			read_elf(&Prologue, prologue_arg);
		else
			read_elf(&Prologue, FUR_DIR"/prologue.o");
		post_new_code(&Prologue);
	}
	if (Allepilogues || Someepilogues) {
		if (epilogue_arg)
			read_elf(&Epilogue, epilogue_arg);
		else
			read_elf(&Epilogue, FUR_DIR"/epilogue.o");
		post_new_code(&Epilogue);
	}
	if (!argv[optind])
		usage("No relocatable supplied");

	if (getenv("FUR_COMPARE") && argv[optind + 1]) {
		compare = 1;
		TestPushPop = 1;
	}

	do {
		fflush(stdout);
		Nfuncs = 0;
		Nnonfuncs = 0;
		Nends = 0;
		Nblocks = 0;
		if (readonly)
			Textalign = 0;
		Text_index = -1;
		if ((fd = open(argv[optind], readonly ? O_RDONLY : O_RDWR)) < 0)
			error("cannot open %s\n", argv[optind]);

		if ((elf_file = elf_begin(fd, readonly ? ELF_C_READ : ELF_C_RDWR, (Elf *)0)) == 0)
			error("ELF error in elf_begin: %s\n", elf_errmsg(elf_errno()));

		/*
		 *	get ELF header
		 */
		if ((Ehdr = elf32_getehdr(elf_file)) == 0)
			error("problem with ELF header: %s\n", elf_errmsg(elf_errno()));

		/*
		 *	check that it is a relocatable file
		 */
		 if (Ehdr->e_type != ET_REL)
			error("%s is not a relocatable file\n", argv[optind]);

		/*
		 *	load section table
		 */
		esections = REZALLOC(esections, sizeof(struct section) * Ehdr->e_shnum);

		i = 1;	/* skip the first entry so indexes match with file */
		scn = 0;

		while ((scn =  elf_nextscn(elf_file, scn)) != 0) {
			esections[i].sec_scn =  scn;
			esections[i].sec_shdr = elf32_getshdr(scn);
			if (esections[i].sec_shdr->sh_type == SHT_SYMTAB) {

				esections[i].sec_data = Sym_data =
					myelf_getdata(scn, 0, "symbol table");
				endsym = (Elf32_Sym *)
					 ((char *) Sym_data->d_buf + Sym_data->d_size);

				Sym_index = i;
				/* get string data for symbol table */
				str_index = esections[i].sec_shdr->sh_link;
			}

			if ((name = elf_strptr(elf_file, Ehdr->e_shstrndx, esections[i].sec_shdr->sh_name)) == NULL)
				error("cannot get name for section header %d\n", i);
			if (strcmp(name, ".eh_ranges") == 0) {
				esections[i].sec_data = Eh_data =
				   myelf_getdata(scn, 0, "Exception handling section\n");
				Eh_index = i;
			}
			else if (strcmp(name, ".eh_other") == 0)
				esections[i].sec_data = Eh_other =
				   myelf_getdata(scn, 0, "Exception handling section\n");
			else if (strcmp(name, ".rel.eh_ranges") == 0)
				esections[i].sec_data = Eh_reldata =
				   myelf_getdata(scn, 0, "Exception handling relocation section\n");
			else if (strcmp(name, ".text") == 0) {
				if (Text_index != -1)
					error("multiple %s sections\n", name);
				Text_index = i;
				esections[i].sec_data = Text_data =
				   myelf_getdata(scn, 0, "section to be rearranged\n");
			}
			else if (strcmp(name, ".rodata") == 0)
				Rodata_sec = i;
			i++;
		}
		if ((Text_index == -1) || (Text_data->d_size == 0)) {
			elf_end(elf_file);
			close(fd);
			if (compile) 
				do_compile(argc, argv, compile);
			if (compare)
				exit(1);
			continue;
		}
		if (esections[str_index].sec_shdr->sh_type != SHT_STRTAB)
			error("symbol table does not point to string table.\n");

		esections[str_index].sec_data = Str_data = myelf_getdata(esections[str_index].sec_scn, 0, "string table");

		Symstart = ((Elf32_Sym *) Sym_data->d_buf) + 1;
		Fnames = REALLOC(Fnames, (endsym - Symstart) * sizeof(char *));
		Origsymtab = (Elf32_Sym *) REALLOC(Origsymtab, Sym_data->d_size);
		memcpy(Origsymtab, Sym_data->d_buf, Sym_data->d_size);

		/* Make pass through all symbols, recording functions in Funcs
		** and non-function symbols in Nonfuncs
		*/
		for (esym = Symstart; esym < endsym; esym++) {
			if (ELF32_ST_TYPE(esym->st_info) == STT_FILE) {
				if (strcmp(NAME(esym), "_fake_hidden"))
					curfile = NAME(esym);
				else
					curfile = NULL;
			}
			else if ((ELF32_ST_BIND(esym->st_info) == STB_GLOBAL) || (ELF32_ST_BIND(esym->st_info) == STB_WEAK))
				curfile = NULL;

			Fnames[esym - Symstart] = curfile;

			if (ELF32_ST_TYPE(esym->st_info) == STT_SECTION) {
				if (esym->st_shndx == Text_index)
					Text_sym = esym - Symstart + 1;
				else if (esym->st_shndx == Eh_index)
					Eh_sym = esym - Symstart + 1;
				continue;
			}

			if (esym->st_shndx != Text_index)
				continue;

			/* Any label must be tracked, because it can
			** be jumped to by another module
			*/
			if (ELF32_ST_TYPE(esym->st_info) != STT_FUNC) {
				if (!(Nnonfuncs % 100))
					Nonfuncs = (Elf32_Sym **) REALLOC(Nonfuncs, (Nnonfuncs + 100) * sizeof(Elf32_Sym *));
				Nonfuncs[Nnonfuncs++] = esym;
				continue;
			}

			if (!(Nfuncs % 100))
				Funcs = (Elf32_Sym **) REALLOC(Funcs, (Nfuncs + 101) * sizeof(Elf32_Sym *));
			Funcs[Nfuncs++] = esym;
		}
		if (Nfuncs) {
			qsort(Funcs, Nfuncs, sizeof(Elf32_Sym *), comp_orig_addr);
			qsort(Nonfuncs, Nnonfuncs, sizeof(Elf32_Sym *), comp_orig_addr);
			Funcs[Nfuncs] = &extrasym;
			Funcs[Nfuncs]->st_value = Text_data->d_size;
		}

		for (i = 1; i < Ehdr->e_shnum; i++) {
			int rtype = esections[i].sec_shdr->sh_type;

			if (rtype != SHT_REL && rtype != SHT_RELA)
				continue;

			if (esections[i].sec_shdr->sh_info != Text_index)
				continue;

			esections[i].sec_data = myelf_getdata(esections[i].sec_scn, 0, "relocation section");

			Textrel = (Elf32_Rel *) esections[i].sec_data->d_buf;
			Endtextrel = Textrel + esections[i].sec_data->d_size / sizeof(Elf32_Rel);
			for (rel = Textrel + 1; rel < Endtextrel; rel++) {
				if (rel->r_offset < rel[-1].r_offset) {
					qsort(Textrel, Endtextrel - Textrel, sizeof(Elf32_Rel), comp_relocations);
					break;
				}
			}
			break;
		}

		fixup_new_code_reloc(endsym - Symstart, Str_data->d_size);
		add_new_code();

		if (!readonly && !Textrel) {
			int old_num;

			esections = REALLOC(esections, sizeof(struct section) * (Ehdr->e_shnum + 1));
			old_num = Ehdr->e_shnum;
			esections[old_num].sec_scn = elf_newscn(elf_file);
			/* elf_newscn increments Ehdr->e_shnum */
			esections[old_num].sec_shdr = elf32_getshdr(esections[old_num].sec_scn);
			esections[old_num].sec_data = elf_newdata(esections[old_num].sec_scn);

			esections[old_num].sec_shdr->sh_name = -1;
			esections[old_num].sec_shdr->sh_type = SHT_REL;
			esections[old_num].sec_shdr->sh_info = Text_index;
			esections[old_num].sec_shdr->sh_link = Sym_index;
			esections[old_num].sec_data->d_type = ELF_T_REL;
			esections[old_num].sec_data->d_align = 4;
			esections[old_num].sec_data->d_version = 1;
		}

		sort_by_name();

		if (access(Keepblocks, F_OK) == 0) {
			if (!getblocks()) {
				if (Keeprebuild)
					genblocks();
				else
					error("%s does not match object file\n", Keepblocks);
			}
		}
		else
			genblocks(argv[optind]);

		if (!Nblocks) {
			elf_end(elf_file);
			close(fd);
			if (compile)
				do_compile(argc, argv, compile);
			if (compare)
				exit(1);
			continue;
		}
		Pass2 = 1;
		if (Freqfile)
			Blocks_stats = (struct block_stats *) REZALLOC(Blocks_stats, Sblocks * sizeof(struct block_stats));

		get_func_info();

		if (Forcedis == NO_SUCH_ADDR)
			find_jump_tables();

		data_entry_points();

		if (Debug && getenv("BLOCK_PRINT"))
			for (i = 0; i < Nblocks; i++)
				printf("%d: FLAGS = %x: START_ADDR = %x: JUMP_TARGET = %d: END_TYPE = %d\n", i, FLAGS(i), START_ADDR(i), JUMP_TARGET(i), END_TYPE(i));
		find_func_groups();

		if (Eh_data)
			eh_decode();

		if (Checkinstructions)
			checkinstructions();

		setup_insertion();

		for (i = 0; i < Nblocks; i++) {
			KEEP_FLAGS(i, DECODE_STABLE_FLAGS);
			if (FLAGS(i) & DATA_BLOCK)
				ADD_FLAGS(i, LOOP_HEADER);
		}

		Blocks_change = (struct block_change *) REZALLOC(Blocks_change, (Sblocks) * sizeof(struct block_change));

		for (i = 0; i < Nblocks; i++) {
			SET_NEW_END_INSTLEN(i, END_INSTLEN(i));
			SET_NEW_END_TYPE(i, END_TYPE(i));
		}

		find_block_reloc();

		compute_has_effect();

		setup_fallthrough();

		proc_freq();

		setup_targets();

		if (compare) {
			if (store1())
				continue;
			store2();
			compare_files();
			exit(0);
		}
		setup_order();

		Textalign = max(Textalign, max(Funcalign, Loopalign));
		if (Metrics) {
			jump_pct = calc_jumps();
			call_pct = calc_calls();
		}

		if (SafetyCheck)
			find_0f();

		for (i = 0; i < Nblocks; i++)
			KEEP_FLAGS(i, ORDER_STABLE_FLAGS);

		if (!readonly || Metrics)
			setup_jumps(readonly);

		if (Metrics) {
			int lue = calc_lue(0);
			int lue_new = calc_lue(1);
			ulong maxfunc = calc_maxfunc();

			printf("Maximum executed function: %s: %d\n", NAME(Funcs[maxfunc]), Func_info[maxfunc].ncalls);
			printf("Jump Percentage: %d.%d\n", jump_pct / 10, jump_pct % 10);
			printf("Call Percentage: %d.%d\n", call_pct / 10, call_pct % 10);
			printf("Line Usage Efficiency before tuning: %d.%d\nLine Usage Efficiency after tuning: %d.%d\n", lue / 10, lue % 10, lue_new / 10, lue_new % 10);
		}
		if (dump_funcs)
			for (i = 0; i < Nfuncs; i++)
				printf("%s%s%s\n", FULLNAME(Funcs[i]));
		if (!readonly) {
			if (!Text_ready) {
				fixup_symbol_table();
				fill_in_text();
				fixup_jumps();
			}
			fixup_other_relocs();
			fixup_pic_jump_tables();

			if (Eh_data)
				eh_gencode();

			Text_data->d_buf = Newtext;
			Text_data->d_size = NEW_START_ADDR(Nblocks);

			/* Lastly, grow the symbol and string tables for the low usage code */
			add_low_usage();

			cleanup_symbol_table();

			free(Blocks_std);
			Blocks_std = NULL;
			if (Blocks_change) {
				free(Blocks_change);
				Blocks_change = NULL;
			}
			Text_data->d_align = Textalign;
			if (elf_update(elf_file, ELF_C_WRITE) == -1)
				error("ERROR: attempting to write object: %s\n", elf_errmsg(elf_errno()));

			elf_end(elf_file);
			if (compile)
				do_compile(argc, argv, compile);
			Nblocks = 0;
		}
		else if (SafetyCheck)
			check_for_erratum();
		close(fd);
	} while (argv[++optind]);
	exit(0);
}

/*
 *	prt_offset ()
 *
 *	Print the offset, right justified, followed by a ':'.
 */

void
prt_offset()
{
	extern	long	loc;	/* from _extn.c */
	extern	int	oflag;
	extern	char	object[];

	if (oflag)
		(void)sprintf(object,"%6lo:  ",loc);
	else
		(void)sprintf(object,"%4lx:  ",loc);
	return;
}

line_nums()
{
}

search_rel_data()
{
}

extsympr()
{
}

compoff()
{
}

locsympr()
{
}

looklabel()
{
}
