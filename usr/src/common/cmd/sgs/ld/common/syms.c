#ident	"@(#)ld:common/syms.c	1.94"
/*
** Module syms
** Symbol table management routines
*/

/****************************************
** Imports
****************************************/

#ifndef	DEBUG
#	define	NDEBUG	/* for assert.h */
#endif

#include	<stdio.h>
#include	<string.h>
#include	<assert.h>
#include	<pfmt.h>
#include	<unistd.h>
#include	"sgs.h"
#include	"globals.h"
#include	"macros.h"

/****************************************
** Local Macros
****************************************/

#define	BUMP_GLOB(symname)	{count_outglobs++; count_strsize += strlen(symname)+1;}

#define	BUMP_DYN(symname)	{count_dynglobs++; count_dynstrsize += strlen(symname)+1;}

/****************************************
** Local Variables
****************************************/

static Ldsym		*oldsym;		/* symbol in internal symbol table */
static Sym		*newsym;		/* new input symbol in file format */
static char		*symname;		/* extracted name of global symbol */
static char		*tsymname;		/* extracted name of global symbol */
static CONST char	*oldflname;		/* filename saved from possibly overwritten oldsym */
static unsigned char	oldbind,		/* binding saved from possibly overwritten oldsym */
			oldtype;		/* type saved from possibly overwritten oldsym */
static Word		oldsize;		/* size saved from possibly overwritten oldsym */

static CONST char command_line[] = COMMAND_LINE ;

/****************************************
** Local Function Definitions
****************************************/

LPROTO(void overrideold, ());
LPROTO(void print_undefs,(Errorlevel));
LPROTO(void sizecheck, (Shdr *,Word, Word));
LPROTO(void sizetypechk, (int /*Boolean*/));
LPROTO(void sym_resolv, ());
LPROTO(Word update_locs,(Word));
LPROTO(Word update_hidden,(Word));
LPROTO(void mark_list_as_export, ());
LPROTO(void mark_list_as_hide, (char **, int));

/****************************************
** Local Function Declarations
****************************************/

/* overrideold()
 ** replace information in internal symbol table (oldsym)
 ** with information from newsym
 */
static void
overrideold()
{

	oldsym->ls_syment = newsym;
	oldsym->ls_flptr = cur_infile_ptr;
	if(oldbind == STB_GLOBAL){
		/* save the binding information for the definition symbol -
			used in weak symbol solution */
		oldsym->ls_def_binding = ELF_ST_INFO( 
						ELF_ST_BIND(newsym->st_info),
					      	ELF_ST_TYPE(newsym->st_info)); 

		oldsym->ls_syment->st_info = ELF_ST_INFO(STB_GLOBAL,ELF_ST_TYPE(newsym->st_info));

	}
	if((newsym->st_shndx != SHN_COMMON) && (newsym->st_shndx != SHN_ABS))
		if((oldsym->ls_scnptr = cur_infile_ptr->fl_insect_ndx[newsym->st_shndx]) == NULL)
			lderror(MSG_FATAL,gettxt(":1120","no defining section for symbol: %s from file: %s"),
				symname, cur_file_name);
}


/* print_undefs(Errorlevel level)
** walk through symbol table and print list of undefined symbols;
** then exit with 'level' error
*/
static void
print_undefs(level)
	Errorlevel	level;
{
	register Ldsym		*sym;
	register Word		h;
	register Listnode	*stcp;
	int 	 		foundone = 0;
		
	/* print header */
	pfmt(stderr,MM_NOSTD,":1196:Undefined\t\t\tfirst referenced\n");
	pfmt(stderr,MM_NOSTD,":1197: symbol  \t\t\t    in file\n");

	/* step through symbol table */
	for(h = 0; h<NBKTS; h++){
	    for(LIST_TRAVERSE(&symbucket[h],stcp,sym)){
		if ((sym->ls_syment->st_shndx == SHN_UNDEF) &&
			(ELF_ST_BIND(sym->ls_syment->st_info) != STB_WEAK) &&
			(sym->ls_deftag != REF_DEFN)){
			/* ignore zeroed out symbol table entries */
			if(sym->ls_syment->st_name != 0 || sym->ls_syment->st_value != 0 ||
			    sym->ls_syment->st_size != 0 || sym->ls_syment->st_info != 0 ||
			    sym->ls_syment->st_other != 0){
				foundone++;
				pfmt(stderr,MM_NOSTD,":1198:%-35s %s\n",sym->ls_name,
					(sym->ls_flptr != NULL) ?
					sym->ls_flptr->fl_name : command_line);
			}
		}
	    }
	}
	if(foundone)
		if (level == MSG_FATAL)
			lderror(MSG_FATAL,gettxt(":1121","Symbol referencing errors. No output written to %s"),outfile_name);
		else
			lderror(level,gettxt(":1122","Symbol referencing errors"));
}


/* sizecheck(Shdr *realshdr,Word realsize, Word tentsize)
 ** check relative sizes of real and tentative definitions for symbol
 */
static void
sizecheck(realshdr,realsize,tentsize)
	Shdr	*realshdr;	/* section header of real defn */
	Word	realsize,	/* size of real definition */
		tentsize;	/* size of tentative definition */
{
	if (realsize == tentsize) {
		if ( oldtype != ELF_ST_TYPE(newsym->st_info))
			lderror(MSG_WARNING,
				gettxt(":1123","incompatible types for symbol: `%s` also in file %s"), 
				symname, oldflname);
	} else if (realsize > tentsize) {
		if (!tflag)
			lderror(MSG_WARNING,
				gettxt(":1124","incompatible sizes for symbol: `%s` also in file %s"), 
				symname, oldflname);
	} else if(realshdr->sh_type == SHT_NOBITS){
			oldsym->ls_syment->st_size = tentsize;
			lderror(MSG_WARNING,
				gettxt(":1125","size of symbol `%s` is overridden with the larger size of tentative definition"),
				symname);
		} else
			lderror(MSG_FATAL,
				gettxt(":1126","attempt to override defined size of symbol `%s` with size of tentative definition"), 
				symname);
}


/* sizetypechk(Boolean ups)
 ** check sizes and types of old and new symbols;
 ** if ups is TRUE, upgrade size kept
 */
static void
sizetypechk(ups)
	Boolean	ups;	/* if set update size of symtab symbol */
{
	
	if (oldsize != newsym->st_size ) {
		if( !tflag)
			lderror(MSG_WARNING,
				gettxt(":1127","symbol: `%s` has different size in file %s"),
				symname, oldflname);
		if (ups)
			if (oldsize < newsym->st_size)
				oldsym->ls_syment->st_size = newsym->st_size;
	}
	if (oldtype != ELF_ST_TYPE(newsym->st_info))
		lderror(MSG_WARNING,
			gettxt(":1123","incompatible types for symbol: `%s` also in file %s"), 
			symname, oldflname);
}


/* sym_resolv()
 ** A symbol input from a current file symbol table (newsym)
 ** matches an symbol already in the internal ld symbol table (oldsym)
 ** This function resolves the two (see ld requirements, Section 3.4)
 */

