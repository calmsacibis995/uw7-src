#ident	"@(#)pat:patsym.c	1.2"

/*
 *	This module may well seem unnecessarily complicated.
 *	That is mainly to handle the case of multiple symbols
 *	in an archive of multiple objects: we try to avoid
 *	revisiting an object repeatedly for different symbols.
 *	(But we fail to avoid searching repeatedly for the
 *	same symbol: would be complicated by CloserSym check.)
 *	It is also complicated by the need to patch local
 *	symbols in a running kernel: binfile is used to find
 *	offset from nearby global, getksym() to locate global.
 */

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <libelf.h>
#include <ar.h>
#include "pat.h"

/*
 * Avoid sys header files specific to UnixWare or OpenServer5,
 * since either might lead to build complications on the other
 */
#define KVBASE		0xC0000000	/* UW and OSr5 default KVBASE */
#define PSMEND		0x00020000	/* rough guess at PSM address limit */
#define MAXSYMNMLEN	1024		/* from sys/ksym.h */

#define ARHDR		sizeof(struct ar_hdr)

/*
 * Error messages
 */
static const char ArchEmpty[]	= "Archive index is empty";
static const char NoSymtab[]	= "Symbol table not found";
static const char SymtabEmpty[]	= "Symbol table is empty";
static const char SymAnother[]	= "Symbol belongs to another file";
static const char SymMissed[]	= "Symbol missed from archive index";
static const char MoreThanOne[]	= "More than one local of that name";
static const char CloserSym[]	= "Closer symbol must be used";
static const char NoGlobal[]	= "No global found in that section";
static const char NotFound[]	= "Not found in symbol table";
static const char NotKerSym[]	= "Not found in running kernel";
static const char KmemUser[]	= "Cannot patch kmem at user address";
static const char ExtentWraps[]	= "Extent beyond address range";
static const char ExtentLimit[] = "Extent beyond section range";
static const char Unpatchable[]	= "No data in file to be patched";
static const char MemoffMoved[]	= "ed_memoff has moved";
static const char BadVersion[]	= "Incompatible version";
static const char Unsuitable[]	= "Unsuitable for -n option";
static const char NotElfCoff[]	= "Neither ELF nor COFF format";
static       char Libelf[]	= "libelf";

int global_bss;				/* test Unpatchable */
int global_data = 0x01010101;		/* test Successful */
const char *patsym();			/* test Successful */
static int local_bss;			/* test Unpatchable */
static int local_data = 0x02020202;	/* test Successful */
static const char *kersym();		/* test Successful */

static char *
symname_offset(patch_t *patp)		/* reattach offset for some errmsgs */
{
	if (patp->plusoffset)
		patp->symname[strlen(patp->symname)] = '+';
	return	patp->symname;
}

static const char *
kersym(patch_t *patp, char *globname)
{
	unsigned long	plusoffset, maxoffset, bot, top, info;
	char symname	[MAXSYMNMLEN];
	int		rv1, rv2;

	plusoffset = patp->offset + patp->plusoffset;
	patp->offset = 0;

	if (getksym(globname, &patp->offset, &info) != 0
	||  patp->offset == 0)
		return NotKerSym;

	if (patp->offset < PSMEND)
		patp->offset += KVBASE;

	if (plusoffset) {
		bot = info = patp->offset;
		top = patp->offset += plusoffset;
		if (top < KVBASE)
			return NULL;		/* fail at higher level */

		if (top < bot) {		/* allow negv offset */
			top = bot - 1;
			bot = patp->offset - 1;
		}
		maxoffset = PSMEND;		/* for failure case */
		if (top - bot <= 0-KVBASE) {
			rv1 = getksym(symname, &info, &info);
			rv2 = getksym(symname, &top, &maxoffset);
		}
		if (rv1 != rv2 || top - bot > maxoffset) {
			(void) symname_offset(patp);
			return CloserSym;
		}
	}

	return NULL;
}

