#ident	"@(#)ld:common/sections.c	1.44"
/*
** Module sections
** initialize special sections
*/

/****************************************
** Imports
****************************************/

#include	<string.h>
#ifdef	__STDC__
#include	<unistd.h>
#endif	/* __STDC__ */
#include	"sgs.h"
#include	"link.h"
#include	"globals.h"
#include	"macros.h"

/****************************************
** Local Variable Definitions
****************************************/

static hashsize[] = { 3, 17, 37, 67, 97, 131, 197, 263, 397, NBKTS };

/* Index of the last entry in the hashsize array */
#define	LAST_HASHSIZE	(sizeof(hashsize) / sizeof(hashsize[0]) - 1)


static Shdr bsshdr ={0,SHT_NOBITS,SHF_ALLOC | SHF_WRITE, (Addr)0, (Off)0,
		0, SHN_UNDEF, 0, 0, 0};

static Elf_Data bssdata = { NULL, ELF_T_BYTE, 0, 0, 0, 0 };

static Insect bsstemp = {".bss", &bsshdr, 0,
		 NULL, NULL, &bssdata, {NULL, NULL}, 0, 0, (Addr)0};

Insect* bss_sect = &bsstemp;




static Shdr comhdr = {0, SHT_PROGBITS, 0, 0, (Off)0, 0,
		 SHN_UNDEF, 0, 1, 1};

static Elf_Data comdata = {NULL, ELF_T_BYTE, 0, 0, 0, 0 };

static Insect comtemp = {".comment", &comhdr, 0,
		NULL, NULL, &comdata, {NULL, NULL}, 0, 0, (Addr)0};

static Insect* com_sect = &comtemp;




static Shdr dynshdr = { 0, SHT_DYNAMIC, SHF_ALLOC | SHF_WRITE, (Addr) 0,
		(Off) 0, 0, SHN_UNDEF, 0, WORD_ALIGN, 0 };

static Elf_Data dyndata = { NULL, ELF_T_DYN, 0, 0, WORD_ALIGN, 0};

static Insect dyntemp = {".dynamic", &dynshdr, 0,
		NULL, NULL, &dyndata, { NULL, NULL }, 0, 0, (Addr) 0 };

Insect* dynamic_sect = NULL;



static Shdr hashshdr = {0, SHT_HASH, SHF_ALLOC, (Addr)0, (Off)0,
		0, SHN_UNDEF, 0, sizeof(Word), sizeof(ELF_T_WORD) };

static Elf_Data hashdata = { NULL, ELF_T_WORD, 0, 0, WORD_ALIGN, 0 };

static Insect hashtemp = {".hash", &hashshdr, 0,
		 NULL, NULL, &hashdata, {NULL, NULL}, 0, 0, (Addr)0};

Insect* hash_sect = &hashtemp;



static Shdr gotshdr = {0, SHT_PROGBITS, SHF_ALLOC | SHF_WRITE, (Addr)0, (Off)0,
		0, SHN_UNDEF, 0, 0, WORD_ALIGN};

static Elf_Data gotdata = { NULL, ELF_T_BYTE, 0, 0, WORD_ALIGN, 0 };

static Insect gottemp = {".got", &gotshdr, 0,
		 NULL, NULL, &gotdata, {NULL, NULL}, 0, 0, (Addr)0};

Insect* got_sect = NULL;



static Shdr gotrelshdr = {0, SHT_REL_TYPE, SHF_ALLOC, (Addr)0, (Off)0,
		0, SHN_UNDEF, 0, 0, WORD_ALIGN};

static Elf_Data gotreldata = { NULL, ELF_T_REL_TYPE, 0, 0, WORD_ALIGN, 0 };

static Insect gotreltemp = {REL_GOT, &gotrelshdr, 0,
		 NULL, NULL, &gotreldata, {NULL, NULL}, 0, 0, (Addr)0};

static Insect* gotrel_sect = &gotreltemp;




static Shdr interpshdr = {0, SHT_PROGBITS, 0, (Addr)0, (Off)0,
		0, SHN_UNDEF, 0, 0, 0};

static Elf_Data interpdata = { NULL, ELF_T_BYTE, 0, 0, 0, 0 };