static void
sym_resolv()
{
	/* save information from symbol already in internal symbol table */

	oldflname = oldsym->ls_flptr->fl_name;
	oldbind = ELF_ST_BIND(oldsym->ls_syment->st_info),
	oldtype = ELF_ST_TYPE(oldsym->ls_syment->st_info);
	oldsize = oldsym->ls_syment->st_size;
	
	DPRINTF(DBG_SYMS, (MSG_DEBUG, "newsym st_shndx = %d; oldsym st_shndx = %d\n",
                        newsym->st_shndx,oldsym->ls_syment->st_shndx));

	/* check for tentative defns and function confusion */
	if((cur_file_ehdr->e_type == ET_DYN)
		&& (oldsym->ls_syment->st_shndx == SHN_COMMON)
		&& (ELF_ST_TYPE(newsym->st_info) == STT_FUNC))
		return;
	else if ((oldsym->ls_flptr->fl_e_type == ET_DYN)	/* curfile is ET_REL */
		&& (newsym->st_shndx == SHN_COMMON)
		&& (oldtype == STT_FUNC)) {
			oldbind = ELF_ST_BIND(newsym->st_info);
			if(oldsym->ls_deftag < REF_DEFN){
				BUMP_GLOB(symname)
			}
			oldsym->ls_deftag = REF_RELOBJ;
			overrideold();
			return;
	}

	/* check relative bindings */
	if (oldbind == STB_WEAK && ELF_ST_BIND(newsym->st_info) == STB_GLOBAL)
		oldsym->ls_syment->st_info = ELF_ST_INFO(STB_GLOBAL,oldtype);

	if (newsym->st_shndx != SHN_UNDEF) {
		if (oldsym->ls_syment->st_shndx != SHN_UNDEF) { 

			/* both defined */

			DPRINTF(DBG_SYMS, (MSG_DEBUG, "both defined\n"));

			if (ELF_ST_BIND(newsym->st_info) == STB_WEAK){
				if(oldsym->ls_flptr->fl_e_type == cur_file_ehdr->e_type)
					return;		/* ignore subsequent weak global defn*/
				else if (cur_file_ehdr->e_type == ET_DYN)

					/* new from .so, old from .o */
					if(oldsym->ls_deftag != REF_DYN){
						if(dmode && !Gflag){
							BUMP_DYN(symname)
							oldsym->ls_deftag = REF_DYN;
						}
					}
					/* new from .o, old from .so */
				else if(oldsym->ls_deftag < REF_DEFN) {
						BUMP_GLOB(symname)
						if(dmode && !Gflag)
							BUMP_DYN(symname)
						oldsym->ls_deftag = REF_DEFN;
					}
			}

			if (newsym->st_shndx == SHN_COMMON) {
				if (oldsym->ls_syment->st_shndx == SHN_COMMON) { 

					/* both are tentative defns */

					DPRINTF(DBG_SYMS, (MSG_DEBUG, "both are tentative\n"));

					if ((oldsym->ls_deftag < REF_RELOBJ) &&
						(cur_file_ehdr->e_type == ET_REL)){
						lderror(MSG_WARNING,
							gettxt(":1128","tentatively defined symbol `%s` appears in a shared object"),
							symname);
   						if (oldsym->ls_deftag < REF_DEFN){
                                                        BUMP_GLOB(symname)
                                                        if(dmode && !Gflag)
                                                                BUMP_DYN(symname)
                                                }
 						oldsym->ls_deftag = (dmode && !Gflag) ? REF_DYN : REF_RELOBJ;
                                                oldsym->ls_flptr = cur_infile_ptr;
                                        }
					if (oldsym->ls_syment->st_value != newsym->st_value ) {
						if (!tflag)
							lderror(MSG_WARNING,
								gettxt(":1129","tentatively defined symbol `%s` has different alignment in file %s"),
								symname, oldflname);
						if (oldsym->ls_syment->st_value < newsym->st_value)
							oldsym->ls_syment->st_value = newsym->st_value;
					}
					sizetypechk(TRUE);
					if (oldtype != STT_OBJECT || ELF_ST_TYPE(newsym->st_info) != STT_OBJECT){
						lderror(MSG_WARNING,
							gettxt(":1130","tentatively defined symbol `%s` appears with type other than STT_OBJECT from input file or %s"), 
							symname, oldflname);
						oldsym->ls_syment->st_info = ELF_ST_INFO(oldbind,STT_OBJECT);
					}
				} else { 
					/* newsym is tent, oldsym is real */

					DPRINTF(DBG_SYMS, (MSG_DEBUG, "new symbol tentative, old is real\n"));

					if (oldsym->ls_deftag == REF_RELOBJ){
						if (cur_file_ehdr->e_type == ET_DYN){
							lderror(MSG_WARNING,
								gettxt(":1131","tentatively defines symbol `%s` appears in shared object"),
                                                        symname);
							if( dmode && !Gflag){
								BUMP_DYN(symname)
								oldsym->ls_deftag = REF_DYN;
							}
						}
						sizetypechk(FALSE);
					} else { /* definition came from a .so */
						if (cur_file_ehdr->e_type == ET_REL)
							if (oldsym->ls_deftag < REF_DEFN){
								BUMP_GLOB(symname)
								if(dmode && !Gflag)
									BUMP_DYN(symname)
								oldsym->ls_deftag = REF_DEFN;
							}
						else if (oldsym->ls_deftag == REF_UNK)
							oldsym->ls_deftag = REF_SEEN;
						sizecheck(oldsym->ls_scnptr->is_shdr,oldsize,newsym->st_size);
					}
				}
			} else if (oldsym->ls_syment->st_shndx == SHN_COMMON) { 

				/* new is real, old is tent */

				DPRINTF(DBG_SYMS, (MSG_DEBUG, "new is real, old is tent\n"));
				overrideold();
				if (cur_file_ehdr->e_type == ET_REL) {
					if (oldsym->ls_deftag < REF_DEFN){
						lderror(MSG_WARNING,
							gettxt(":1132","tentatively defines symbol `%s` appears in shared object %s"),
                                                        symname, oldflname);
						BUMP_GLOB(symname)
						if(dmode && !Gflag)
							BUMP_DYN(symname)
						oldsym->ls_deftag = (dmode && !Gflag) ? REF_DYN : REF_RELOBJ;
					}
					sizetypechk(FALSE);
				} else {
					if(oldsym->ls_deftag == REF_DYN)
						oldsym->ls_deftag = REF_DEFN;
					else if(oldsym->ls_deftag == REF_RELOBJ){
						if(dmode && !Gflag)
							BUMP_DYN(symname)
						oldsym->ls_deftag = REF_DEFN;
					} else if(oldsym->ls_deftag == REF_UNK){
						lderror(MSG_WARNING,
							gettxt(":1132","tentatively defines symbol `%s` appears in shared object %s"),
                                                        symname, oldflname);
						oldsym->ls_deftag = REF_SEEN;
					}
					sizecheck(my_elf_getshdr(my_elf_getscn(cur_file_ptr,newsym->st_shndx)),newsym->st_size,oldsize);
				}
			} else {	 /* both are real */

				DPRINTF(DBG_SYMS, (MSG_DEBUG, "both are real\n"));

				if (dmode) { /* reqt's 52 and 53 */
					if (oldsym->ls_deftag < REF_RELOBJ) {
						if (cur_file_ehdr->e_type != ET_REL){
						
							if( (ELF_ST_TYPE(newsym->st_info) == STT_FUNC)
								&& (oldtype == STT_FUNC))
									return;
							else if( (ELF_ST_TYPE(newsym->st_info) == STT_OBJECT)
								&& (oldtype == STT_OBJECT))
									if(newsym->st_size != oldsym->ls_syment->st_size)
										lderror(MSG_FATAL,
											gettxt(":1133","symbol `%s` multiply defined in two shared objects %s: size = %d; %s: size = %d"),
											symname, oldflname, oldsym->ls_syment->st_size, cur_file_name, newsym->st_size);
									else
										return;
						} else { /* if new one is from a .o and is not weak -- keep it */
							if (ELF_ST_BIND(newsym->st_info) == STB_WEAK)
								return;
							overrideold();
							if (oldsym->ls_deftag < REF_DEFN){
								BUMP_GLOB(symname)
								if(!Gflag)
									BUMP_DYN(symname)
							}
							oldsym->ls_deftag = (!Gflag) ? REF_DYN : REF_RELOBJ;
						} 
					} else {
						if (cur_file_ehdr->e_type != ET_REL ){
							if( !Gflag && oldsym->ls_deftag == REF_RELOBJ){
								BUMP_DYN(symname)
								oldsym->ls_deftag = REF_DYN;
							}
							return;
						} else{
							/* put in a support for MetaWare use of absolute common symbols */
							/* for version control. */
							/* ignore multiply-defined absolute globals */
							if (oldbind == STB_GLOBAL &&
								ELF_ST_BIND(newsym->st_info) == STB_GLOBAL &&
								oldsym->ls_syment->st_shndx == SHN_ABS &&
								newsym->st_shndx == SHN_ABS &&
								oldsym->ls_syment->st_value == newsym->st_value)
								return;

							lderror(MSG_FATAL,
								gettxt(":1134","symbol `%s` multiply-defined, also in file %s"),
									symname, oldflname);
						}
					}
				} else{
					/* put in a support for MetaWare use of absolute common symbols */
					/* for version control. */
					/* ignore multiply-defined absolute globals */
					if (oldbind == STB_GLOBAL &&
						ELF_ST_BIND(newsym->st_info) == STB_GLOBAL &&
						oldsym->ls_syment->st_shndx == SHN_ABS &&
						newsym->st_shndx == SHN_ABS &&
						oldsym->ls_syment->st_value == newsym->st_value)
						/* ignore multiply-defined absolute globals */
						return;

					lderror(MSG_FATAL,
						gettxt(":1134","symbol `%s` multiply-defined, also in file %s"),
						 symname, oldflname);
				}
			}
		} else {

			/* newsym defined, oldsym undefined */

			DPRINTF(DBG_SYMS, (MSG_DEBUG, "new defined, old undefined\n"));

			overrideold();
			if(oldsym->ls_deftag == REF_DYN &&
				cur_file_ehdr->e_type == ET_DYN)
				oldsym->ls_deftag = REF_DEFN;
			else if(oldsym->ls_deftag < REF_RELOBJ)
				if (cur_file_ehdr->e_type == ET_REL){
					if (oldsym->ls_deftag < REF_DEFN){
						BUMP_GLOB(symname)
						if(dmode && !Gflag)
							BUMP_DYN(symname)
					}
					oldsym->ls_deftag = (dmode && !Gflag) ? REF_DYN : REF_RELOBJ;
				} else 
					oldsym->ls_deftag = REF_SEEN;
			else if (cur_file_ehdr->e_type != ET_REL) {
				if(dmode && !Gflag)
					BUMP_DYN(symname)
				oldsym->ls_deftag = REF_DEFN;
			}
		}
	} else {
		 /* newsym undefined */

		if (oldsym->ls_syment->st_shndx != SHN_UNDEF) {

			/* newsym undefined, oldsym defined */

			if (oldsym->ls_deftag < REF_DEFN)
				if(cur_file_ehdr->e_type == ET_REL){
					BUMP_GLOB(symname)
					if(dmode && !Gflag)
						BUMP_DYN(symname)
					oldsym->ls_deftag = REF_DEFN;
				} else
					oldsym->ls_deftag = REF_SEEN;
			else if (oldsym->ls_deftag == REF_RELOBJ)
 				if(cur_file_ehdr->e_type == ET_DYN && !Gflag){
 					BUMP_DYN(symname)
 					oldsym->ls_deftag = REF_DYN;
 				}
		} else  {

			/* both undefined */

			DPRINTF(DBG_SYMS, (MSG_DEBUG, "both are undefined\n"));

			if(cur_file_ehdr->e_type == ET_REL &&
				oldsym->ls_deftag < REF_RELOBJ) {
					BUMP_GLOB(symname)
					oldsym->ls_deftag = REF_RELOBJ;
					oldsym->ls_flptr = cur_infile_ptr;
					if (dmode && !Gflag){
                                                BUMP_DYN(symname)
                                                oldsym->ls_deftag = REF_DYN;
                                        }
			}
			if (cur_file_ehdr->e_type == ET_DYN &&
				oldsym->ls_deftag == REF_RELOBJ){
					if (dmode && !Gflag){
						BUMP_DYN(symname)
						oldsym->ls_deftag = REF_DYN;
					}
			}
		}
	}
}

