#ident	"@(#)dis:common/utls.c	1.14"

#include	<stdio.h>

#include	"dis.h"
#include	"sgs.h"
#include	"structs.h"
#include	"libelf.h"
#include	"ccstypes.h"
#include	"libdwarf2.h"

#ifdef __STDC__
#include <stdlib.h>
#endif

#define TOOL	"dis"


extern void	exit();

/*
 *	This structure is used for storing data needed for the looking up
 *	of symbols for labels.  It consists of a pointer to the labels' name
 *	and it's location within the section, and a pointer to another
 *	structure of its own type (a forward linked list will be created).
 */

struct	LABELS {
	unsigned	char	*label_name; 
	long	label_loc;
	struct	LABELS *next;
};

typedef struct LABELS elt1;
typedef elt1 *link1;
static	link1	fhead = NULL, ftail;



FUNCLIST	*currfunc;

static		Elf32_Rel	*rel_data;
static		size_t		no_rel_entries;
static		size_t		no_sym_entries;
static		Elf32_Sym	*rel_sym_data;
static		Elf32_Word	rel_sym_str_index;
static		char		*sym_name;

extern		int		Rel_data;
extern		int		Rela_data;
/*								*/
/*  Determine if the current section has a relocation section.	*/
/*  If it does, then its relocation section will be searched  	*/
/*  during the symbolic disassembly phase.			*/
/*								*/

int
get_rel_section(sec_no)
Elf32_Word sec_no;
{
	extern	Elf	*elf;
	Elf_Scn         *scn,  *scn1;
	Elf32_Shdr	*shdr, *shdr1;
	Elf_Data	*data, *data1;

	Rel_data = FAILURE;
	scn = 0;
	while ((scn = elf_nextscn(elf,scn)) != 0)
	{
       		if ((shdr = elf32_getshdr(scn)) != 0)
               	{
			if ( shdr->sh_type == SHT_REL &&
				 shdr->sh_info == sec_no )
			{
				/* get relocation data */
				data = 0;
				if ((data = elf_getdata(scn, data)) == 0 ||
				     data->d_size == 0 )
					return(FAILURE);
				
				rel_data = (Elf32_Rel *)data->d_buf;
				no_rel_entries = data->d_size/
							 sizeof(Elf32_Rel);

				/* get its associated symbol table */
				if ((scn1 = elf_getscn(elf,shdr->sh_link)) != 0)
				{
					/* get index of symbol table's string table */
					if ((shdr1 = elf32_getshdr(scn1)) != 0)
						rel_sym_str_index = shdr1->sh_link;
					else
						return(FAILURE);	
					data1 = 0;
					if ((data1 = elf_getdata(scn1, data1))
					      == 0 || data1->d_size == 0 )	
						return(FAILURE);
					rel_sym_data = (Elf32_Sym *)data1->d_buf;
					no_sym_entries = data1->d_size/sizeof(Elf32_Sym);
				}
				else
					return(FAILURE);	

				Rel_data = SUCCESS;
				return(SUCCESS);
			}
		}
		else
			return(FAILURE);
	}
	return(FAILURE);
}


void
search_rel_data(in_offset,val,pos)
Elf32_Addr in_offset;
long val;
char **pos;
{
	extern		Elf	*elf;
	extern		int	oflag;

	Elf32_Rel	*rel_data1;
	size_t          no_rel_entries1;
	size_t		sym;
	Elf32_Sym	*symbol;

	no_rel_entries1 = no_rel_entries;

	if (Rel_data)
	{
		rel_data1 = rel_data;
		while(no_rel_entries1--)
		{
			if (in_offset == rel_data1->r_offset)
			{
				sym = ELF32_R_SYM(rel_data1->r_info);
				if ( (sym <= no_sym_entries) && (sym >= 1) )
				{
					symbol = rel_sym_data + sym;
					sym_name = (char *) elf_strptr(elf, rel_sym_str_index, symbol->st_name);
					*pos+=sprintf(*pos,"%s",sym_name);
					return;
				}
				else
				{
					*pos+=sprintf(*pos, oflag?"0%o":"0x%x",val);
					return;
				}
			}
			rel_data1++;
		}
		*pos+=sprintf(*pos, oflag?"0%o":"0x%x",val);
		return;
	}
}

void
locsympr(val, regno, pos)
long val;       /* offset from base register */
int regno;      /* base register */
char **pos;     /* postion in output string */
{
	extern	int	oflag,
			trace;
	extern	char	**regname;

	if (trace)
		(void)printf("\noffset from base reg %d is %ld\n", regno, val);

	if (regno == -1)
		*pos+=sprintf(*pos,"%s",regname[val]);
	else
		*pos+=sprintf(*pos,OFLAG,val,regname[regno]);
	return;
}



