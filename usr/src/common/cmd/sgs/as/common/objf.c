#ident	"@(#)nas:common/objf.c	1.8"
/*
* common/objf.c - object file handling
*
*	for now:this file only provides for FORMAT == ELF
*	eventually implement FORMAT == COFF
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifndef __STDC__
#  include <memory.h>
#endif
#include <fcntl.h>
#include "common/as.h"
#include "common/eval.h"
#include "common/expr.h"
#include "common/objf.h"
#include "common/sect.h"
#include "common/syms.h"
#include "target.h"

#include "intemu.h"

#if FORMAT != ELF
 #error "common/objf.c:FORMAT != ELF unimplemented"
#endif

	/*
	* Make use of the name-creating macros here so that the
	* following code will look less strange.
	*/
#define ETYPE_Ehdr	EMKT(Ehdr)
#define ETYPE_Sym	EMKT(Sym)
#define ETYPE_Shdr	EMKT(Shdr)
#define ETYPE_Word	EMKT(Word)
#define ETYPE_Rel	EMKT(Rel)
#define ETYPE_Rela	EMKT(Rela)

#define EMACR_ST_INFO	EMKM(ST_INFO)
#define EMACR_R_INFO	EMKM(R_INFO)

#define EFUNC_newehdr	EMKF(newehdr)
#define EFUNC_getshdr	EMKF(getshdr)
#define EFUNC_fsize	EMKF(fsize)

static Elf *elf;		/* the handle to libelf's information */
static ETYPE_Ehdr *elf_ehdr;	/* the elf file header */
static const Uchar *srcf_ptr;	/* specified source file name */
static size_t srcf_len;		/* length of srcf_ptr+\0 */

static struct	/* shared counts of various parts of the object file */
{
	size_t	sections;	/* total number of sections */
	size_t	user_sect;	/* number of input-generated sections */
	size_t	rel_sect;	/* actual number of relocation sections */
	size_t	symbols;	/* total number of symbols */
	size_t	local_sym;	/* number of regular local symbols */
	size_t	nonloc_sym;	/* number of regular global/weak symbols */
} num_of;

static struct	/* shared string table information */
{
	Uchar	*base;	/* beginning of string table */
	Uchar	*next;	/* next available byte in table */
	Section	*secp;	/* string table's section */
	size_t	size;	/* total available bytes */
	size_t	used;	/* number of bytes used */
} strtab;
static const Uchar MSGstrtab[] = ".strtab";	/* string table name */

static struct	/* shared symbol table information */
{
	ETYPE_Sym	*base;		/* beginning of symbol table */
	Section		*secp;		/* symbol table's section */
	size_t		index;		/* index number for symtab section */
	size_t		sect_base;	/* offset for first section symbol */
	size_t		local_base;	/* offset for first local symbol */
	size_t		nonloc_base;	/* offset for first nonlocal symbol */
} symtab;
static const Uchar MSGsymtab[] = ".symtab";	/* symbol table name */

struct outsect
{
	const Section	*secp;	/* internal version of section */
	Elf_Scn		*scn;	/* elf's handle for the section */
	ETYPE_Shdr	*shdr;	/* the section header for the section */
	Elf_Data	*data;	/* all the data for the section */
};

static struct outsect *outsect_list;	/* all elf sections */

static ETYPE_Word
#ifdef __STDC__
addstr(const Uchar *str, size_t len)	/* add str to string table */
#else
addstr(str, len)Uchar *str; size_t len;
#endif
{
	ETYPE_Word cur;

	if (strtab.used + len > strtab.size)
	{
		fatal(gettxt(":842","addstr():strtab too small, need %lu, max %lu"),
			(Ulong)(strtab.used + len), (Ulong)strtab.size);
	}
	cur = strtab.used;
	(void)memcpy((void *)strtab.next, (const void *)str, len);
	strtab.next += len;
	strtab.used += len;
	return cur;
}

static void
#ifdef __STDC__
startstrs(void)	/* start string table section */
#else
startstrs()
#endif
{
	strtab.size++;	/* \0 at beginning of table */
#ifdef DEBUG
	if (DEBUG('o') > 0)
	{
		(void)fprintf(stderr, "String table size=%lu\n",
			(Ulong)strtab.size);
	}
#endif
	strtab.secp = lookup(MSGstrtab, sizeof(MSGstrtab) - 1)->sym_sect;
	strtab.base = (Uchar *)alloc((void *)0, sizeof(Uchar) * strtab.size);
	*strtab.base = '\0';
	strtab.next = 1 + strtab.base;
	strtab.used = 1;
}