/* If the symbol must be hidden, then give it a output symbol table index
** that will be greater than the last local symbol and less than the first
** global symbol.
*/

static Word
update_hidden(ndx)
register Word		ndx;
{

	register Word	h;
	register Listnode *stcp;
	Ldsym*		sym;

	for(h = 0; h< NBKTS; h++){
		for(LIST_TRAVERSE(&symbucket[h],stcp,sym)) {
			if(sym->ls_Bhide == TRUE) {
				sym->ls_outndx = ndx++;
				DPRINTF(DBG_SYMS, (MSG_DEBUG, "updating hidden symbol %s with output index %d\n",sym->ls_name, sym->ls_outndx));
			}
		}
	}
	return ndx;
}

/*
** static Word
** update_locs();
** for each local, assign new virtual address or displacement (value)
*/

static Word
update_locs(ndx)
	register Word		ndx;
{
	register Ldsym		*locsym;
	register Listnode	*flp;
	register Infile		*fdp;
	register Ldsym		*els;

	for(LIST_TRAVERSE(&infile_list,flp,fdp)) {
		if (fdp->fl_e_type == ET_DYN)
			continue;
		els = &(fdp->fl_locals[1]) + fdp->fl_countlocs;
		for(locsym = &(fdp->fl_locals[1]); locsym < els; ++locsym){
			if (locsym->ls_syment->st_shndx == SHN_ABS) {
				locsym->ls_outndx = ndx++;
				continue;
			}
			if(ELF_ST_TYPE(locsym->ls_syment->st_info) == STT_SECTION ){
				if( locsym->ls_scnptr != NULL && locsym->ls_scnptr != (Insect*)(-1)){
					if(locsym->ls_scnptr->is_shdr->sh_flags & SHF_ALLOC)
						locsym->ls_syment->st_value = locsym->ls_scnptr->is_newVAddr;
					else
						locsym->ls_syment->st_value = locsym->ls_scnptr->is_displ;
				 }
			} else {
				locsym->ls_outndx = ndx++;
				if (locsym->ls_scnptr) {
					if (rflag ||
					(!(locsym->ls_scnptr->is_shdr->sh_flags & 
					SHF_ALLOC)))
						locsym->ls_syment->st_value +=
						locsym->ls_scnptr->
						is_displ;
					else
						locsym->ls_syment->st_value +=
						locsym->ls_scnptr->
						is_newVAddr;
					locsym->ls_syment->st_shndx =
						my_elf_ndxscn(locsym->
						ls_scnptr->is_outsect_ptr->
						os_scn);
				}
			}
		}
	}
	return ndx;
}


/***************************************
** Global Function Definitions
****************************************/

/*
** Go through list of symbols that should be handled like -Bsymbolic
** and mark their symbol table entries
*/

void
mark_bsymbolic()
{
	Ldsym	*sym;
	char	**ptr;
	for(ptr = bsym_list; ptr && *ptr; ptr++)
	{
		if ((sym = sym_find(*ptr, NOHASH)) != NULL)
			sym->ls_Bsymbolic = TRUE;
	}
}

/*
** Go through list of symbols that should be hidden in a SHARED OBJECT
** or relocatable
** and mark their symbol table entries as ls_Bhide = TRUE and 
** ls_Bsymbolic = TRUE (to resolve at link time).
** Increment count_hidden and decrement count_outglobs.
*/
	