static const char *
elfsymbols(Elf *elf, patch_t *opatp, char **badnamep, unsigned long archoffset, int namekmem)
{
	Elf_Scn		*scnp;
	Elf32_Shdr	*shdrp;
	Elf_Data	*datap;
	Elf32_Sym	*symp;
	Elf32_Sym	*esymp;
	Elf32_Sym	*botp, *topp;
	patch_t		*patp;
	char		*symname;
const	char		*errmsg;
	unsigned long	bot, top;
	int		local, found;

	scnp = NULL;
	while ((scnp = elf_nextscn(elf, scnp)) != NULL) {
		if ((shdrp = elf32_getshdr(scnp)) == NULL
		||   shdrp->sh_type == SHT_SYMTAB)
			break;
	}
	if (scnp == NULL)
		return opatp->search_all? NULL: NoSymtab;
	if (shdrp == NULL
	|| (datap = elf_getdata(scnp, NULL)) == NULL)
		return opatp->search_all? NULL: elf_errmsg(-1);
	if (datap->d_size == 0)
		return opatp->search_all? NULL: SymtabEmpty;

	esymp = (Elf32_Sym *)((size_t)datap->d_buf + datap->d_size);

	for (patp = opatp; patp; patp = patp->next) {
		if (opatp->search_all) {
			if (!patp->search_all)
				continue;
		}
		else if (patp->got_offset || patp->archoffset != archoffset)
			continue;
		found = 0;
		local = (patp->sectindex != 0);	/* carry over if search_all */
		symp = (Elf32_Sym *)datap->d_buf;
		while (++symp < esymp) {	/* skip dummy first entry */
			if (!symp->st_name)
				continue;
			switch (ELF32_ST_TYPE(symp->st_info)) {
			case STT_NOTYPE:
			case STT_OBJECT:
			case STT_FUNC:
				break;
			default:
				continue;
			}
			symname = elf_strptr(elf,shdrp->sh_link,symp->st_name);
			if (symname == NULL)
				return elf_errmsg(-1);
			if (symname[0] != patp->symname[0]
			||  strcmp(symname, patp->symname) != 0)
				continue;
			if (symp->st_shndx == SHN_UNDEF /* 0 */) {
				*badnamep = patp->symname;
				return SymAnother;
			}
			found = 1;
			patp->sectindex = symp->st_shndx;
			patp->archoffset = archoffset;
			patp->offset = symp->st_value;
			if (ELF32_ST_BIND(symp->st_info) != STB_LOCAL) {
				if (patp->search_all) {
					*badnamep = patp->symname;
					return SymMissed;
				}
				if (namekmem == UWKMEM) {
					*badnamep = patp->symname;
					return NotKerSym;
				}
				local = 0;
				break;
			}
			if (local++ && patp->search_all)
				break;
		}
		if (local > 1) {
			*badnamep = patp->symname;
			return MoreThanOne;
		}
		if (!found) {
			if (patp->search_all)
				continue;
			*badnamep = patp->symname;
			return NotFound;
		}
		if (namekmem)
			patp->got_offset = 1;
		if (!patp->plusoffset && namekmem != UWKMEM)
			continue;
		bot = patp->offset + 1;
		top = patp->offset += patp->plusoffset;
		if (patp->plusoffset && top < bot) {	/* allow negv offset */
			top = bot - 2;
			bot = patp->offset;
		}
		botp = topp = NULL;
		symp = (Elf32_Sym *)datap->d_buf;
		while (++symp < esymp) {	/* skip dummy first entry */
			if (symp->st_shndx != patp->sectindex
			||  ELF32_ST_BIND(symp->st_info) != STB_GLOBAL
			||  !symp->st_name)
				continue;
			switch (ELF32_ST_TYPE(symp->st_info)) {
			case STT_NOTYPE:
			case STT_OBJECT:
			case STT_FUNC:
				break;
			default:
				continue;
			}
			if (symp->st_value >= bot
			&&  symp->st_value <= top) {
				*badnamep = symname_offset(patp);
				return CloserSym;
			}
			if (namekmem != UWKMEM)
				continue;
			if (symp->st_value <= patp->offset) {
				if (!botp || symp->st_value > botp->st_value)
					botp = symp;
			}
			else {
				if (!topp || symp->st_value < topp->st_value)
					topp = symp;
			}
		}
		if (namekmem == UWKMEM) {
			if (!(symp = botp) && !(symp = topp)) {
				*badnamep = patp->symname;
				return NoGlobal;
			}
			symname = elf_strptr(elf,shdrp->sh_link,symp->st_name);
			if (symname == NULL)
				return elf_errmsg(-1);
			patp->offset -= patp->plusoffset + symp->st_value;
			if ((errmsg = kersym(patp, symname)) != NULL) {
				*badnamep = patp->symname;
				return errmsg;
			}
		}
	}

	return NULL;
}

