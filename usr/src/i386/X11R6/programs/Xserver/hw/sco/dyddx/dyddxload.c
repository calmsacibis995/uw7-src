/*
 *	@(#)dyddxload.c	6.4	2/27/96	16:27:45
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * SCO Modification History
 *
 * 	S000	Thu Oct 15 12:11:08 PDT 1992	 staceyc
 * 	- fix some ancient type problems with pointer subtraction to make gcc
 *	less unhappy
 *	S001	Thu Oct 15 12:11:08 PDT 1992	mikep@sco.com
 *	- Change strdup to Xstrdup, calloc to Xcalloc and free to Xfree.
 *	S002	Fri Nov 06 14:59:51 PST 1992	buckm@sco.com
 *	- Align common data appropriately.
 *	S003	Mon Oct 04 06:53:16 PDT 1993	buckm@sco.com
 *	- globalSymbols is sorted now, so use a binary search for lookups.
 *	S004	Tue Feb 27 16:26:45 PST 1996	hiramc@sco.COM
 *	- RELSZ is 10 on SCO, 12 on UWare, force 10 on UWare
 */

#include	<stdio.h>
#include	<malloc.h>
#include	<string.h>

/*
 * COFF header files
 */
#include	<aouthdr.h>

#if defined(gemini)
typedef char BOOL;              /* Boolean variable  */
#define         TRUE     1      /* logical true  */
#define         FALSE    0      /* logical false  */
#else
#include      <bool.h>
#endif

#include	<filehdr.h>
#include	<scnhdr.h>
#include	<syms.h>
#include	<reloc.h>
#include	<ldfcn.h>

#include	"dyddx.h"

extern char	*ldgetname();
extern char * Xstrdup(char *);

extern symbolDef	globalSymbols[];
extern int		globalSymbolCount;			/* S003 */

/*
 * Internal symbol table structure
 */
struct sym
{
    char	*name;		/* symbol name		*/
    long	value;		/* value		*/
    long	size;		/* value		*/
    short	scnum;		/* section number	*/
    char	sclass;		/* storage class	*/
};

/*
 * Maximum number of sections in a COFF file
 */
#define	MAXSCNS	16		/* arbitrary but should be more than enough */