void
mark_hidden_symbols(list, quiet)
char **list;
Boolean quiet;
{
	Ldsym	*sym;
	char	**ptr;
	int	fatal=0;
	int	save_count_hidden = count_hidden;

	for(ptr = list; ptr && *ptr; ptr++)
	{
		if ((sym = sym_find(*ptr, NOHASH)) != NULL){
			if ( sym->ls_deftag == REF_RELOBJ &&
			     sym->ls_syment->st_shndx != SHN_UNDEF &&
			   (ELF_ST_BIND(sym->ls_syment->st_info) == STB_GLOBAL
			     ||
			   ELF_ST_BIND(sym->ls_syment->st_info) == STB_WEAK))
			{
				if ( sym->ls_Bhide == FALSE ){
					int	nlen;
					DPRINTF(DBG_SYMS, (MSG_DEBUG, "In mark_hidden_symbols(): hiding symbol %s",sym->ls_name));
					sym->ls_Bsymbolic = TRUE;
					sym->ls_Bhide = TRUE;
					count_hidden++;
					count_outglobs--;
					nlen = strlen(sym->ls_name) + 1;
					count_strsize -= nlen;
					count_locstr += nlen;
					if (sym->ls_syment->st_shndx ==
						SHN_COMMON)
						local_common = TRUE;
				}
			} else if (!quiet) {
				if (!fatal) /* print header */ {
					pfmt(stderr,MM_NOSTD,":1519:Cannot hide symbol\n");
					fatal=1;
				}
				pfmt(stderr,MM_NOSTD,":1520:%s\n", *ptr);
			       }
					
		} else if (!quiet) {
			if (!fatal) /* print header */ {
				pfmt(stderr,MM_NOSTD,":1519:Cannot hide symbol\n");
				fatal=1;
			}
			pfmt(stderr,MM_NOSTD,":1520:%s\n", *ptr);
		       }
	}

	if (fatal)
		lderror(MSG_FATAL, gettxt(":1521","hidden symbols require a global or weak definition"));
 

	if (count_hidden && !save_count_hidden)
		/* first time */
		count_hidden++;  /* Add one for the special _fake_hidden symbol
					table entry for the debugger */
}

/*
** Mark the symbols in the list as exported. All other defined globals and
** weaks will be hidden.  
*/

static void
mark_list_as_export()
{
	Ldsym   *sym;
        char    **ptr;
	int	fatal=0;

        for(ptr = bexport_list; ptr && *ptr; ptr++)
        {
                if ((sym = sym_find(*ptr, NOHASH)) != NULL) {
			sym->ls_Bexport = TRUE;
			DPRINTF(DBG_SYMS, (MSG_DEBUG, "exporting symbol %s",sym->ls_name));
		} else {
			if (!fatal) /* print header */ {
				pfmt(stderr,MM_NOSTD,":1522:Cannot export symbol\n");
				fatal=1;
			}
			pfmt(stderr,MM_NOSTD,":1520:%s\n", *ptr);
			}
	}
	if (fatal)
		lderror(MSG_FATAL,
			gettxt(":1523","exported symbols must be globals or weaks"));
		
}

/*
** If -Bhide, mark all the defined global and weak symbols as hidden in a 
** SHARED OBJECT or relocatable. If -Bexport with symbols and -Bhide, 
** then mark those symbols 
** in the export list as exported so that they are excluded from the hidden 
** list. 
*/

void
mark_def_globals_as_hidden()
{
	register Word	h;
	Listnode*	np;
	Ldsym*		sym;

	if (bexport_list && Bflag_export == LIST)
		mark_list_as_export();

	for (h = 0; h < NBKTS; h++) {
		for (LIST_TRAVERSE(&symbucket[h], np, sym)) {
			assert(sym->ls_syment != NULL && sym->ls_name != NULL);
			if (sym->ls_deftag == REF_RELOBJ && 
			    sym->ls_syment->st_shndx != SHN_UNDEF &&
			    (ELF_ST_BIND(sym->ls_syment->st_info) == STB_GLOBAL
			      ||
			     ELF_ST_BIND(sym->ls_syment->st_info) == STB_WEAK))
			{
				if ( sym->ls_Bhide == FALSE && sym->ls_Bexport == FALSE)
				{
					int	nlen;
					DPRINTF(DBG_SYMS, (MSG_DEBUG, "In mark_def_globals_as_hidden(): hiding symbol %s",sym->ls_name));

					sym->ls_Bsymbolic = TRUE;
					sym->ls_Bhide = TRUE;
					count_hidden++;
					count_outglobs--;
					nlen = strlen(sym->ls_name) + 1;
					count_strsize -= nlen;
					count_locstr += nlen;
					if (sym->ls_syment->st_shndx ==
						SHN_COMMON)
						local_common = TRUE;
				}
			}
		}
	}
	if (count_hidden)
		count_hidden++;  /* Add one for the special _fake_hidden symbol
					table entry for the debugger */
}

/*
** If -znodefs is on then add any undefined REF_RELOBJ symbols to
** the dynamic symbol table.  Also add any weak references that
** have remained undefined.
*/

void
add_undefs_to_dynsymtab()
{
	register Word	h;
	Listnode*	np;
	Ldsym*		sym;

	for (h = 0; h < NBKTS; h++) {
		for (LIST_TRAVERSE(&symbucket[h], np, sym)) {
			assert(sym->ls_syment != NULL &&
				sym->ls_name != NULL);
			if ((sym->ls_syment->st_shndx == SHN_UNDEF &&
				sym->ls_deftag == REF_RELOBJ)
				&& (znflag ||
				(ELF_ST_BIND(sym->ls_syment->st_info)
						== STB_WEAK)))
					BUMP_DYN(sym->ls_name);
		
		}
	}
}

/*
** If -Bexport with symbols, mark symbols as exported if the symbol is global, 
** or weak, has not already been seen in the list, and is not an undefined 
** symbol with the znflag set (these will be added by add_undefs_to_dynsymtab().
** A symbol that is not in the global symbol table will be a fatal error.
*/

void
add_export_list_to_dynsymtab()
{

	Ldsym	*sym;
	char	**ptr;
	int	fatal=0;

	for(ptr = bexport_list; ptr && *ptr; ptr++)
	{
		if ((sym = sym_find(*ptr, NOHASH)) != NULL) {
			if (sym->ls_deftag == REF_RELOBJ 
			  && 
			    (ELF_ST_BIND(sym->ls_syment->st_info) == STB_GLOBAL 
			    || ELF_ST_BIND(sym->ls_syment->st_info) == STB_WEAK)
			  &&
			    !(znflag && sym->ls_syment->st_shndx == SHN_UNDEF)
			  &&
			    sym->ls_Bexport == FALSE )
			{
				DPRINTF(DBG_SYMS, (MSG_DEBUG, "exporting symbol %s",sym->ls_name));

				BUMP_DYN(sym->ls_name);
				sym->ls_Bexport = TRUE;
			}
		} else {
			if (!fatal) /* print header */ {
				pfmt(stderr,MM_NOSTD,":1522:Cannot export symbol\n");
				fatal=1;
			}
			pfmt(stderr,MM_NOSTD,":1520:%s\n", *ptr);
			}
	}
	if (fatal)
		lderror(MSG_FATAL, 
		gettxt(":1523","exported symbols must be globals or weaks"));

}

/*
** Mark the symbols in the list as hidden.  All other global and weak
** symbols will be exported.
*/

static void
mark_list_as_hide(list, quiet)
char	**list;
Boolean	quiet;
{
	Ldsym   *sym;
        char    **ptr;
	int	fatal=0;

        for(ptr = list; ptr && *ptr; ptr++)
        {
                if ((sym = sym_find(*ptr, NOHASH)) != NULL &&
		     sym->ls_deftag == REF_RELOBJ &&
		     sym->ls_syment->st_shndx != SHN_UNDEF &&
		     (ELF_ST_BIND(sym->ls_syment->st_info) == STB_GLOBAL ||
		      ELF_ST_BIND(sym->ls_syment->st_info) == STB_WEAK)){
			sym->ls_Bhide = TRUE;
			DPRINTF(DBG_SYMS, (MSG_DEBUG, "In mark_list_as_hide(): hiding symbol %s",sym->ls_name));
			}

		else if (!quiet) {
			if (!fatal) /* print header */ {
				pfmt(stderr,MM_NOSTD,":1519:Cannot hide symbol\n");
				fatal=1;
			}
			pfmt(stderr,MM_NOSTD,":1520:%s\n", *ptr);
		      }

	}
	if (fatal)
		lderror(MSG_FATAL, gettxt(":1521","hidden symbols require a global or weak definition"));
}