static Insect interptemp = {".interp", &interpshdr, 0,
		 NULL, NULL, &interpdata, {NULL, NULL}, 0, 0, (Addr)0};

Insect* interp_sect = NULL;



static Shdr pltshdr = {0, SHT_PROGBITS, PLT_SHF_PERM, (Addr)0, (Off)0,
		0, SHN_UNDEF, 0, 0, WORD_ALIGN };

static Elf_Data pltdata = { NULL, PLT_TYPE, 0, 0, WORD_ALIGN, 0 };

static Insect plttemp = {".plt", &pltshdr, 0,
		 NULL, NULL, &pltdata, {NULL, NULL}, 0, 0, (Addr)0};

Insect* plt_sect = NULL;



static Shdr pltrelshdr = {0, SHT_REL_TYPE, SHF_ALLOC, (Addr)0, (Off)0,
		0, SHN_UNDEF, 0, 0, WORD_ALIGN};

static Elf_Data pltreldata = { NULL, ELF_T_REL_TYPE, 0, 0, WORD_ALIGN, 0 };

static Insect pltreltemp = {REL_PLT, &pltrelshdr, 0,
		 NULL, NULL, &pltreldata, {NULL, NULL}, 0, 0, (Addr)0};

Insect* pltrel_sect = &pltreltemp;




static Shdr shstrtabshdr = {0, SHT_STRTAB, 0, (Addr)0, (Off)0,
		0, SHN_UNDEF, 0, 0, 0};

static Elf_Data shstrtabdata = { NULL, ELF_T_BYTE, 0, 0, 0, 0 };

static Insect shstrtabtemp = {".shstrtab", &shstrtabshdr, 0,
		 NULL, NULL, &shstrtabdata, {NULL, NULL}, 0, 0, (Addr)0};

Insect* shstrtab_sect = &shstrtabtemp;



static Shdr strtabshdr = {0, SHT_STRTAB, 0, (Addr)0, (Off)0,
		0, SHN_UNDEF, 0, 0, 0};

static Elf_Data strtabdata = { NULL, ELF_T_BYTE, 0, 0, 0, 0 };

static Insect strtabtemp = {".strtab", &strtabshdr, 0,
		 NULL, NULL, &strtabdata, {NULL, NULL}, 0, 0, (Addr)0};

Insect* strtab_sect = &strtabtemp;



static Shdr symtabshdr = {0, SHT_SYMTAB, 0, (Addr)0, (Off)0,
		0, SHN_UNDEF, 0, WORD_ALIGN, 0};

static Elf_Data symtabdata = { NULL, ELF_T_SYM, 0, 0, WORD_ALIGN, 0 };

static Insect symtabtemp = {".symtab", &symtabshdr, 0,
		 NULL, NULL, &symtabdata, {NULL, NULL}, 0, 0, (Addr)0};

Insect* symtab_sect = &symtabtemp;


static Shdr dynstrtabshdr = {0, SHT_STRTAB, SHF_ALLOC, (Addr)0, (Off)0,
		0, SHN_UNDEF, 0, 0, 0 };

static Elf_Data dynstrtabdata = { NULL, ELF_T_BYTE, 0, 0, 0, 0 };

static Insect dynstrtabtemp = {".dynstr", &dynstrtabshdr, 0,
		 NULL, NULL, &dynstrtabdata, {NULL, NULL}, 0, 0, (Addr)0};

Insect* dynstrtab_sect = &dynstrtabtemp;



static Shdr dynsymtabshdr = {0, SHT_DYNSYM, SHF_ALLOC, (Addr)0, (Off)0,
		0, SHN_UNDEF, 0, WORD_ALIGN, 0};

static Elf_Data dynsymtabdata = { NULL, ELF_T_SYM, 0, 0, WORD_ALIGN, 0 };

static Insect dynsymtabtemp = {".dynsym", &dynsymtabshdr, 0,
		 NULL, NULL, &dynsymtabdata, {NULL, NULL}, 0, 0, (Addr)0};

Insect* dynsymtab_sect = &dynsymtabtemp;


/****************************************
** Global Function Definitions
****************************************/