void
extsympr(val,pos)
long val;       /* address of current operand */
char **pos;     /* position in output string */
{
	extern	int	symtab;
	extern	int	oflag;
	extern  Elf     *elf;
        Elf_Scn         *scn;
	Elf32_Shdr	*shdr;
        Elf_Data        *data;
	Elf32_Sym	*sym_data;
	size_t		no_sym_entries;
	char		*sym_name;
	int		sym_type;
	

	if (( scn = elf_getscn(elf, symtab)) != 0)
	{
	  if ((shdr = elf32_getshdr(scn)) != 0)
	  {	
		data = 0;
		if ((data = elf_getdata(scn, data)) != 0 && data->d_size != 0 )
		{
			sym_data = (Elf32_Sym *)data->d_buf;
			sym_data++;
			no_sym_entries = data->d_size/sizeof(Elf32_Sym);
		
		   while(no_sym_entries--)
		   {
			sym_type = ELF32_ST_TYPE(sym_data->st_info);		
			if ( ( (sym_type == STT_OBJECT || sym_type ==STT_NOTYPE)
			        && ( val == sym_data->st_value || 
				(val >= sym_data->st_value && 
			         val < sym_data->st_value + sym_data->st_size)))
			     ||( sym_type == STT_FUNC && 
				 val == sym_data->st_value) )
			{
				sym_name = (char *) elf_strptr(elf, shdr->sh_link, sym_data->st_name);
				if (val - sym_data->st_value) 
				  *pos+=sprintf(*pos, "(%s+%ld)", 
					sym_name, val - sym_data->st_value);
				else 
				  *pos+=sprintf(*pos, "%s", sym_name);
				return;
			}
			sym_data++;	
		   }
		}
	   }
	}

	*pos+=sprintf(*pos, oflag?"0%o":"0x%x",val);
	return;
}


/*
 *	build_labels ()
 *
 *	Construct a forward linked structure containing all the label entries 
 *	found in the  .debug section.  This is needed in looking up the labels.
 */

void
build_labels(name, location)
unsigned char	*name;
long	location;
{
	extern int	trace;

	if (fhead == NULL)
        {
                fhead = (link1) malloc(sizeof(elt1));
                ftail = fhead;
        }
        else
        {
                ftail->next = (link1) malloc(sizeof(elt1));
                ftail = ftail->next;
        }
        ftail->label_name = name;
	ftail->label_loc  = location;
        ftail->next = NULL;

	if (trace > 0) 
		(void)printf("\nlabel_name %s and location %ld\n",
			 ftail->label_name, ftail->label_loc);
        return;
}


void
label_free()
{
	link1 fheadp;
	link1 temp;

	if (fhead == NULL)
		return;

	fheadp = fhead;
	while (fheadp)
	{
		temp = fheadp;
		fheadp = fheadp->next;
		free(temp);
	}

	ftail = fhead = NULL;
	return;
}


/*
 *	compoff (lng, temp)
 *
 *	This routine will compute the location to which control is to be
 *	transferred.  'lng' is the number indicating the jump amount
 *	(already in proper form, meaning masked and negated if necessary)
 *	and 'temp' is a character array which already has the actual
 *	jump amount.  The result computed here will go at the end of 'temp'.
 *	(This is a great routine for people that don't like to compute in
 *	hex arithmetic.)
 */

void
compoff(lng, temp)
long	lng;
char	*temp;
{
	extern	int	oflag;	/* from _extn.c */
	extern	long	loc;	/* from _extn.c */

	lng += loc;
	if (oflag)
		(void)sprintf(temp,"%s <%lo>",temp,lng);
	else
		(void)sprintf(temp,"%s <%lx>",temp,lng);
	return;
}


/*
 *      void lookbyte ()
 *
 *      read a byte, mask it, then return the result in 'curbyte'.
 *      The byte is not immediately placed into the string object[].
 *      is incremented.
 */

void
lookbyte()
{
	extern unsigned short curbyte;
        extern  long    loc;            /* from _extn.c */
	extern unsigned char *p_data;
	unsigned char *p = (unsigned char *)&curbyte;

	*p = *p_data; ++p_data;
        loc++;
        curbyte = *p & 0377;
}