/* Add all global and weak symbols to the .dynsymtab except for those symbols
** listed with -Bhide.
*/

void
add_exports_to_dynsymtab()
{
	register Word	h;
	Listnode*	np;
	Ldsym*		sym;

	if (bhide_list && Bflag_hide == LIST)
		mark_list_as_hide(bhide_list, FALSE);
	if (qhide_list && qflag_hide == LIST)
		mark_list_as_hide(qhide_list, TRUE);

	for (h = 0; h < NBKTS; h++) {
		for (LIST_TRAVERSE(&symbucket[h], np, sym)) {
			assert(sym->ls_syment != NULL && sym->ls_name != NULL);
			if (sym->ls_deftag == REF_RELOBJ 
			  && 
			   (ELF_ST_BIND(sym->ls_syment->st_info) == STB_GLOBAL 
			   || ELF_ST_BIND(sym->ls_syment->st_info) == STB_WEAK)
			  &&
			   !(znflag && sym->ls_syment->st_shndx == SHN_UNDEF) 
			  &&
			   sym->ls_Bhide == FALSE )
			{
				DPRINTF(DBG_SYMS, (MSG_DEBUG, "exporting symbol %s",sym->ls_name));
				BUMP_DYN(sym->ls_name);
				sym->ls_Bexport = TRUE;
			}

			if (sym->ls_Bhide) 
				sym->ls_Bhide = FALSE;  /* still want the
					symbol in the .symtab as a GLOBAL */
		}
	}
}

/* add an undefined symbol to the symbol table - -u name option */
void
add_usym(name)
	CONST char	*name;
{
	register Ldsym	*osym;
	register Sym	*nsym;

	/* the only symbols that could exist in the symbol table
	 * are those seen via -u option
	 */

	if (sym_find(name, NOHASH) == NULL) {
		nsym = NEWZERO(Sym);
		nsym->st_info = ELF_ST_INFO(STB_GLOBAL,STT_NOTYPE);
		nsym->st_shndx = SHN_UNDEF;
		osym = sym_enter(nsym, name, NOHASH);
		osym->ls_deftag = REF_RELOBJ;
		osym->ls_flptr = NEWZERO(Infile);
		osym->ls_flptr->fl_name = command_line;
		osym->ls_flptr->fl_e_type = ET_REL;
		BUMP_GLOB(name)
	}
}

/* build_specsym(name,uname)
** build symbol table entry for special symbols
** takes special symbol name with and without underscores.
** If definitions for either name exists, that symbol
** is left alone. If a reference for either name exists,
** that reference becomes an absolute symbol with value == value
** if symbol name with an underscore does not exist, create an
** absolute symbol - return pointers to both symbols if they exist
*/

void
build_specsym(name, uname)
	CONST char	*name,
			*uname;
{

	register Ldsym	*sym,
			*usym;
	Sym		*nsym; 

	sym = sym_find(name, NOHASH);
	usym = sym_find(uname, NOHASH);
	if (usym) {
		/* symbol with underscore exists */
		if (usym->ls_syment->st_shndx == SHN_UNDEF) {
			/* undefined reference - update values */
			usym->ls_syment->st_shndx = SHN_ABS;
			usym->ls_syment->st_info = ELF_ST_INFO(STB_GLOBAL,STT_OBJECT);
		}

		DPRINTF(DBG_SYMS, (MSG_DEBUG, "updating special symbol %s",uname));

		if (usym->ls_deftag < REF_DEFN) {
			BUMP_GLOB(uname)
			if(dmode && !Gflag){
				usym->ls_deftag = REF_DYN;
				BUMP_DYN(uname)
			} else
				usym->ls_deftag = REF_RELOBJ;
		}
		/* REF_DEFN means ".so symbol resolves global in .o file".
		** This situation arises when building a .so with other .so's
		** in the search path.  Special symbols should be resolved 
		** in this manner; change to REF_DYN or REF_RELOBJ here.
		*/
		else if (usym->ls_deftag == REF_DEFN) {
			if(dmode && !Gflag){
				usym->ls_deftag = REF_DYN;
			} else
				usym->ls_deftag = REF_RELOBJ;
		}
	}
	else {
		/* no such symbol - create one */
		nsym = NEWZERO(Sym);
		nsym->st_shndx = SHN_ABS;
		nsym->st_info = ELF_ST_INFO(STB_GLOBAL,STT_OBJECT);
		usym = sym_enter(nsym, uname, NOHASH);
		BUMP_GLOB(uname)
		if(dmode && !Gflag){
			usym->ls_deftag = REF_DYN;
			BUMP_DYN(uname)
		} else
			usym->ls_deftag = REF_RELOBJ;

		DPRINTF(DBG_SYMS, (MSG_DEBUG, "creating special symbol %s",uname));

	}
	if (sym) {
		if (sym->ls_syment->st_shndx == SHN_UNDEF) {
			/* undefined reference - update values */
			sym->ls_syment->st_shndx = SHN_ABS;
			sym->ls_syment->st_info = ELF_ST_INFO(STB_WEAK,ELF_ST_TYPE(sym->ls_syment->st_info));
			usym->ls_syment->st_info = ELF_ST_INFO(ELF_ST_BIND(sym->ls_syment->st_info),STT_OBJECT);
		}

		DPRINTF(DBG_SYMS, (MSG_DEBUG, "creating special symbol %s",name));

		if (sym->ls_deftag < REF_DEFN) {
			BUMP_GLOB(name)
			if(dmode && !Gflag){
				sym->ls_deftag = REF_DYN;
				BUMP_DYN(name)
			} else
				sym->ls_deftag = REF_RELOBJ;
		}
	}
}