static const char *
elfoffsets(Elf *elf, patch_t *opatp, char **badnamep, unsigned long archoffset, int execable)
{
	Elf_Scn		*scnp;
	Elf32_Shdr	*shdrp;
	patch_t		*patp;

	for (patp = opatp; patp; patp = patp->next) {
		if (patp->got_offset || patp->archoffset != archoffset)
			continue;
		if (patp->sectindex >= SHN_LORESERVE) {
			*badnamep = patp->symname;
			return Unpatchable;
		}
		if ((scnp = elf_getscn(elf, patp->sectindex)) == NULL)
			return elf_errmsg(-1);
		if ((shdrp = elf32_getshdr(scnp)) == NULL)
			return elf_errmsg(-1);
		for (opatp = patp; patp; patp = patp->next) {
			if (patp->sectindex != opatp->sectindex
			||  patp->archoffset != archoffset)
				continue;
			if (shdrp->sh_type != SHT_PROGBITS) {
				*badnamep = patp->symname;
				return Unpatchable;
			}
			if (execable) /* COFF seems to need that condition */
				patp->offset -= shdrp->sh_addr;
			patp->offset += shdrp->sh_offset;
			if (patp->offset < shdrp->sh_offset
			||  patp->offset + patp->length >
			    shdrp->sh_offset + shdrp->sh_size) {
				*badnamep = symname_offset(patp);
				return ExtentLimit;
			}
			patp->offset += archoffset;
			patp->got_offset = 1;
		}
		patp = opatp;
	}

	return NULL;
}

static const char *
doarchive(int nfd, Elf *arf, patch_t *opatp, char **badnamep, int namekmem)
{
	Elf_Arsym	*oarsymp;
	Elf_Arsym	*arsymp;
	Elf		*elf;
	patch_t		*patp;
	size_t		nents;
	unsigned long	archoffset;
const	char		*errmsg = NULL;

	if ((oarsymp = elf_getarsym(arf, &nents)) == NULL)
		return elf_errmsg(-1);
	if (nents == 0 || oarsymp->as_name == NULL)
		return ArchEmpty;

	for (patp = opatp; patp; patp = patp->next) {
		if (patp->got_offset)
			continue;
		for (arsymp = oarsymp; ; arsymp++) {
			if (arsymp->as_name == NULL) {
				/* presumably the symbol is local */
				patp->search_all = 1;
				break;
			}
			if (arsymp->as_name[0] == patp->symname[0]
			&&  strcmp(arsymp->as_name, patp->symname) == 0) {
				if (namekmem == UWKMEM) {
					*badnamep = patp->symname;
					return NotKerSym;
				}
				patp->archoffset = arsymp->as_off + ARHDR;
				break;
			}
		}
	}

	for (patp = opatp; patp; patp = patp->next) {
		if (patp->got_offset || patp->search_all)
			continue;
		if (elf_rand(arf, patp->archoffset - ARHDR) !=
				  patp->archoffset - ARHDR)
			return elf_errmsg(-1);
		if ((elf = elf_begin(nfd, ELF_C_READ, arf)) == NULL)
			return elf_errmsg(-1);
		if ((errmsg = elfsymbols(elf, patp, badnamep,
			patp->archoffset, namekmem)) == NULL && !namekmem)
			errmsg = elfoffsets(elf, patp, badnamep,
				patp->archoffset, 0);
		(void)elf_end(elf);
		if (errmsg != NULL)
			return errmsg;
	}

	for (patp = opatp; ; patp = patp->next) {
		if (patp == NULL)
			return NULL;
		if (patp->search_all)
			break;
	}

	if (elf_rand(arf, SARMAG) != SARMAG)
		return elf_errmsg(-1);
	while ((elf = elf_begin(nfd, ELF_C_READ, arf)) != NULL) {
		static int once;
		archoffset = ((size_t *)elf)[5]; /* ed_memoff */
		if (!once++ && archoffset != SARMAG + ARHDR) {
			*badnamep = Libelf;
			errmsg = MemoffMoved;	/* internal error: fix pat */
		}
		else switch (elf_kind(elf)) {
		case ELF_K_ELF:
		case ELF_K_COFF:
			if ((errmsg = elfsymbols(elf, patp, badnamep,
				archoffset, namekmem)) == NULL && !namekmem)
				errmsg = elfoffsets(elf, patp, badnamep,
					archoffset, 0);
			break;
		}
		/* elf_next() seems to be unnecessary */
		(void)elf_end(elf);
		if (errmsg != NULL)
			return errmsg;
	}

	return NULL;
}