/*
** getIsectByName() : 
*/ 
Insect *getIsectByName(inputSectionsPtr, sectionName)
	List *inputSectionsPtr;
	char *sectionName;
{
	register Listnode *tmpPtr;
	Insect *iPtr;

	for(LIST_TRAVERSE(inputSectionsPtr, tmpPtr, iPtr)){
		if(strcmp(iPtr->is_name, sectionName) == 0)
			return iPtr;
	}
	return ((Insect *)0);
}
/* make_bss()
** Only called if the proper flags are set (aflag || dmode);
** builds a dummy bss section for allocation of tentative definitions.
** sh_name, sh_addr, sh_offset, sh_size fields will NOT be set.
*/
void
make_bss()
{
	/* build a dummy bss section structure */

	register Listnode	*stcp;
	register Ldsym		*sym;
	register Word		h;
	Listnode		*fileRecPtr;
	Infile			*fileDataPtr;

	Word size = 0;

	bss_sect->is_shdr->sh_addralign = bss_align;
	bss_sect->is_rawbits->d_align = bss_align;
	bss_sect->is_rawbits->d_version = libver;

#define SymVal(sym) sym->ls_syment->st_value
#define SymSz(sym) sym->ls_syment->st_size

   if(!Bsortbss) {
	for(h=0; h<NBKTS; h++) {
		for(LIST_TRAVERSE(&symbucket[h],stcp,sym)){
			if((sym->ls_syment->st_shndx == SHN_COMMON)
				&& (sym->ls_deftag >= REF_DEFN)
				&& !sym->ls_not_alloc
				&& (!rflag || sym->ls_Bhide)){
				size = ROUND(size,SymVal(sym)) + SymSz(sym);
			}
		}
	}
   } else {
	/* if Bsortbss is true, for each input .o file assign space 
	** for the commons associated with that file in the .bss
	** section, if one exists, of that file. We do this by incrementing
	** the size of the .bss section. If the .o does not have a 
	** .bss section then assign space for its commons in the bss_sect.
	*/
	
	for (LIST_TRAVERSE(&infile_list, fileRecPtr, fileDataPtr)){
		Insect *iPtr; int esize;
		int assignToBss;
/*
		if(fileDataPtr->fl_e_type == ET_DYN) continue;
*/
		assignToBss = 0;
		iPtr = getIsectByName(&fileDataPtr->fl_insects, ".bss");

		if(iPtr && iPtr->is_outsect_ptr) {
			assignToBss = 1;

			/* File has .bss sections, so save the size of the
			** locals, we need this to calculate 
			** offsets from section start for the commons.
			*/
			esize = iPtr->is_rawbits->d_size;
			iPtr->is_initSize = esize;
		}
		for(h=0; h<NBKTS; h++) {
			for(LIST_TRAVERSE(&symbucket[h],stcp,sym)){
				if((sym->ls_syment->st_shndx == SHN_COMMON)
				&& (sym->ls_deftag >= REF_DEFN) 
				&& (sym->ls_flptr == fileDataPtr)
				&& !sym->ls_not_alloc
				&& (!rflag || sym->ls_Bhide)){
				/* the conditional above makes sure we traverse
				** the symbol table for each input .o file
				*/
				   if(assignToBss)
				     esize=ROUND(esize,SymVal(sym))+SymSz(sym);
				   else 
				     size=ROUND(size,SymVal(sym)) + SymSz(sym);
				}
			}
		}
		if(assignToBss){
			/* Increase size of the section to accomodate
			** the commons associated with that file.
			*/
			iPtr->is_rawbits->d_size = esize;
   			iPtr->is_shdr->sh_size = esize;
		}
	}
#if 0
	/* assign variables that have no assigned file */
	for(h=0; h<NBKTS; h++) {
		for(LIST_TRAVERSE(&symbucket[h],stcp,sym)){
			if((sym->ls_syment->st_shndx == SHN_COMMON)
				&& (sym->ls_deftag >= REF_DEFN)){
				size = ROUND(size,SymVal(sym)) + SymSz(sym);
			}
		}
	}
#endif
   }
   bss_sect->is_rawbits->d_size = size;
   bss_sect->is_shdr->sh_size = size;
   place_section(bss_sect);

   return;
}