/*
** process_symtab(Elf_Scn scn)
** process the global symbols in the current file's symbol table(s)
** new symbols are added via calls to sym_enter
** symbols matching names already seen are resolved via sym_resolv
*/
void
process_symtab(scn)
	Elf_Scn	*scn;
{
	Shdr		*stshdr;	/* pointer to symbol table section header */
	register Sym	*s; 		/* pointer to first (index 1) symbol in file symbol table */
	register Sym	*ts; 		/* pointer to first (index 1) symbol in file symbol table */
	Sym		*es, *els;	/* last (local) symbol */
	register Word	ndx;		/* temp index through symbol table */
	Word		numsyms;	/* number of symbols in this symbol table */
	Ldsym 		*ldptr;		/* pointer into internal ld symbol table */

	DPRINTF(DBG_SYMS, (MSG_DEBUG, "processing syms for %s", cur_file_name));
	stshdr = my_elf_getshdr(scn);
	if (stshdr->sh_size <= 0)
		lderror(MSG_FATAL, gettxt(":1135","symbol table has size 0"));
	else if (stshdr->sh_entsize <= 0)
		lderror(MSG_FATAL, gettxt(":1136","symbol table has erroneous entry size"));

	numsyms = stshdr->sh_size / stshdr->sh_entsize;
	cur_infile_ptr->fl_oldindex = (Ldsym**) mycalloc((numsyms+1)*sizeof(Ldsym*));
	s = (Sym* )((my_elf_getdata(scn,NULL))->d_buf);

	es = s + numsyms;
	ndx = 1;
	cur_infile_ptr->fl_countlocs = stshdr->sh_info-1;
	cur_infile_ptr->fl_numsyms = numsyms;
	s++;
	if (stshdr->sh_info - 1 > 0) {
		if (cur_file_ehdr->e_type == ET_REL) {
			/* there are locals to save */
			els = s + (stshdr->sh_info) - 1;
			cur_infile_ptr->fl_locals = (Ldsym*)mycalloc((stshdr->sh_info)*sizeof(Ldsym));
			DPRINTF(DBG_SYMS,(MSG_DEBUG,"syms: s= %#x, els= %#x, es= %#x, info= %d, entsize=%d",s - 1,els,es,stshdr->sh_info,stshdr->sh_entsize));
			for(; s<els ;s++, ndx++ ){
				cur_infile_ptr->fl_oldindex[ndx] = &(cur_infile_ptr->fl_locals[ndx]);
				cur_infile_ptr->fl_locals[ndx].ls_syment = s;
				cur_infile_ptr->fl_locals[ndx].ls_name = (char *)symname_bits->d_buf + (size_t)s->st_name;

			DPRINTF(DBG_SYMS, (MSG_DEBUG, "local symbol read is: %s\n", cur_infile_ptr->fl_locals[ndx].ls_name));

				if( ELF_ST_TYPE(s->st_info) == STT_SECTION ){
					if ( s->st_shndx == SHN_COMMON ||
						s->st_shndx == SHN_UNDEF){
						lderror(MSG_WARNING,
							gettxt(":1137","bogus local section definition in file; section %s defined with type %s"),
							cur_infile_ptr->fl_locals[ndx].ls_name,
							s->st_shndx);
						cur_infile_ptr->fl_locals[ndx].ls_scnptr = NULL;
					} else if ( s->st_shndx == SHN_ABS)
							 cur_infile_ptr->fl_locals[ndx].ls_scnptr = NULL;
						else
							cur_infile_ptr->fl_locals[ndx].ls_scnptr = 
							    cur_infile_ptr->fl_insect_ndx[s->st_shndx];
					
				} else if ( ELF_ST_TYPE(s->st_info) != STT_FILE ) {
					if (s->st_shndx != SHN_ABS &&
						s->st_shndx != SHN_COMMON &&
						s->st_shndx != SHN_UNDEF){

						if (strip_debug && cur_infile_ptr->fl_insect_ndx[s->st_shndx] == (Insect *)-1) {
							/* symbol refer
							 * to a stripped
							 * section
							 */
							s->st_shndx = SHN_ABS;
							s->st_value = 0;
						}
						else
						cur_infile_ptr->fl_locals[ndx].ls_scnptr = 
							cur_infile_ptr->fl_insect_ndx[s->st_shndx];
					}
					count_locstr += strlen(cur_infile_ptr->fl_locals[ndx].ls_name)+1;
					count_outlocs++;
				} else {
					count_locstr += strlen(cur_infile_ptr->fl_locals[ndx].ls_name)+1;
					count_outlocs++;
				}
				cur_infile_ptr->fl_locals[ndx].ls_deftag = REF_RELOBJ;
			}
		} else { /* shared object - do not read locals */
			ndx = stshdr->sh_info; /* 1st global */
			s += stshdr->sh_info - 1;
			if (shared_object)
			{
				ts = mymalloc( sizeof(*s)*(numsyms-ndx));
				(void)memcpy(ts,s,sizeof(*s)*(numsyms-ndx));
				s = ts;
				es = s + (numsyms-ndx);

			}
		}
	}


	if (stshdr->sh_info > numsyms-1)
		return;			 /* no globals */

	for ( ; s < es; s++, ndx++ ){
		symname =  (char *)symname_bits->d_buf + (size_t)s->st_name;

		DPRINTF(DBG_SYMS, (MSG_DEBUG, "global symbol[%d] read is: %s\n",ndx,symname));

		if (ELF_ST_BIND(s->st_info) != STB_GLOBAL &&
		    ELF_ST_BIND(s->st_info) != STB_WEAK) {
			lderror(MSG_WARNING,
				gettxt(":1138","non_global symbol `%s` found in global section of symbol table"),
				symname);
			continue;
		}
		if (strip_debug &&
			( ELF_ST_TYPE(s->st_info) != STT_FILE ) &&
			( ELF_ST_TYPE(s->st_info) != STT_SECTION ) &&
			(s->st_shndx != SHN_COMMON) &&
			(s->st_shndx != SHN_ABS) &&
			(s->st_shndx != SHN_UNDEF) &&
			(cur_infile_ptr->fl_insect_ndx[s->st_shndx] == (Insect *)-1)) {
				/* definition refers to a symbol
				 * from a deleted section
				 */
				s->st_shndx = SHN_ABS;
				s->st_value = 0;
		}
		if (check_cplus_version &&
			(s->st_size == 1)) {
			const char	*cpp_sym = "__cpp_unixware_20";
			if (strcmp(symname, cpp_sym) == 0) {
				wrong_cplus_version = TRUE;
				pfmt(stderr,MM_NOSTD,":1635:attempt to load old C++ object %s\n",
					cur_infile_ptr->fl_name);
			}
		}
			
		if ((ldptr = sym_find(symname, NOHASH)) == NULL) {
			ldptr = sym_enter(s, symname, NOHASH); 
		} else {
			oldsym = ldptr;
			newsym = s;
			sym_resolv();
		}
		cur_infile_ptr->fl_oldindex[ndx] = ldptr;
		if( (dmode || aflag ) && 
			s->st_shndx == SHN_COMMON ){
			bss_align = (s->st_value<=bss_align) ? bss_align : s->st_value;
		}
	}
	return;
}

/*
** Ldsym*
** sym_enter(Sym* es, char* name, unsigned long hashval)
** enters the given file symbol entry into the ld symbol table, hashing on name
*/

Ldsym*
sym_enter(es, name, hashval)
	Sym		*es;		/* the elf file symbol table entry */
	CONST char	*name;		/* the name to hash */
	unsigned long	hashval;	/* the hashval if already known */
{
	Word		h;		/* the hash value */
	register Ldsym	*slot;		/* the slot to use */


	slot = NEWZERO(Ldsym);
	slot->ls_name = name;
	if( hashval == NOHASH )
		hashval = elf_hash((CONST char*)name);
	h = ( slot->ls_hashval = hashval ) % NBKTS;
	(void) list_append(&symbucket[h], slot);
	slot->ls_syment = es;
	slot->ls_flptr = cur_infile_ptr;
	if (cur_infile_ptr != NULL) {
		if (slot->ls_syment->st_shndx == SHN_ABS || slot->ls_syment->st_shndx ==
		    SHN_COMMON || slot->ls_syment->st_shndx == SHN_UNDEF)
			slot->ls_scnptr = NULL;
		else
			slot->ls_scnptr = cur_infile_ptr->fl_insect_ndx[es->st_shndx];
		if (cur_file_ehdr->e_type == ET_REL){
			slot->ls_deftag = REF_RELOBJ;
			BUMP_GLOB(symname)
		} else
			slot->ls_deftag = REF_UNK;
	}

	return slot;
}

/*
** sym_find(char* name, unsigned long hashval)
** finds the given name in the symbol table and returns a pointer
** to the Ldsym or NULL if not found
*/
Ldsym*
sym_find(name,hashval)
	CONST char	*name;		/* name of the symbol to find */
	unsigned long	hashval;	/* pure hash value of symbol to find, if known */
{
	Word			h;		/* hash value of name */
	register Listnode	*stcp;		/* temp ptr to node */
	register Ldsym		*ldsp;		/* slot ptr */

	if( hashval == NOHASH )
		hashval = elf_hash((CONST char*)name);
	h = hashval % NBKTS;
	for  (LIST_TRAVERSE(&symbucket[h], stcp, ldsp)){
		if ( hashval == ldsp->ls_hashval)
			if (strcmp(name, ldsp->ls_name) == SAME)
				return ldsp;
	}
	return NULL;
}