#ifdef DEBUG

static void
#ifdef __STDC__
prtsec(struct outsect *osp)	/* print section entry */
#else
prtsec(osp)struct outsect *osp;
#endif
{
	(void)fprintf(stderr, "[%.2lu]:nm=%#-8lx ty=%#-10lx fl=%#-10lx",
		(Ulong)(osp - outsect_list), (Ulong)osp->shdr->sh_name,
		(Ulong)osp->shdr->sh_type, (Ulong)osp->shdr->sh_flags);
	(void)fprintf(stderr, " lk=%#-10lx in=%#-10lx es=%#-10lx %s\n",
		(Ulong)osp->shdr->sh_link, (Ulong)osp->shdr->sh_info,
		(Ulong)osp->shdr->sh_entsize,
		osp->shdr->sh_name + (char *)strtab.base);
}

static void
#ifdef __STDC__
prtsym(ETYPE_Sym *symp)	/* print symbol table entry */
#else
prtsym(symp)ETYPE_Sym *symp;
#endif
{
	(void)fprintf(stderr, "[%.3lu]:nm=%#-8lx vl=%#-10lx",
		(Ulong)(symp - symtab.base), (Ulong)symp->st_name,
		(Ulong)symp->st_value);
	(void)fprintf(stderr, " sz=%#-10lx in=%#-4x sx=%#-6x %s\n",
		(Ulong)symp->st_size, (Uint)symp->st_info,
		(Uint)symp->st_shndx, symp->st_name + (char *)strtab.base);
}

#endif /*DEBUG*/

static void
#ifdef __STDC__
startsyms(void)	/* start symbol table section */
#else
startsyms()
#endif
{
	static const ETYPE_Sym empty = {0};
	register ETYPE_Sym *symp;

	num_of.symbols++;	/* empty entry at beginning of table */
#ifdef DEBUG
	if (DEBUG('o') > 0)
	{
		(void)fprintf(stderr,
			"Symbols:empty%s+(usersect:%lu)+(local:%lu)+%lu=%lu",
			srcf_ptr == 0 ? "" : "+file", (Ulong)num_of.user_sect,
			(Ulong)num_of.local_sym, (Ulong)num_of.nonloc_sym,
			(Ulong)num_of.symbols);
		if (srcf_ptr == 0)
			(void)putc('\n', stderr);
		else
		{
			(void)fprintf(stderr, ", file=\"%s\"\n",
				prtstr(srcf_ptr, srcf_len - 1));
		}
	}
#endif
	if (1 + (srcf_ptr != 0) + num_of.user_sect
		+ num_of.local_sym + num_of.nonloc_sym != num_of.symbols)
	{
		fatal(gettxt(":843","startsyms():wrong number of symbols"));
	}
	symtab.secp = lookup(MSGsymtab, sizeof(MSGsymtab) - 1)->sym_sect;
	symp = (ETYPE_Sym *)alloc((void *)0,
		sizeof(ETYPE_Sym) * num_of.symbols);
	symtab.base = symp;
	*symp = empty;
	if (srcf_ptr != 0)
	{
		symp++;
		symp->st_name = addstr(srcf_ptr, srcf_len);
		symp->st_value = 0;
		symp->st_size = 0;
		symp->st_info = EMACR_ST_INFO(STB_LOCAL, STT_FILE);
		symp->st_other = 0;
		symp->st_shndx = SHN_ABS;
#ifdef DEBUG
		if (DEBUG('o') > 1)
			prtsym(symp);
#endif
	}
}