/* build a comment section (-Qy option) */
void 
make_comment()
{
	char	*str;
	size_t	size;

	size = strlen(" ld : ") + strlen(SGS) + strlen(SGU_REL) + 1;
	str = mymalloc(size);
	sprintf(str,"%s ld : %s",SGS,SGU_REL);
	com_sect->is_rawbits->d_buf = str;
	com_sect->is_rawbits->d_size = size;
	com_sect->is_rawbits->d_version = libver;
	com_sect->is_shdr->sh_size = size;
	place_section(com_sect);
}



/*
 * Make the dynamic section.
 */
void
make_dyn()
{
	size_t		dyn_entries = 0;
	Listnode	*np1;
	char		*lib;
	Ldsym		*ldsymp;

	for (LIST_TRAVERSE(&soneeded_list, np1, lib)) { /* NEEDED */
		if (Gflag)
			count_strsize += strlen(lib) + 1;
		else
			count_dynstrsize += strlen(lib) + 1;
		dyn_entries++;
	}

	if ((ldsymp = sym_find(INIT_SYM, NOHASH)) != NULL
	    && ldsymp->ls_deftag >= REF_RELOBJ) /* INIT */
		dyn_entries++;

	if ((ldsymp = sym_find(FINI_SYM, NOHASH)) != NULL
	    && ldsymp->ls_deftag >= REF_RELOBJ) /* FINI */
		dyn_entries++;

	if (Gflag && dynoutfile_name != NULL){
		dyn_entries++;
		count_strsize += strlen(dynoutfile_name) + 1;
	}

	if ( !Gflag && (ld_run_path = getenv("LD_RUN_PATH")) != NULL) 
		if(strcmp(ld_run_path,"") != SAME) {
			dyn_entries++;
			count_dynstrsize += strlen(ld_run_path) + 1;
		} else
			ld_run_path = NULL;

	dyn_entries += 5;		/* HASH, STRTAB, SYMTAB, STRSZ, SYMENT */

	if ( textrel)			/* TEXTREL */
		dyn_entries++;

	if (countPLT != PLT_XNumber)	/* PLT, PLTSZ PLTREL, and JMPREL */
		dyn_entries += 4;

	if (count_rela != 0)		/* REL, RELSZ, RELENT */
		dyn_entries += 3;

	if( dmode && !Gflag)
		dyn_entries++;		/* DEBUG */

	if (Bflag_symbolic)
		dyn_entries++;		/* SYMBOLIC */
	if (Bbind_now)
		dyn_entries++;		/* BIND_NOW */

	/* If a C++ program with Exception handling must add up to 5 new
	   entries to the .dynamic section: 
		1) address of the eh_ranges section
	   	2) size of the eh_ranges section
		3) address of the first reloc section for delayed relocs
		4) size of the reloc sections for delayed relocs
		5) type of relocation style (rel vs rela) of the
		   delayed relocations
	*/

	if (eh_ranges_sect)
		dyn_entries += 2;

	if (count_delay_rel)
		dyn_entries += 3;

	dyn_entries++;			/* NULL */

	dynamic_sect = &dyntemp;
	dynamic_sect->is_rawbits->d_buf = (char*) mymalloc(dyn_entries * sizeof(Dyn));
	dynamic_sect->is_rawbits->d_size = dyn_entries * sizeof(Dyn);
	dynamic_sect->is_rawbits->d_version = libver;
	dynamic_sect->is_shdr->sh_entsize = my_elf_fsize(ELF_T_DYN,1,libver);
	dynamic_sect->is_shdr->sh_addralign = WORD_ALIGN;

	if (Gflag)
		dynamic_sect->is_shdr->sh_flags = SHF_ALLOC;
	place_section(dynamic_sect);
}

/* build the global offset table section and its associated relocation entries */
void
make_got(size_gotrels)
	Word	size_gotrels;		/* number of relocations output for the got section */
{
	size_t	gotsz = countGOT * GOTENTSZ;


	got_sect = &gottemp;
	got_sect->is_rawbits->d_buf = mycalloc(gotsz);
	got_sect->is_rawbits->d_size = gotsz;
	got_sect->is_rawbits->d_version = libver;
	got_sect->is_shdr->sh_size = gotsz;

	place_section(got_sect);

	/* make its relocation section if needed */

	if(size_gotrels){
		gotrel_sect->is_rawbits->d_buf = mycalloc(size_gotrels);

		gotrel_sect->is_rawbits->d_size = (size_t)size_gotrels;
		gotrel_sect->is_rawbits->d_version = libver;
		gotrel_sect->is_shdr->sh_size = size_gotrels;

		place_section(gotrel_sect);

		got_sect->is_outsect_ptr->os_outrels = gotrel_sect;
		got_sect->is_outsect_ptr->os_szoutrel = size_gotrels;
	}
}

