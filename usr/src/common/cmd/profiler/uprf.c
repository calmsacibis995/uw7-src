/*		copyright	"%c%" 	*/

#ident	"@(#)profiler:uprf.c	1.2"
#ident	"$Header:"

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <libelf.h>
#include <sys/prf.h>

struct section {
	Elf32_Shdr *sec_shdr;
	Elf_Scn	*sec_scn;
} *esections;
/* round a up to next multiple of b */
#define ALIGN(a, b) ((b == 0) ? (a) : ((((a) +(b) -1) / (b)) * (b)))


struct mprf *
getsym(const char *filename, unsigned long addr)
{
	int 		symnum,fd,i;
	char 		*name;
	Elf		*elf_file;
	Elf32_Ehdr	*p_ehdr;	/* elf file header */
	Elf_Scn		*scn;		/* elf section header	*/
	Elf_Data	*sym_data;	/* info on symtab	*/
	unsigned long	strsize;
	char		*strspace;
	unsigned int	str_ndx;
	Elf32_Sym	*esym;		/* pointer to ELF symbol	*/
	struct mprf	*retprf;
	Elf32_Shdr	*shdr;
	boolean_t 	relocatable;


	if ((fd = open(filename, O_RDWR)) < 0)
		return(NULL);

	if (elf_version(EV_CURRENT) == EV_NONE)
		return(NULL);

	if ((elf_file = elf_begin(fd, ELF_C_RDWR, (Elf *)0)) == 0) {
				return(NULL);
	}

	/*
	 *	get ELF header
	 */
	if ((p_ehdr = elf32_getehdr( elf_file )) == 0) {
				return(NULL);
	}
	if(p_ehdr->e_type == ET_REL)
		relocatable = B_TRUE;
	else if(p_ehdr->e_type == ET_EXEC)
		relocatable = B_FALSE;
	else
		return(NULL);

	/*
	 *	load section table
	 */
	if((esections = calloc(sizeof(struct section), p_ehdr->e_shnum)) == NULL)
		return(NULL);

	i=1;	/* skip the first entry so indexes match with file */
	scn = 0;
	while(( scn =  elf_nextscn( elf_file,scn )) != 0 ) {
		shdr = esections[ i ].sec_shdr = elf32_getshdr( scn );
		switch(shdr->sh_type) {
			case SHT_SYMTAB:
				str_ndx = esections[ i ].sec_shdr->sh_link;
	
				esym = NULL;
				sym_data = 0;
				if ((sym_data = elf_getdata(scn, sym_data)) == 0)
					return(NULL);
				break;
			case SHT_PROGBITS:
			case SHT_NOBITS:
			case SHT_MOD:
				if(shdr->sh_flags & SHF_ALLOC) {
					addr = ALIGN(addr,shdr->sh_addralign);
					shdr->sh_addr = addr;
					addr += shdr->sh_size;
					shdr->sh_type = SHT_SHLIB; /* marker */
				}
				break;
		}
		i++;
	}
	if(esections[str_ndx].sec_shdr->sh_type != SHT_STRTAB)
		return(NULL);

	
	

	strsize=0;
	symnum=0;

	for(esym = (Elf32_Sym *)sym_data->d_buf; 
	    esym < (Elf32_Sym *) ((char *)sym_data->d_buf + sym_data->d_size); 
	    esym++) {


		/* SHT_SHLIB indicates a selected section */
/*
 *		Pick up function names.  This is rather complicated
 *		because we want to see assembler functions, which
 *		may not have been declared as STT_FUNC.
 */
		if(((ELF32_ST_TYPE(esym->st_info) == STT_FUNC) ||
		  ((ELF32_ST_TYPE(esym->st_info) == STT_NOTYPE )) &&
		  (ELF32_ST_BIND(esym->st_info) == STB_GLOBAL)) &&
		  ((esym->st_shndx & SHN_LORESERVE) != SHN_LORESERVE) &&
		  esections[esym->st_shndx].sec_shdr->sh_type == SHT_SHLIB) {
			name = elf_strptr( elf_file, str_ndx, (size_t)esym->st_name);

			esym->st_other = 0xff; 		/* marker */
			strsize += strlen(name)+1;
			symnum++;
		}
	
	}

	if((retprf = calloc(1, (symnum + 1)*sizeof(struct mprf) + strsize)) == NULL)
		return(NULL);

	strspace = (char *) retprf + (symnum +1)*sizeof(struct mprf);

	retprf->mprf_addr = symnum;
	retprf->mprf_offset = strsize;

	strsize = 0;
	symnum = 1;
	for(esym = (Elf32_Sym *)sym_data->d_buf; 
	    esym < (Elf32_Sym *) ((char *)sym_data->d_buf + sym_data->d_size); 
	    esym++) {


		/* st_other == 0xff indicates desired symbol */
		if(esym->st_other == 0xff) {
			name = elf_strptr( elf_file, str_ndx, (size_t)esym->st_name);

			if(relocatable)
				retprf[symnum].mprf_addr = esym->st_value + 
				   esections[esym->st_shndx].sec_shdr->sh_addr;
			else
				retprf[symnum].mprf_addr = esym->st_value;

			retprf[symnum].mprf_offset = strsize;
			strcpy(strspace+strsize,name);
			strsize += strlen(name)+1;
			symnum++;
		}
	
	}

	free(esections);
	elf_end(elf_file);

	close(fd);
	return(retprf);
}