/* update_syms() 
** create section symbols for each output section
** and update the special symbols END_USYM, DATA_USYM, END_USYM,
** DYN_USYM
** for each symbol in global symbol table,
** assign new virtual address, output section index
** and output symbol table index;
** common symbols are allocated to the bss section
*/
void 
update_syms()
{
	register Word 	ndx; 
	register	Word dynndx = 1;	/* first symbol table entry is null */
	Listnode	*lptr1,
			*lptr2;
	register Os_desc *osect; 
	Addr 		bss_addr;		/* vaddr of pseudo bss insect */
	Os_desc		*bss_osect;		/* output bss section descriptor */
	Half 		bssndx = 0;
	Sg_desc		*tlast = NULL, *dlast = NULL, *seg;
	register Ldsym	*sym;
	register Word	h;
	register Listnode
			*stcp;
	Ldsym		*esym = NULL,
			*eusym = NULL;
	Boolean		firstime = TRUE;

	if (!xflag)
		ndx = 2;
		/* first symbol table entry is null, 
		 * 2nd holds file symbol
		 */
	else
		/* first symbol table entry is null */
		ndx = 1;

	/* build local symbols for each output section */
	for (LIST_TRAVERSE(&seg_list, lptr1, seg)) {

		if (seg->sg_phdr.p_type == PT_LOAD)
			if (((seg->sg_phdr.p_flags & (PF_W | PF_R)) == PF_R) &&
				(seg->sg_osectlist.head != NULL) && seg->sg_phdr.p_memsz > 0)
				tlast = seg;
			else if (((seg->sg_phdr.p_flags & (PF_W | PF_R)) == (PF_W | PF_R)) &&
				(seg->sg_osectlist.head != NULL) && seg->sg_phdr.p_memsz > 0)
				dlast = seg;
		if (!xflag) {
			for (LIST_TRAVERSE(&(seg->sg_osectlist),
				lptr2,osect)) {

				osect->os_ndxsectsym = ndx++;

				DPRINTF(DBG_SYMS, (MSG_DEBUG, "creating section symbol for output section %s",osect->os_name));

			}
		}
	}

	if (!xflag) {
		ndx = update_locs(ndx);

		if (count_hidden)
			ndx = update_hidden(ndx+1);
	}
	else
		/* must still update values for relocation */
		(void)update_locs(0);
	
	if(!rflag && !Gflag){
		if (tlast == NULL)
			lderror(MSG_WARNING, gettxt(":1139","No read-only segments found.  Setting `%s` to 0.")
				,ETEXT_USYM );
		if (dlast == NULL)
			lderror(MSG_WARNING, gettxt(":1140","No read-write segments found.  Setting `%s` to 0.")
				,EDATA_USYM);
	}

	if (aflag || dmode || local_common) {
		bss_addr = bss_sect->is_newVAddr;
		bss_osect = bss_sect->is_outsect_ptr;
		bssndx =my_elf_ndxscn(bss_osect->os_scn);
	}

	/* traverse symbol table, update information, allocate common
	 * if an undefined is found and aflag is set call print_undefs
	 */

	DPRINTF(DBG_SYMS, (MSG_DEBUG, "updating global symbols"));

	for(h = 0; h< NBKTS; h++){
	    for(LIST_TRAVERSE(&symbucket[h],stcp,sym)){
		/* assign index in output [dynamic] symbol table */

		/* add the undefined symbols to output dynamic symbol
		   table when -znodefs or we have unresolved
		   weak reference by changing ls_deftag */
		if (sym->ls_syment->st_shndx == SHN_UNDEF &&
			sym->ls_deftag == REF_RELOBJ &&
			(dmode && !Gflag) &&
			(znflag || 
			((ELF_ST_BIND(sym->ls_syment->st_info) == 
				STB_WEAK) && 
				!create_defs_for_weak_refs))) {
			sym->ls_deftag = REF_DYN;
		}
		if (sym->ls_deftag >= REF_DEFN && !sym->ls_Bhide) {
			if(dmode && !Gflag &&
				((sym->ls_deftag == REF_DEFN) || 
				(sym->ls_deftag == REF_DYN) ||
				(sym->ls_Bexport == TRUE))) {
				sym->ls_outndx = dynndx++;
			}
			else
				sym->ls_outndx = ndx++;
		}

		if (sym->ls_syment->st_shndx == SHN_UNDEF) {
			if (ELF_ST_BIND(sym->ls_syment->st_info) != STB_WEAK){
				/* ignore zeroed out symbol table entries */
				if(sym->ls_syment->st_name != 0 || sym->ls_syment->st_value != 0 ||
				    sym->ls_syment->st_size != 0 || sym->ls_syment->st_info != 0 ||
				    sym->ls_syment->st_other != 0){
					if (aflag || zdflag) 
						print_undefs(MSG_FATAL);
					if ( dmode && Gflag && Bflag_symbolic && firstime) {
						print_undefs(MSG_WARNING);
						firstime = FALSE;
						continue;
					}
				}
			}
		}
		/* assign new symbol value */
		switch(sym->ls_syment->st_shndx) {
		case SHN_ABS:
			if( strcmp(sym->ls_name,ETEXT_SYM) == SAME || strcmp(sym->ls_name,ETEXT_USYM) == SAME){
				if (tlast != NULL)
					sym->ls_syment->st_value =
						tlast->sg_phdr.p_vaddr + tlast->sg_phdr.p_filesz;
				else
					sym->ls_syment->st_value = (Addr) 0;
			} else if( strcmp(sym->ls_name,EDATA_SYM) == SAME || strcmp(sym->ls_name,EDATA_USYM) == SAME){
				if (dlast != NULL)
					sym->ls_syment->st_value =
						dlast->sg_phdr.p_vaddr + dlast->sg_phdr.p_filesz;
				else
					sym->ls_syment->st_value = (Addr) 0;
			} else if (strcmp(sym->ls_name,DYN_SYM) == SAME || strcmp(sym->ls_name,DYN_USYM) == SAME)
				if(dmode){
					sym->ls_syment->st_value = dynamic_sect->is_newVAddr;
					sym->ls_GOTndx = GOT_XDYNAMIC;
				} else
					sym->ls_syment->st_value = (Addr) 0;
			else if (strcmp(sym->ls_name, GOT_SYM) == SAME ||
                                 strcmp(sym->ls_name, GOT_USYM) == SAME) {
                                sym->ls_syment->st_value = got_sect->is_newVAddr;
				sym->ls_GOTndx = 0;
                        } else if( strcmp(sym->ls_name,END_SYM) == SAME ||
				strcmp(sym->ls_name,END_USYM) == SAME){
				if (dlast != NULL)
					sym->ls_syment->st_value =
						dlast->sg_phdr.p_vaddr + dlast->sg_phdr.p_filesz;
				else
					sym->ls_syment->st_value = (Addr) 0;
				if( strcmp(sym->ls_name,END_SYM) == SAME)
					esym = sym;
				else
					eusym = sym;
			}
			break;
		case SHN_UNDEF:
			if(sym->ls_syment->st_value != 0) {
				lderror(MSG_SYSTEM,gettxt(":1141","undefined symbol `%s` with non-zero value encountered"),
					sym->ls_name);
				break;
			}
			/* undefined weak reference - for static
			 * a.outs change to absolute definition;
			 * for dynamic a.outs and shared libraries
			 * only change if the compatibility
			 * flag was asserted (never change for
			 * relocatables).
			 */
			if ((ELF_ST_BIND(sym->ls_syment->st_info)
				== STB_WEAK) && !rflag &&
				(!dmode || create_defs_for_weak_refs))
				sym->ls_syment->st_shndx = SHN_ABS;
			break;
		case SHN_COMMON:
			/* if Bsortbss is true delay the address evalutation
			** for common symbols , we will do it later in the 
			** loop below
			*/
			if(Bsortbss) break;
			/* common symbol - if allocating common assign it
		 	 * an address in the bss section - else leave it as
		 	 * is
		 	 */
			if ((aflag || dmode ||
				sym->ls_Bhide) && !sym->ls_not_alloc){
				DPRINTF(DBG_SYMS, (MSG_DEBUG, "Assigning bss in update_sym for %s\n", sym->ls_name));
				sym->ls_syment->st_shndx = bssndx;

				/* assign value according to 
				 * current bss_addr and symbol alignment
				 */
				sym->ls_syment->st_value = ROUND(bss_addr,sym->ls_syment->st_value);
				bss_addr = sym->ls_syment->st_value + sym->ls_syment->st_size;
			}
			break;
		default: 
			if (sym->ls_deftag == REF_DEFN){
				sym->ls_syment->st_shndx = SHN_UNDEF;
				sym->ls_syment->st_value = 0;
			} else 
				if (sym->ls_deftag >= REF_RELOBJ)
					sym->ls_syment->st_shndx = 
					my_elf_ndxscn(sym->ls_scnptr->is_outsect_ptr->os_scn);

			/* in an executable, the new symbol value 
			 * is old value (offset into
		 	 * defining section) plus virtual address of
		 	 * defining section;
			 * in a relocatable, the new value is the old value
			 * plus the displacement of the section within the file
		 	 */
			if (rflag ||
				(!(sym->ls_scnptr->is_shdr->sh_flags & 
					SHF_ALLOC)))
				sym->ls_syment->st_value += sym->ls_scnptr->is_displ;
			else
				sym->ls_syment->st_value += sym->ls_scnptr->is_newVAddr;
			break;
		}
	    }
	}
	/* if Bsortbss is true evaluate the values for those symbols here.
	*/
	if(Bsortbss){
	    Listnode	*fileRecPtr;
	    Infile	*fileDataPtr;
	    for (LIST_TRAVERSE(&infile_list, fileRecPtr, fileDataPtr)){
		Insect *iPtr;
		Addr bAddr;

		int assignToBss;
		/*
		** See if the file has a .bss section associated with it
		** (this is true if it has uninitialized static defined
		** within it ). If it does then allocate addresses from
		** that section to the commons. In make_bss() in sections.c
		** we had increased the space in the .bss sections to
		** accomodate these commons. If file has no associated
		** .bss section just allocate address from bss_sect as
		** usual ( we had accounted for that in make_bss()).
		*/
		assignToBss=0;
/*
		if(fileDataPtr->fl_e_type == ET_DYN) continue;
*/

		iPtr = getIsectByName(&fileDataPtr->fl_insects, ".bss");
		if(iPtr && iPtr->is_outsect_ptr) {
			assignToBss=1;
			bAddr = iPtr->is_newVAddr + iPtr->is_initSize;
		}

		for(h=0; h<NBKTS; h++) {
			for(LIST_TRAVERSE(&symbucket[h],stcp,sym)){
			    if((sym->ls_syment->st_shndx == SHN_COMMON) 
			      && (sym->ls_flptr == fileDataPtr) && 
			      (!rflag || sym->ls_Bhide) &&
			      !sym->ls_not_alloc ){
				if (aflag || dmode){
		 			sym->ls_syment->st_shndx = bssndx;

					/* assign value according to 
				 	* current bss_addr and symbol alignment
				 	*/
			if(assignToBss){
	sym->ls_syment->st_value = ROUND(bAddr,sym->ls_syment->st_value);
	bAddr = sym->ls_syment->st_value + sym->ls_syment->st_size;
			} else {
	sym->ls_syment->st_value = ROUND(bss_addr,sym->ls_syment->st_value);
	bss_addr = sym->ls_syment->st_value + sym->ls_syment->st_size;
			}
				}
			    }
			}
		}
	   }
	}

	/* update memory size of last readable/writable loadable segment to reflect bss
	 * also update value of END_USYM and END_SYM
	 */
	if (aflag || dmode) {

		Sg_desc	*eseg;

		if (dlast == NULL) {
			if (tlast == NULL)
				goto warn;
			else
				eseg = tlast;
		} else if (tlast == NULL)
			eseg = dlast;
		else if (dlast->sg_phdr.p_vaddr > tlast->sg_phdr.p_vaddr)
			eseg = dlast;
		else if (dlast->sg_phdr.p_vaddr < tlast->sg_phdr.p_vaddr)
			eseg = tlast;
		else {
	warn:
			lderror(MSG_WARNING, gettxt(":1142","Setting `%s` and `%s` to 0."),END_SYM,END_USYM);
			eseg = NULL;
		}

		if (esym) {
			if (eseg != NULL)
				esym->ls_syment->st_value = 
				ROUND(eseg->sg_phdr.p_vaddr + eseg->sg_phdr.p_memsz,WORD_ALIGN);
			else
				esym->ls_syment->st_value = (Addr) 0;
		}

		if (eusym) {
			if (eseg != NULL)
				eusym->ls_syment->st_value  =
				ROUND(eseg->sg_phdr.p_vaddr + eseg->sg_phdr.p_memsz,WORD_ALIGN);
			else
				eusym->ls_syment->st_value = (Addr) 0;
		}
	}
}