/* Make the hash table */
void
make_hash()
{
	size_t	size;
	size_t	nsyms;

	/* do place_section first since it will affect count_outlocs */

	place_section(hash_sect);

	/* In the following code 
	 * The constant 2 [or 1] is:
	 *	1 for the null first entry in the symbol table (always present)
	 *	1 for the shstrtab entry (only if locals present)
	 *	1 for the output FILE symbol (only if locals present)
	 */

	if(Gflag) {
		nsyms = count_outglobs + 1;
		if (!xflag) {
			nsyms += count_hidden + count_outlocs + 2;
		}
	}
	else {
		nsyms = count_dynglobs + 1;
	}
	/* The constant 2 is for the nbucket, nchain entries in the hash table.
	 */
	size = my_elf_fsize(ELF_T_WORD, (size_t)(nsyms + 2 + dynbkts),
		libver);
	hash_sect->is_rawbits->d_buf = (char*) mycalloc(size);
	hash_sect->is_rawbits->d_size = size;
	hash_sect->is_shdr->sh_size = size;
	hash_sect->is_rawbits->d_version = libver;
}



/* build an interp section */
void
make_interp()
{
	char	*iname;
	size_t	size;


	interp_sect = &interptemp;
	if(interp_path != NULL)
		iname = interp_path;
	else 
		iname = INTERP_DEFAULT;

	size = strlen(iname)+1;
	interp_sect->is_rawbits->d_buf = (char *) mymalloc(size);

	(void)strcpy((char*)interp_sect->is_rawbits->d_buf,iname);
	interp_sect->is_rawbits->d_size = size;
	interp_sect->is_rawbits->d_version = libver;
	interp_sect->is_shdr->sh_size = size;

	place_section(interp_sect);
}

	
/* build the procedure linkage table section and its associated relocation entries */
void
make_plt(size_pltrels)
	Word	size_pltrels;		/* number of relocations output for the plt section */
{
	size_t	pltsz = countPLT * PLTENTSZ + NOP_SPACE;

	plt_sect = &plttemp;

	plt_sect->is_rawbits->d_buf = mycalloc(pltsz);

	plt_sect->is_rawbits->d_size = pltsz;
	plt_sect->is_rawbits->d_version = libver;
	plt_sect->is_shdr->sh_size = pltsz;

	place_section(plt_sect);

	/*make its relocation section if needed */

	if(size_pltrels){
		pltrel_sect->is_rawbits->d_buf = (char *) mycalloc(size_pltrels);

		pltrel_sect->is_rawbits->d_size = (size_t)size_pltrels;
		pltrel_sect->is_rawbits->d_version = libver;
		pltrel_sect->is_shdr->sh_size = size_pltrels;

		place_section(pltrel_sect);

		plt_sect->is_outsect_ptr->os_outrels = pltrel_sect;
		plt_sect->is_outsect_ptr->os_szoutrel = size_pltrels;
	}
}


/* build a section header strtab section */
void
make_shstrtab()
{
	/* must do place_section first since it will affect count_namelen */
	place_section(shstrtab_sect);

	/* one for the null byte at beginning of section */
	shstrtab_sect->is_rawbits->d_buf = (char *) mymalloc(++count_namelen);

	shstrtab_sect->is_rawbits->d_size = (size_t)count_namelen;
	shstrtab_sect->is_rawbits->d_version = libver;
	shstrtab_sect->is_shdr->sh_size = count_namelen;

	((char *)shstrtab_sect->is_rawbits->d_buf)[0] =
		((char *)shstrtab_sect->is_rawbits->d_buf)[count_namelen-1] = '\0';

}