void
get_line_info()
{
	extern 	Elf	*elf;
	extern	int	line;
	extern	unsigned	char *ptr_line_data;
	extern	size_t		size_line;
	extern	Dwarf2_Line_entry *LineTable;
	extern	Dwarf2_Line_entry *get_line_table();
	extern	int	Dwarf;

	Elf_Scn	*scn;
	Elf_Data	*line_data;
	Elf32_Shdr      *shdr;

	if ( (scn = elf_getscn(elf, line)) == NULL)
	{
		(void) fprintf(stderr,
		"%sdis: failed to get section .line; limited functionality\n"
		,SGS);
		line = 0;
		return;
	}
	else
	if ((shdr = elf32_getshdr(scn)) != 0)
	{
		line_data = 0;	
		if ((line_data=elf_getdata(scn, line_data))==0 || line_data->d_size ==0)
		{
			(void) fprintf(stderr,
			"%sdis: no data in section .line\n", SGS);
			line = 0;
			return;
		}
		else
		{
			ptr_line_data = (unsigned char *)line_data->d_buf;
			size_line	= line_data->d_size;
		}
	}
	if (Dwarf == 2)
		LineTable = get_line_table(line_data->d_buf, line_data->d_size);
}
	
/*
 *	line_nums ()
 *
 *	This function prints out the names of functions being disassembled
 *	and break-pointable line numbers.  First it checks the address
 *	of the next function in the list of functions; if if matches
 *	the current location, it prints the name of that function.
 *
 *	It then examines the line number entries. If the address of the
 *	current line number equals that of the current location, the
 *	line number is printed.
 */

void
line_nums()
{
	extern	int		oflag;
	extern	int		line;
	extern	int		Dwarf;
	extern	void		print_line();
	extern	unsigned	char	*ptr_line_data;
	extern	size_t		size_line;
	extern	long		loc;
	extern	FUNCLIST	*next_function;
	extern	FUNCLIST	*currfunc;
	unsigned long		index;

	
	while (next_function != NULL)
	{
		/* not there yet */
		if (loc < next_function->faddr)
			break;

		if (loc > next_function->faddr)
		{
			/* this is an error condition */
			(void)fflush( stdout );
			(void)fprintf( stderr, "\nWARNING: Possible strings in text or bad physical address before location ");
			if (oflag) 
				(void)fprintf( stderr, "0%lo\n", loc );
			else
				(void)fprintf( stderr, "0x%lx\n", loc );
			/* (void)FSEEK( t_ptr, (next_function->faddr - scnhdr.s_paddr) + scnhdr.s_scnptr, 0); */
			loc = next_function->faddr;
		}

		if (next_function->stt_type == STT_FUNC)
			(void)printf("%s()\n", next_function->funcnm);
		else
			(void)printf("%s\n", next_function->funcnm);

		currfunc = next_function;
		next_function = next_function->nextfunc;

	}

	if (line)
		if (Dwarf == 1)
			print_line(loc, ptr_line_data, size_line);
		else
			print_debug_line(loc, ptr_line_data, size_line);

	(void)printf("\t");
}


/*
 *	looklabel (addr)
 *
 *	This function will look in the symbol table to see if
 *	a label exists which may be printed.
 */
void
looklabel(addr)
long	addr;
{
	link1	label_ptr;

	if (fhead == NULL) 	/* the file may have debugging info by default, */
		return;	   	/* but it may not have label info becuse 	*/
				/* label info in only put in with "cc -g".	*/

	label_ptr = fhead;
	while (label_ptr != NULL)
	{
		if (label_ptr->label_loc == addr )
			/* found the label so print it	*/
			(void)printf("%s:\n",label_ptr->label_name);
		label_ptr = label_ptr->next;
	}
	return;
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


/*
 *	resync ()
 *
 *	If a bad op code is encountered, the disassembler will attempt
 *	to resynchronize itself. The next line number entry and the
 *	next function symbol table entry will be found. The restart
 *	point will be the smaller of these two addresses and bytes
 *	of object code will be dumped (not disassembled) until the
 *	restart point is reached.
 */
int
resync()
{
	return(0);
}

/*
 *	fatal()
 *
 *	print an error message and quit
 */
void
fatal( message )
char	*message;
{
	extern 	char *fname;
	extern  Elf_Arhdr       *mem_header;
	extern	int		archive;

	if (archive)
		(void)fprintf(stderr, "\n%s%s: %s[%s]: %s\n", SGS, TOOL, fname, mem_header->ar_name, message);
	else
		(void)fprintf(stderr, "\n%s%s: %s: %s\n", SGS, TOOL, fname,  message);

	exit(1);
	/*NOTREACHED*/
}