/* void
** print_globals()
** walk through symbol table and print all symbols
*/
void 
print_globals()
{
	register Word  h;
	Listnode*	stcp;
	Ldsym *sym;

	/* print header */
	pfmt(stderr,MM_NOSTD,":1199:GLOBAL SYMBOLS:\n");
	pfmt(stderr,MM_NOSTD,":1200:shndx\ttype\tbind\tsize\tvalue\toutndx\tname\tlevel\n");
	/* step through symbol table */
	for(h = 0; h < NBKTS; h++){
		for (LIST_TRAVERSE(&symbucket[h], stcp, sym)){
			pfmt(stderr,MM_NOSTD,":1201:0x%x\t%d\t%d\t%d\t0x%x\t%d\t0x%x\t%s\t%d\n",
			sym->ls_syment->st_shndx,ELF_ST_TYPE(sym->ls_syment->st_info),
			ELF_ST_BIND(sym->ls_syment->st_info),
			sym->ls_syment->st_size,sym->ls_syment->st_value,sym->ls_outndx,
			sym->ls_name,sym->ls_deftag);
		}
	} 
}

/*
** Go through the bss_cpy_list, if the not_alloc flag is set, then
** set the value of the symbol to its synonym value.
*/
void
update_syn_value()
{
	register Listnode *ip;
	Bss_cpy 	  *dp;

	for (LIST_TRAVERSE(&bss_cpy_list,ip,dp)){
		if (dp->ldsym->ls_not_alloc == TRUE && dp->synonym_ptr != NULL
		    && dp->synonym_ptr->ldsym != NULL){
						
			dp->ldsym->ls_syment->st_value = 
				dp->synonym_ptr->ldsym->ls_syment->st_value +
				dp->data_seg_off - dp->synonym_ptr->data_seg_off;
			dp->ldsym->ls_syment->st_shndx = 
				dp->synonym_ptr->ldsym->ls_syment->st_shndx;

			DPRINTF(DBG_SYMS, (MSG_DEBUG, "Setting the value of %s to %x\n",dp->sym_name, dp->ldsym->ls_syment->st_value));
		}
	}
}

void
lookup_profiling_symbol()
{

	Ldsym  *ldsym;

	if ((ldsym = sym_find(".rtld.event", NOHASH)) != NULL){
		ldsym->ls_Bexport = TRUE; 
		BUMP_DYN(ldsym->ls_name);
	}
}