int
#ifdef __STDC__
openobjf(const char *name)	/* start object file processing */
#else
openobjf(name)char *name;
#endif
{
	register ETYPE_Ehdr *ehdr;
	int fd;

#ifdef DEBUG
	if (DEBUG('o') > 0)
	{
		(void)fprintf(stderr, "openobjf(%s)\n", name);
		(void)fprintf(stderr,
			"Sections:empty+2*(user:%lu)+symtab+strtab=%lu\n",
			(Ulong)num_of.user_sect, (Ulong)num_of.sections + 1);
	}
#endif
	if (num_of.sections != 2 * num_of.user_sect + 2)
		fatal(gettxt(":844","openobjf():wrong number of sections"));
	/* Try opening read + write first so that libelf.a (elf_update())
	   can use mmap() */
	if ((fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0
	     && (fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0)
	{
		error(gettxt(":845","cannot open output file: %s"), name);
		return 0;
	}
	/*
	* Get all libelf startup stuff in order.
	*/
	if (elf_version(EV_CURRENT) == EV_NONE)
	{
		error(gettxt(":846","object file library/assembler version mismatch"));
		return 1;
	}
	if ((elf = elf_begin(fd, ELF_C_WRITE, (Elf *)0)) == 0)
	{
		error(gettxt(":847","cannot begin output file: %s"), name);
		return 1;
	}
	elf_ehdr = ehdr = EFUNC_newehdr(elf);
	ehdr->e_ident[EI_DATA] = TARGET_DATA;
	ehdr->e_type = ET_REL;
	ehdr->e_machine = TARGET_MACHINE;
	ehdr->e_version = EV_CURRENT;
	ehdr->e_entry = 0;
	ehdr->e_flags = TARGET_EFLAGS;
	startstrs();
	startsyms();
	outsect_list = (struct outsect *)alloc((void *)0,
		(num_of.sections + 1) * sizeof(struct outsect));
	return 1;
}

static struct outsect *
#ifdef __STDC__
getoutsect(size_t index, size_t name)	/* fill outsect with elf stuff */
#else
getoutsect(index, name)size_t index, name;
#endif
{
	register struct outsect *osp;

	osp = &outsect_list[index];
	if ((osp->scn = elf_newscn(elf)) == 0)
	{
		error(gettxt(":848","cannot create section: %s"),
			name + (char *)strtab.base);
		return 0;
	}
	if ((osp->data = elf_newdata(osp->scn)) == 0)
	{
		error(gettxt(":849","cannot create data block for section: %s"),
			name + (char *)strtab.base);
		return 0;
	}
	if ((osp->shdr = EFUNC_getshdr(osp->scn)) == 0)
	{
		error(gettxt(":850","cannot create section header for section: %s"),
			name + (char *)strtab.base);
		return 0;
	}
	osp->data->d_version = EV_CURRENT;
	osp->shdr->sh_name = name;
	osp->shdr->sh_addr = 0;
	return osp;
}

static void
#ifdef __STDC__
genrel(struct outsect *osp, const Section *secp) /* generate rel section */
#else
genrel(osp, secp)struct outsect *osp; Section *secp;
#endif
{
	register ETYPE_Rel *rp;
	register Ulong symndx;
	register const Code *cp;
	int sectbase = srcf_ptr != 0;
	size_t locbase = symtab.local_base;
	size_t nonlocbase = symtab.nonloc_base;
	Elf_Data *dp;
	size_t sz;

	osp->shdr->sh_entsize = EFUNC_fsize(ELF_T_REL, (size_t)1, EV_CURRENT);
	sz = secp->sec_size * sizeof(ETYPE_Rel);
	rp = (ETYPE_Rel *)alloc((void *)0, sz);
	dp = osp->data;
	dp->d_buf = (Elf_Void *)rp;
	dp->d_type = ELF_T_REL;
	dp->d_size = sz;
	/*
	* Fill in the relocation entries.
	*/
	cp = secp->sec_code;
	for (; cp->code_kind != CodeKind_Unset; cp = cp->code_next)
	{
		if (cp->code_addr & ~MASK(TARGET_ADDR_BIT))
		{
			error(gettxt(":851","address for relocation out of range: %#lx"),
				cp->code_addr);
		}
		rp->r_offset = cp->code_addr;
		/*
		* If the section or symbol pointer is null, base
		* the relocation off of the special zero symbol.
		*/
		if (cp->code_kind == CodeKind_RelSec)
		{
			register const Section *sec;

			if ((sec = cp->info.code_sec) == 0)
				symndx = 0;		/* STN_UNDEF */
			else
				symndx = sec->sec_index + sectbase;
		}
		else
		{
			register const Symbol *sp;

			if ((sp = cp->info.code_sym) == 0)
				symndx = 0;		/* STN_UNDEF */
			else
			{
				symndx = sp->sym_index;
				if (sp->sym_bind == STB_LOCAL)
					symndx += locbase;
				else
					symndx += nonlocbase;
			}
		}
		if (cp->code_size & ~MASK(TARGET_RELT_BIT))
		{
			fatal(gettxt(":852","genrel():relocation type out of range: %#lx"),
				(Ulong)cp->code_size);
		}
		if (symndx & ~MASK(TARGET_RELS_BIT))
		{
			fatal(gettxt(":853","genrel():reloc symbol index out of range: %#lx"),
				symndx);
		}
		rp->r_info = EMACR_R_INFO(symndx, cp->code_size);
		rp++;
	}
}

static void
#ifdef __STDC__
genrela(struct outsect *osp, const Section *secp) /* generate rela section */
#else
genrela(osp, secp)struct outsect *osp; Section *secp;
#endif
{
	register ETYPE_Rela *rp;
	register Ulong symndx;
	register const Code *cp;
	int sectbase = srcf_ptr != 0;
	size_t locbase = symtab.local_base;
	size_t nonlocbase = symtab.nonloc_base;
	Elf_Data *dp;
	size_t sz;

	osp->shdr->sh_entsize = EFUNC_fsize(ELF_T_RELA, (size_t)1, EV_CURRENT);
	sz = secp->sec_size * sizeof(ETYPE_Rela);
	rp = (ETYPE_Rela *)alloc((void *)0, sz);
	dp = osp->data;
	dp->d_buf = (Elf_Void *)rp;
	dp->d_type = ELF_T_RELA;
	dp->d_size = sz;
	/*
	* Fill in the relocation entries.
	*/
	cp = secp->sec_code;
	for (; cp->code_kind != CodeKind_Unset; cp = cp->code_next)
	{
		if (cp->code_addr & ~MASK(TARGET_ADDR_BIT))
		{
			error(gettxt(":851","address for relocation out of range: %#lx"),
				cp->code_addr);
		}
		rp->r_offset = cp->code_addr;
		/*
		* If the section or symbol pointer is null, base
		* the relocation off of the special zero symbol.
		*/
		if (cp->code_kind == CodeKind_RelSec)
		{
			register const Section *sec;

			if ((sec = cp->info.code_sec) == 0)
				symndx = 0;		/* STN_UNDEF */
			else
				symndx = sec->sec_index + sectbase;
		}
		else
		{
			register const Symbol *sp;

			if ((sp = cp->info.code_sym) == 0)
				symndx = 0;		/* STN_UNDEF */
			else
			{
				symndx = sp->sym_index;
				if (sp->sym_bind == STB_LOCAL)
					symndx += locbase;
				else
					symndx += nonlocbase;
			}
		}
		if (cp->code_size & ~MASK(TARGET_RELT_BIT))
		{
			fatal(gettxt(":854","genrela():relocation type out of range: %#lx"),
				(Ulong)cp->code_size);
		}
		if (symndx & ~MASK(TARGET_RELS_BIT))
		{
			fatal(gettxt(":855","genrela():reloc symbol index out of range: %#lx"),
				symndx);
		}
		rp->r_info = EMACR_R_INFO(symndx, cp->code_size);
		rp->r_addend = cp->data.code_addend;
		rp++;
	}
}

static int
#ifdef __STDC__
relsection(register const Section *secp, size_t index)	/* matching reloc */
#else
relsection(secp, index)register Section *secp; size_t index;
#endif
{
	register ETYPE_Shdr *shp;
	register struct outsect *osp;

#ifdef DEBUG
	if (DEBUG('o') > 0)
	{
		(void)fprintf(stderr, "relsection(%s,index=%lu)\n",
			outsect_list[secp->sec_index].shdr->sh_name
			- TARGET_RELNMSZ + (char *)strtab.base,
			(Ulong)index);
	}
#endif
	/*
	* Each section with a matching relocation section has
	* already created its string table entry, TARGET_RELNMSZ
	* characters before the user section's name.
	*/
	if ((osp = getoutsect(index,
		outsect_list[secp->sec_index].shdr->sh_name
		- TARGET_RELNMSZ)) == 0)
	{
		return 0;
	}
	shp = osp->shdr;
	shp->sh_link = symtab.index;
	shp->sh_info = secp->sec_index;
	secp = secp->sec_relo;
	osp->secp = secp;
	shp->sh_type = secp->type.sec_value;
	shp->sh_flags = secp->attr.sec_value;
	osp->data->d_align = TARGET_SYMREL_ALIGN;
	if (secp->type.sec_value == SHT_REL)
		genrel(osp, secp);
	else
		genrela(osp, secp);
	return 1;
}

void
#ifdef __STDC__
closeobjf(void)		/* finish processing of object file */
#else
closeobjf()
#endif
{
	size_t ns, nr;
	size_t str_index;
	struct outsect *osp;
	register ETYPE_Shdr *shp;
	register Elf_Data *dp;
	register Symbol *sp;

#ifdef DEBUG
	if (DEBUG('o') > 0)
		(void)fputs("closeobj()\n", stderr);
#endif
	symtab.index = num_of.user_sect + num_of.rel_sect + 1;
	if ((str_index = symtab.index + 1) >= SHN_LORESERVE)
	{
		error(gettxt(":856","too many total sections, max %u"),
			(Uint)SHN_LORESERVE - 1);
		return;
	}
	elf_ehdr->e_shstrndx = str_index;
	/*
	* Generate contents of relocation sections.
	*/
	nr = num_of.user_sect;
	for (ns = 0; ++ns <= num_of.user_sect;)
	{
		register const Section *secp = outsect_list[ns].secp;

		if (secp == 0)
			fatal(gettxt(":857","closeobjf():missing section: %lu"), (Ulong)ns);
		if (secp->sec_relo != 0 && relsection(secp, ++nr) == 0)
			return;
	}
	/*
	* Instantiate symbol table.
	*/
	sp = symtab.secp->sec_sym;
	if ((osp = getoutsect(symtab.index,
		addstr(sp->sym_name, sp->sym_nlen + 1))) == 0)
	{
		return;
	}
	shp = osp->shdr;
	shp->sh_type = symtab.secp->type.sec_value;
	shp->sh_flags = symtab.secp->attr.sec_value;
	shp->sh_link = str_index;
	shp->sh_info = symtab.nonloc_base + 1;	/* +1 due to 1..n sym_index */
	shp->sh_entsize = EFUNC_fsize(ELF_T_SYM, (size_t)1, EV_CURRENT);
	dp = osp->data;
	dp->d_buf = (Elf_Void *)symtab.base;
	dp->d_type = ELF_T_SYM;
	dp->d_size = num_of.symbols * sizeof(ETYPE_Sym);
	dp->d_align = TARGET_SYMREL_ALIGN;
	/*
	* Instantiate string table.
	*/
	sp = strtab.secp->sec_sym;
	if ((osp = getoutsect(str_index,
		addstr(sp->sym_name, sp->sym_nlen + 1))) == 0)
	{
		return;
	}
	shp = osp->shdr;
	shp->sh_type = strtab.secp->type.sec_value;
	shp->sh_flags = strtab.secp->attr.sec_value;
	shp->sh_link = 0;
	shp->sh_info = 0;
	shp->sh_entsize = sizeof(Uchar);
	dp = osp->data;
	dp->d_buf = (Elf_Void *)strtab.base;
	dp->d_type = ELF_T_BYTE;
	dp->d_size = strtab.used * sizeof(Uchar);
	dp->d_align = 0;
	/*
	* Ship it.
	*/
	if (elf_update(elf, ELF_C_IMPURE_WRITE) < 0)
		error(gettxt(":858","cannot write output file"));
	elf_end(elf);
}

void
#ifdef __STDC__
objfsection(register const Section *secp) /* create section entry */
#else
objfsection(secp)register Section *secp;
#endif
{
	register ETYPE_Shdr *shp;
	register ETYPE_Sym *symp;
	register struct outsect *osp;
	register Elf_Data *dp;
	Symbol *sp;

	sp = secp->sec_sym;
#ifdef DEBUG
	if (DEBUG('o') > 0)
		(void)fprintf(stderr, "objfsection(%s)\n", sp->sym_name);
#endif
	/*
	* A little tricky here--for sections that have a matching
	* relocation section, since the name of the section is
	* the end of the relocation section's name, share the name.
	*/
	if (secp->sec_relo != 0)
	{
		num_of.rel_sect++;
		(void)addstr((const Uchar *)".rela", (size_t)TARGET_RELNMSZ);
	}
	if ((osp = getoutsect(secp->sec_index,
		addstr(sp->sym_name, sp->sym_nlen + 1))) == 0)
	{
		return;
	}
	osp->secp = secp;
	shp = osp->shdr;
	if (secp->type.sec_value & ~MASK(TARGET_WORD_BIT))
	{
		error(gettxt(":859","type for section \"%s\" out of range: %#lx"),
			(const char *)secp->sec_sym->sym_name,
			secp->type.sec_value);
	}
	shp->sh_type = secp->type.sec_value;
	if (secp->attr.sec_value & ~MASK(TARGET_WORD_BIT))
	{
		error(gettxt(":860","attribute for section \"%s\" out of range: %#lx"),
			(const char *)secp->sec_sym->sym_name,
			secp->attr.sec_value);
	}
	shp->sh_flags = secp->attr.sec_value;
	shp->sh_entsize = 0;
	shp->sh_link = 0;
	shp->sh_info = 0;
	switch (secp->type.sec_value)	/* those that need link/info */
	{
	case SHT_DYNAMIC:
	case SHT_HASH:
	case SHT_REL:
	case SHT_RELA:
	case SHT_SYMTAB:
	case SHT_DYNSYM:
		warn(gettxt(":861","link and info unset in header for section: %s"),
			(const char *)sp->sym_name);
		break;
	}
	dp = osp->data;
	dp->d_buf = (Elf_Void *)secp->sec_data;
	dp->d_size = secp->sec_size;
	dp->d_align = secp->sec_align;
	dp->d_type = ELF_T_BYTE;
#ifdef DEBUG
	if (DEBUG('o') > 0)
		prtsec(osp);
#endif
	/*
	* Create a corresponding symbol table entry for the section.
	*/
	symp = symtab.sect_base + secp->sec_index + symtab.base;
	symp->st_name = shp->sh_name;	/* share string table entry */
	symp->st_value = 0;
	symp->st_size = 0;
	symp->st_info = EMACR_ST_INFO(STB_LOCAL, STT_SECTION);
	symp->st_other = 0;
	symp->st_shndx = secp->sec_index;
#ifdef DEBUG
	if (DEBUG('o') > 1)
		prtsym(symp);
#endif
}

int
#ifdef __STDC__
objfsrcstr(Expr *ep)		/* note source file string */
#else
objfsrcstr(ep)Expr *ep;
#endif
{
	if (ep->ex_op != ExpOp_LeafString)
		fatal(gettxt(":862","objfsrcstr():not passed string expr"));
	if (srcf_ptr != 0)
		return 0;
	srcf_len = ep->left.ex_len;
	srcf_ptr = savestr(ep->right.ex_str, srcf_len);
	num_of.symbols++;
	symtab.sect_base++;
	symtab.local_base++;
	symtab.nonloc_base++;
	strtab.size += ++srcf_len;	/* include \0 terminator */
	return 1;
}

void
#ifdef __STDC__
objfmksyms(size_t nl, size_t no, size_t nc)	/* set symbol sizes */
#else
objfmksyms(nl, no, nc)size_t nl, no, nc;
#endif
{
	num_of.symbols += nl + no;
	num_of.local_sym = nl;
	num_of.nonloc_sym = no;
	symtab.nonloc_base += nl;
	strtab.size += nc;
}

void
#ifdef __STDC__
objfmksect(size_t ns, size_t nc)	/* allocate section table */
#else
objfmksect(ns, nc)size_t ns, nc;
#endif
{
	if (ns >= SHN_LORESERVE)
		error(gettxt(":863","too many sections, max %u"), (Uint)SHN_LORESERVE - 1);
	/*
	* There can be no more than one relocation section
	* for each user section.  ??VALID ASSUMPTION??
	*/
	num_of.sections = 2 * ns + 2;	/* ... + syms + strs */
	num_of.user_sect = ns;
	num_of.symbols += ns;
	symtab.local_base += ns;
	symtab.nonloc_base += ns;
	/*
	* Since the number of sections actually requiring matching
	* relocation sections is not known, the string table size
	* is the maximum necessary.  For each matching relocation
	* section, an additional TARGET_RELNMSZ characters are put
	* in the string table (just before the other section name).
	*/
	strtab.size += nc + sizeof(MSGsymtab) + sizeof(MSGstrtab)
		+ ns * TARGET_RELNMSZ;	/* see objfsection() */
}

void
#ifdef __STDC__
objfsymbol(register const Symbol *sp)	/* create symbol table entry */
#else
objfsymbol(sp)register Symbol *sp;
#endif
{
	register ETYPE_Sym *symp;
	register size_t base;
	register Expr *ep;
	register Ulong val;	/* holds values before copying to *symp */

	/*
	* In the following code, each value is checked to see whether
	* it is too big for the object file's version of the information.
	* Unfortunately, the construct "if ((val = ...) & 0){...}" is
	* completely "optimized" away (including the assignment!) by
	* an unspecified "almost" C compiler.  Thus, these expressions
	* have been rewritten using comma operators with the hope that
	* these otherwise useless tests can still be eliminated.
	*/
	if (sp->sym_bind == STB_LOCAL)
		base = symtab.local_base;
	else
		base = symtab.nonloc_base;
	base += sp->sym_index;
	symp = base + symtab.base;
	symp->st_name = addstr(sp->sym_name, sp->sym_nlen + 1);
	/*
	* Calculate the value for the symbol.
	* The value for different symbol types vary.
	*/
	switch (sp->sym_kind)
	{
		register Eval *vp;

	case SymKind_Set:
		if (sp->sym_exty == ExpTy_Operand)
		{
			fatal(gettxt(":864","objfsymbol():operand-typed .set: %s"),
				(const char *)sp->sym_name);
		}
		vp = evalexpr(sp->addr.sym_oper->oper_expr);
		/*
		* No reevaluation check needed here since all addresses
		* should be fixed by now.  Allow negative values as long
		* as they fit in TARGET_ADDR_BIT's.
		*/
		if (vp->ev_minbit > TARGET_ADDR_BIT || vp->ev_flags & EV_OFLOW)
		{
			exprerror(vp->ev_expr,
				gettxt(":865","value for \"%s\" out of range: %s"),
				(const char *)sp->sym_name,
				num_tohex(vp->ev_int));
		}
		val = vp->ev_ulong;
		break;
	case SymKind_Common:
		/*
		* The alignment value is placed in the value field.
		*/
		if ((val = sp->align.sym_value), val & ~MASK(TARGET_ADDR_BIT))
		{
			backerror((Ulong)sp->sym_file, sp->sym_line,
				gettxt(":866","alignment for \"%s\" out of range: %#lx"),
				(const char *)sp->sym_name,
				sp->align.sym_value);
		}
		break;
	case SymKind_Dot:
		fatal(gettxt(":867","objfsymbol():dot symbol"));
		/*NOTREACHED*/
	default:
		if ((ep = sp->addr.sym_expr) == 0)	/* undefined symbol */
			val = 0;
		else if (ep->ex_op != ExpOp_LeafCode)
		{
			fatal(gettxt(":868","objfsymbol():non-label regular name: %s"),
				(const char *)sp->sym_name);
		}
		else if ((val = ep->right.ex_code->code_addr),
			val & ~MASK(TARGET_ADDR_BIT))
		{
			backerror((Ulong)sp->sym_file, sp->sym_line,
				gettxt(":869","address for \"%s\" out of range: %#lx"),
				(const char *)sp->sym_name, val);
		}
		break;
	}
	symp->st_value = val;
	if ((val = sp->size.sym_value), val & ~MASK(TARGET_WORD_BIT))
	{
		backerror((Ulong)sp->sym_file, sp->sym_line,
			gettxt(":870","size for \"%s\" out of range: %#lx"),
			(const char *)sp->sym_name, val);
	}
	symp->st_size = val;
	if (sp->sym_bind & ~MASK(TARGET_BIND_BIT))
	{
		fatal(gettxt(":871","objfsymbol():out of range binding: %u"),
			(Uint)sp->sym_bind);
	}
	if ((val = sp->type.sym_value), val & ~MASK(TARGET_SYMT_BIT))
	{
		backerror((Ulong)sp->sym_file, sp->sym_line,
			gettxt(":872","type for \"%s\" out of range: %#lx"),
			(const char *)sp->sym_name, val);
	}
	symp->st_info = EMACR_ST_INFO(sp->sym_bind, val);
	symp->st_other = 0;
	if (sp->sym_exty == ExpTy_Integer)
		symp->st_shndx = SHN_ABS;
	else if (sp->sym_kind == SymKind_Common)
		symp->st_shndx = SHN_COMMON;
	else if (sp->sym_defn == 0)
		symp->st_shndx = SHN_UNDEF;
	else if ((val = sp->sym_defn->sec_index) >= SHN_LORESERVE)
		fatal(gettxt(":873","objfsymbol():section number too big: %lu"), val);
	else
		symp->st_shndx = val;
#ifdef DEBUG
	if (DEBUG('o') > 1)
		prtsym(symp);
#endif
}