const char *
patsym(char *binfile, patch_t *opatp, char **badnamep, int namekmem)
{
	int		nfd, execable;
	Elf		*arf;	/* which may turn out to be elf */
	Elf32_Ehdr	*ehdrp;
	patch_t		*patp;
const	char		*errmsg;

	for (patp = opatp; patp; patp = patp->next) {
		if (patp->symname[0] == '0') {
			patp->got_offset = 1;
			patp->offset += patp->plusoffset;
			if (opatp == patp)
				opatp = patp->next;
		}
		else if (namekmem == UWKMEM) {
			if ((errmsg = kersym(patp, patp->symname)) == NULL) {
				patp->got_offset = 1;
				if (opatp == patp)
					opatp = patp->next;
			}
			else if (errmsg != NotKerSym) {
				*badnamep = patp->symname;
				return errmsg;
			}
			else if (errno == ENOSYS)
				namekmem = O5KMEM;
		}
		if (patp->got_offset) {
			if (patp->offset + patp->length < patp->offset) {
				/* off by one but saves pain elsewhere */
				*badnamep = symname_offset(patp);
				return ExtentWraps;
			}
			if (namekmem && patp->offset < KVBASE) {
				*badnamep = symname_offset(patp);
				return KmemUser;
			}
		}
	}

	if ((patp = opatp) == NULL)	/* all found without binfile */
		return NULL;
	if ((nfd = open(binfile, O_RDONLY, 0)) < 0)
		return strerror(errno);
	if (elf_version(EV_CURRENT) == EV_NONE) {
		close(nfd);
		*badnamep = Libelf;
		return BadVersion;	/* internal error: fix pat */
	}
	if ((arf = elf_begin(nfd, ELF_C_READ, NULL)) == NULL) {
		close(nfd);
		return elf_errmsg(-1);
	}

	switch (elf_kind(arf)) {
	case ELF_K_ELF:
	case ELF_K_COFF:
		if ((ehdrp = elf32_getehdr(arf)) == NULL) {
			errmsg = elf_errmsg(-1);
			break;
		}
		execable = (ehdrp->e_type == ET_EXEC);
		errmsg = (namekmem == O5KMEM && !execable)? Unsuitable:
			elfsymbols(arf, patp, badnamep, 0, namekmem);
		if (errmsg == NULL && !namekmem)
			errmsg = elfoffsets(arf, patp, badnamep, 0, execable);
		break;
	case ELF_K_AR:
		errmsg = (namekmem == O5KMEM)? Unsuitable:
			doarchive(nfd, arf, patp, badnamep, namekmem);
		break;
	default:
		errmsg = NotElfCoff;
		break;
	}

	(void)elf_end(arf);
	(void)close(nfd);

	if (errmsg)
		return errmsg;

	for (; patp; patp = patp->next) {
		if (patp->got_offset) {
			if (patp->offset + patp->length < patp->offset) {
				/* off by one but saves pain elsewhere */
				*badnamep = symname_offset(patp);
				return ExtentWraps;
			}
			if (namekmem && patp->offset < KVBASE) {
				*badnamep = symname_offset(patp);
				return KmemUser;
			}
		}
		else {
			*badnamep = patp->symname;
			return NotFound;
		}
	}

	return NULL;
}