void *(* dyddxload( int inFD, struct dynldinfo *infop ) )()
{
    LDFILE	*ldptr;
    SCNHDR	scntab[MAXSCNS+2];
    char	*scnaddr[MAXSCNS+2];
    SYMENT	coffsym;
    struct sym	*symtab;
    struct sym	*s;
    int		externals;
    long	bss_size;
    int		bss_scn;
    long	memsize;
    char	*loadaddr;
    char	*m;
    int		i;
    int		j;
    FILE	*f;
    AOUTHDR	aouthdr;
    long	entry = 0;

	unsigned short type;
	extern LDFILE *allocldptr();


    loadaddr	= NULL;
    symtab	= NULL;

    if ((f = fdopen (inFD, "r")) == NULL)
    	return (NULL);

	if (fread((char *)&type, sizeof(type), 1, f) != 1 ||
		(ldptr = allocldptr()) == NULL)
	{
		fclose(f);
		return (NULL);
	}

	TYPE(ldptr) = type;
	OFFSET(ldptr) = 0L;
	IOPTR(ldptr) = f;
	if (!(FSEEK(ldptr, 0L, BEGINNING) == OKFSEEK) ||
	    !(FREAD((char *)&(HEADER(ldptr)), FILHSZ, 1, ldptr) == 1))
	{
		return (NULL);
	}

	FSEEK(ldptr, FILHSZ, BEGINNING);
	FREAD((char *) &aouthdr, sizeof(aouthdr), 1, ldptr);



    if (HEADER(ldptr).f_flags & F_RELFLG) {
	ErrorF("no relocation information\n");
	goto out;
    }

    if (HEADER(ldptr).f_nscns > MAXSCNS) {
	ErrorF("too many sections\n");
	goto out;
    }

    /*
     * read the section table
     * (NB sections are numbered starting at 1)
     */
    for (i = 1; i <= HEADER(ldptr).f_nscns; i++)
	if (ldshread(ldptr, i, &scntab[i]) != SUCCESS) {
	    ErrorF("can't read section header %d\n", i);
	    goto out;
	}

    /*
     * calculate amount of memory needed to load the file
     * (round individual section sizes to a multiple of 4
     * so that we can align the start of the next section
     * on a 4 byte boundary)
     */
    memsize	= sizeof(int (*)());

    for (i = 1; i <= HEADER(ldptr).f_nscns; i++)
	if (scntab[i].s_flags & (STYP_TEXT | STYP_DATA | STYP_BSS))
	    memsize += (scntab[i].s_size + 3) & ~3;

    /*
     * load the symbol table
     */
    symtab = (struct sym *) Xcalloc(HEADER(ldptr).f_nsyms * sizeof(struct sym));

    if (symtab == NULL) {
	ErrorF("can't allocate memory for symbol table\n");
	goto out;
    }

    for (i = 0; i < HEADER(ldptr).f_nsyms; i += (coffsym.n_numaux + 1)) {
        if (ldtbread(ldptr, i, &coffsym) != SUCCESS) {
	    ErrorF("can't read coff symbol index %d\n", i);
	    goto out;
	}
	symtab[i].name	= Xstrdup(ldgetname(ldptr, &coffsym));
	symtab[i].value	= coffsym.n_value;
	symtab[i].sclass= coffsym.n_sclass;
	symtab[i].scnum	= coffsym.n_scnum;
    }

    /*
     * Allocate common storage in BSS.
     * C "common" data (ie uninitialised external arrays) is indicated
     * in the symbol table by a storage class of C_EXT and a section
     * number of N_UNDEF. The value field represents the size of the
     * object. Storage for these objects has to be allocated in the
     * BSS section.
     * NB - the COFF section header for BSS in a relocatable object file
     * does NOT contain the size of BSS.
     */
    bss_size	= 0;
    bss_scn	= HEADER(ldptr).f_nscns + 1;

    for (i = 0, s = symtab; i < HEADER(ldptr).f_nsyms; i++, s++) {
	if (s->sclass == C_EXT && s->scnum == N_UNDEF && s->value != 0) {
	    /* S002
	     * align words on word boundaries;
	     * align anything larger on dword boundaries.
	     */
	    if (s->value > 2)
		bss_size = (bss_size + 3) & ~3;
	    else if (s->value == 2)
		bss_size = (bss_size + 1) & ~1;
	    s->size	= s->value;
	    s->value	= bss_size;	/* value is now offset in BSS	*/
	    s->scnum	= bss_scn;	/* section is now BSS		*/
	    bss_size   += s->size;
	}
    }

    memsize	+= bss_size;

    /*
     * allocate memory
     */
    if ((m = loadaddr = (char *) Xalloc(memsize)) == NULL) {
	ErrorF("can't allocate %d bytes\n", memsize);
	goto out;
    }

    /*
     * initialise entrypoint to NULL
     */
    *(int (**)())m	= NULL;
    m			+= sizeof(int (*)());

    /*
     * assign base address to each section
     */
    for (i = 1; i <= HEADER(ldptr).f_nscns; i++)
	if (scntab[i].s_flags & (STYP_TEXT | STYP_DATA | STYP_BSS)) {
	    scnaddr[i]	= m;
	    m		+= (scntab[i].s_size + 3) & ~3;
	}

    /*
     * set up additional entry for C common data allocated in BSS
     */
    scntab[bss_scn].s_vaddr	= 0;
    scnaddr[bss_scn]		= m;
    memset(scnaddr[bss_scn], 0, bss_size);

    /*
     * load and relocate TEXT and DATA
     */
    externals	= 0;

    for (i = 1; i <= HEADER(ldptr).f_nscns; i++) {
	long		scn_size;	/* section size			*/
	char		*scn_addr;	/* section address in memory	*/
	long		scn_vaddr;	/* section virtual address	*/
	int		n;		/* number of relocation entries	*/
	RELOC		rel;		/* COFF relocation entry	*/
	unsigned long	*fixup;		/* location to be fixed up	*/
	unsigned char	*target;	/* target address of fixup	*/

	if ((scn_size = scntab[i].s_size) == 0)
	    continue;

	scn_addr	= scnaddr[i];
	scn_vaddr	= scntab[i].s_vaddr;

	/*
	 * clear BSS
	 */
	if (scntab[i].s_flags & STYP_BSS) {
	    memset(scn_addr, 0, scn_size);
	    continue;
	}

	/*
	 * ignore everything except TEXT and DATA
	 */
	if ((scntab[i].s_flags & (STYP_TEXT | STYP_DATA)) == 0)
	    continue;

	/*
	 * seek to data for section and read it in to memory
	 */
	if (ldsseek(ldptr, i) != SUCCESS) {
	    ErrorF("can't seek to data for section %d\n", i);
	    goto out;
	}

	if (FREAD(scn_addr, 1, scn_size, ldptr) != scn_size) {
	    ErrorF("can't read data for section %d\n", i);
	    goto out;
	}

	/*
	 * check for relocation information (there may not be any)
	 */
	if ((n = scntab[i].s_nreloc) == 0)
	    continue;

	/*
	 * seek to start of relocation information for this section
	 */
	if (ldrseek(ldptr, i) != SUCCESS) {
	    ErrorF("can't seek to relocation info for section %d\n", i);
	    goto out;
	}

	while (--n >= 0)
	{
#if defined(gemini)
#define	RELSZ	10
#endif	/*	For reading COFF XDriver.o files from OSR 5.0	*/
	    if (FREAD(&rel, RELSZ, 1, ldptr) != 1) {
		ErrorF("can't read relocation entry\n");
		goto out;
	    }

	    /*
	     * get pointer to symbol table entry
	     */
	    s = &symtab[rel.r_symndx];

	    /*
	     * calculate address in memory to be fixed up and
	     * the target address that it references
	     */
	    fixup	= (unsigned long *)(scn_addr + (rel.r_vaddr-scn_vaddr));

	    /*
	     * check for unresolved externals
	     */
	    if (s->sclass == C_EXT && s->scnum == N_UNDEF)
	    {
		target = NULL;					/* vvv S003 */

		if (s->name) {
		    int lo = 0, hi = globalSymbolCount - 1, mid, cmp;

		    while (lo <= hi) {
			mid = (lo + hi) / 2;
			cmp = strcmp(s->name, globalSymbols[mid].name);
			if (cmp == 0) {
			    target = (unsigned char *)globalSymbols[mid].ptr;
			    break;
			}
			if (cmp < 0)
			    hi = mid - 1;
			else
			    lo = mid + 1;
		    }
		}						/* ^^^ S003 */

		if (target == NULL) {
		    ErrorF("unresolved external '%s'\n", s->name);
		    ++externals;
		    continue;
		}
	        /*
	         * perform the relocation / fixup
	         */
	        switch (rel.r_type)
	        {
		    unsigned long old;
		    default:
		        ErrorF("unknown relocation type %03o\n", rel.r_type);
		        goto out;

		    case R_PCRLONG:	/* 32 bit PC-relative		*/
		    	old = *fixup + rel.r_vaddr + 4;
		        *fixup	= (unsigned long)((unsigned long)(target + old)
			    - (unsigned long)((char *)fixup + 4));
		        break;

		    case R_DIR32:	/* 32 bit direct reference	*/
		        *fixup	= (unsigned long)target + *fixup;
		        break;
	        }
	    }
	    else if (s->scnum == bss_scn)
	    {
		target = scnaddr[bss_scn] + s->value + *fixup - s->size;
	        switch (rel.r_type)
	        {
		    default:
		        ErrorF("unknown relocation type %03o\n", rel.r_type);
		        goto out;

		    case R_PCRLONG:	/* 32 bit PC-relative		*/
		        *fixup	= (unsigned long)((unsigned long)target -
			    (unsigned long)((char *)fixup + 4));
		        break;

		    case R_DIR32:	/* 32 bit direct reference	*/
		        *fixup	= (unsigned long)target;
		        break;
	        }
	    }
	    else
	    {
		    if (s->scnum < 1 || (s->scnum > HEADER(ldptr).f_nscns)) {
			ErrorF("bad section # in relocation entry\n");
			goto out;
	    	    }

		target	= scnaddr[s->scnum] +
			  (*fixup - scntab[s->scnum].s_vaddr);

	    /*
	     * perform the relocation / fixup
	     */
	    switch (rel.r_type)
	    {
		default:
		    ErrorF("unknown relocation type %03o\n", rel.r_type);
		    goto out;

		case R_PCRLONG:	/* 32 bit PC-relative		*/
		    if (i!=s->scnum)
			ErrorF("Scary relocation!\n");
		    break;

		case R_DIR32:	/* 32 bit direct reference	*/
		    *fixup	= (unsigned long)target;
		    break;
	    }
	    }

    	}
    }

    if (externals != 0)
	goto out;

    /*
     * look for the entrypoint
     */

    for (i = 0, s = symtab; i < HEADER(ldptr).f_nsyms; i++, s++)
    {
	if (s->value == aouthdr.entry)
	{
	    if (s->value == C_EXT && s->scnum == N_UNDEF) {
		ErrorF("entrypoint '%s' is an unresolved external\n", s->name);
		goto out;
	    }

	    if (s->scnum < 1 || s->scnum > bss_scn) {
		ErrorF("bad section # for entry point\n");
		goto out;
	    }
	    
	    entry = (long) scnaddr[s->scnum] + s->value - scntab[s->scnum].s_vaddr;
	    break;
	}
    }


    if (i >= HEADER(ldptr).f_nsyms)
	ErrorF("can't find entry point\n");

out:
    infop->base = (void *) loadaddr;

/*
    if (loadaddr != NULL && *(char **)loadaddr == NULL) {
	Xfree(loadaddr);
	loadaddr = NULL;
    }
*/

    if (symtab != NULL) {
	for (i = 0, s = symtab; i < HEADER(ldptr).f_nsyms; i++, s++)
	    if (s->name != NULL)
		Xfree(s->name);
	Xfree(symtab);
    }

    ldclose(ldptr);

    return (entry);
}

void
dyddxunload( struct dynldinfo *infop )
{
/*	Xfree (infop->base);*/
	return ;
}