/* build a symbol table strtab section */
void
make_strtab()
{

	count_strsize += 1;
		/* for the null byte at beginning of section */

	if (!xflag) {
		count_strsize += count_locstr;
			/* add in strings for locals */
		count_strsize += strlen(outfile_name)+1;
		if (count_hidden)
			count_strsize += strlen(FAKE_NAME) + 1; 
			/* add in the special symbol 
			 * _fake_hidden so that the
			 * debugger can find the hidden symbols
			 */
	}

	strtab_sect->is_rawbits->d_buf = (char *)mymalloc(count_strsize);

	((char *)strtab_sect->is_rawbits->d_buf)[0] = 
		((char *)strtab_sect->is_rawbits->d_buf)[count_strsize-1] = '\0';

	if(dmode && Gflag)
		strtab_sect->is_shdr->sh_flags = SHF_ALLOC;


	strtab_sect->is_rawbits->d_size = (size_t)count_strsize;
	strtab_sect->is_rawbits->d_version = libver;
	strtab_sect->is_shdr->sh_size = count_strsize;

	place_section(strtab_sect);
}


/* build a symtab section */
void
make_symtab()
{
	Word	size;
	Word	nsyms;

	Word	ndx;

        if(dmode && Gflag)
                symtab_sect->is_shdr->sh_flags = SHF_ALLOC;


	symtab_sect->is_shdr->sh_entsize = my_elf_fsize(ELF_T_SYM,1,libver);
	place_section(symtab_sect);

	nsyms = count_outglobs + 1;
		/* one for the null first entry */
	if (!xflag) {
		nsyms += count_hidden + count_outlocs + 2;
		/* one for the FILE symbol */
		/* one for .shstrtab entry */
		if(dmode)
			nsyms++; /* one for .hash entry */;
	}


	size = nsyms * sizeof(Sym);

	symtab_sect->is_rawbits->d_buf = (char *) mycalloc(size);

	symtab_sect->is_rawbits->d_size = (size_t)size;
	symtab_sect->is_rawbits->d_version = libver;
	symtab_sect->is_shdr->sh_size = nsyms * symtab_sect->is_shdr->sh_entsize;


	/* for a shared object calculate the number of output hash buckets */
	if (Gflag) {
		for (ndx = 0; ndx < LAST_HASHSIZE; ndx++) {
			if (nsyms < hashsize[ndx+1]){
				dynbkts = hashsize[ndx];
				break;
			}
		}
		if(!dynbkts)
			dynbkts = NBKTS;
	}
}


/* build a dynamic subset symbol table strtab section */
/* only called if building a dynamic executable */
void
make_dynstrtab()
{
	count_dynstrsize++;	/* one for the null byte at beginning of section */

	dynstrtab_sect->is_rawbits->d_buf = (char *) mymalloc(count_dynstrsize);

	((char *)dynstrtab_sect->is_rawbits->d_buf)[0] =
		 ((char *)dynstrtab_sect->is_rawbits->d_buf)[count_dynstrsize-1] = '\0';
	dynstrtab_sect->is_rawbits->d_size = (size_t)count_dynstrsize;
	dynstrtab_sect->is_rawbits->d_version = libver;
	dynstrtab_sect->is_shdr->sh_size = count_dynstrsize;

	place_section(dynstrtab_sect);
}


/* build a dynamic subset symtab section */
/* only called if building a dynamic executable */
void
make_dynsymtab()
{
	Word	size;
	Word	nsyms;

	Word	ndx;

	dynsymtab_sect->is_shdr->sh_entsize = my_elf_fsize(ELF_T_SYM,1,libver);
	place_section(dynsymtab_sect);

	nsyms = count_dynglobs + 1;		/* one for the null first entry */

	size = nsyms * sizeof(Sym);

	dynsymtab_sect->is_rawbits->d_buf = (char *) mycalloc(size);

	dynsymtab_sect->is_rawbits->d_size = (size_t)size;
	dynsymtab_sect->is_rawbits->d_align = WORD_ALIGN;
	dynsymtab_sect->is_rawbits->d_version = libver;
	dynsymtab_sect->is_shdr->sh_size = nsyms * symtab_sect->is_shdr->sh_entsize;

	/* calculate the number of output hash buckets */
	for( ndx = 0; ndx < LAST_HASHSIZE; ndx++){
		if( nsyms < hashsize[ndx+1]){
			dynbkts = hashsize[ndx];
			break;
		}
	}
	if(!dynbkts)
		dynbkts = NBKTS;
}
