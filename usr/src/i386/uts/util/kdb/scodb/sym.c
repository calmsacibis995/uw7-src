#ident	"@(#)kern-i386:util/kdb/scodb/sym.c	1.2.1.1"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1995.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Modification History:
 *
 *	L000	scol!nadeem	22apr92
 *	- add new function pns() to print out a number with leading
 *	  spaces.  Used in ps().
 *	L001	scol!nadeem	23apr92
 *	- from scol!blf 12jun91
 *	- Don't use the symbol table unless it exists.
 *	L002	scol!nadeem	12may92
 *	- added support for BTLD.  The new scodb uses a different symbol table
 *	  format than the old scodb.  This causes problems when trying to BTLD
 *	  the new scodb into the kernel, because /boot currently builds a
 *	  symbol table for scodb in the old format.  There is not enough code
 *	  space in /boot at present to allow it to recognise which scodb is
 *	  being loaded and build the symbol table of the appropriate format.
 *	  As a result, the new scodb will recognise both the old and the new
 *	  symbol table format and will use the old symbol table format when it
 *	  has been BTLD'd into a kernel.  This has the advantage that the new
 *	  scodb can be BTLD'd into an old (ie: 3.2v4 final) kernel, and also
 *	  that when /boot eventually generates the new symbol table format
 *	  then no changes will be needed to scodb.
 *	- this has meant splitting references to "struct sent" and "symt"
 *	  into "struct new_sent", "struct old_sent", "new_symt", and "old_symt"
 *	  as appropriate.  These are unmarked as they are so prolific.
 *	L003	scol!nadeem	12jun92
 *	- added new routine sent_prev() due to scodb being modified to handle
 *	  both new and old symbol table format.  Given a symbol entry, return
 *	  a pointer to the previous entry in the table.  Used to correct
 *	  disassembly bug in dis.c.
 *	L004	scol!nadeem	14jul92
 *	- correct the outputting of addresses in "symbol+offset" form such that
 *	  in a restricted field the symbol is truncated rather than the
 *	  offset.  So, for example, you would now get "a_long_symbol_na+14c"
 *	  instead of the potentially confusing "a_long_symbol_name+1".
 *	  Created prst_sym() and prstb_sym() for this purpose.
 *	L005	scol!nadeem	31jul92
 *	- added support for static text symbols.  Defined new routine
 *	  findsym_global().
 *	L006	scol!nadeem	13jul93
 *	- Removed the need for separate Driver.o and Driver.boot object modules.
 *	  Removed all "#ifdef BOOT" variable definitions and moved then into
 *	  the space.c file instead.  This used to be the only module which
 *	  was different between Driver.o and Driver.boot, so with these changes
 *	  the Driver.o module becomes generic.
 *	L007	scol!nadeem	11aug93
 *	- added support for displaying source code line numbers during
 *	  disassembly.  Added three new functions; pnd() for converting a
 *	  number to decimal output, db_line_init() for initialising the
 *	  new line number table, and db_search_lineno() for determining
 *	  whether a particular address has a line number associated with it.
 *	L008	scol!hughd	06feb95
 *	- added hiding of unhelpful symbols to findsym() and findsym_global(),
 *	  based on the new layout of the vuifile, as in mskdb's findsymname()
 *	L009	nadeem@sco.com	13dec94
 *	- added support for user level scodb:
 *	- move ufind() routine from bkp.c to here.  This is because bkp.c is
 *	  not compiled into the user level binary but ufind() is still needed,
 *	- modified ufind() to have a mode whereby it does not prompt the user
 *	  with a list of partially matching symbols whilst the user level scodb
 *	  is starting up.  This is because ufind() is used (via findsymbyname())
 *	  to scan for certain required symbols during startup, and if they are
 *	  not present we want to fail with an error rather than prompt the user
 *	  for possible matches.
 *	- db_sym_init() is obsolete and so is #ifdef'd out.  It's function
 *	  has been replaced with the routine load_symbols() which loads
 *	  symbols from the namelist.
 *	- created new routine pnsb() to print out a number to a buffer.  This
 *	  is based around pns().
 */

#include	<util/param.h>
#include	<util/mod/mod_obj.h>
#include	"dbg.h"
#include	"sent.h"
#include	"stunv.h"
#include	"bkp.h"

/*
*	Old structure of symbol table:
*		Array of structures
*			structures have
*				vaddr		( 4b)
*				flags		( 1b)
*				sym name	(15b)
*
*	New structure of symbol table:
*		Array of structures
*			structures have
*				vaddr			(4b)
*				flags			(1b)
*				sym ptr	into strings	(4b)
*		String table
*/

int ufind_do_query = 1;						/* L009 */

int db_nlineno = 0;						/* L007 */
struct scodb_lineno *db_lineno = NULL;				/* L007 */

/*
 * Local symbol table
 */

char *lsym_getname();
Elf32_Sym *lsym_byvalue(), *lsym_byname();
int lsym_entries = 0;
Elf32_Sym *lsym_symtab = NULL;
char *lsym_strtab = NULL;

/*
 * Maximum symbolic offset possible for "<sym>+<offset>" output.
 */

unsigned scodb_maxsymoff = 0x4000;

extern char scodb_kstruct_info[];

#ifndef USER_LEVEL						/* L009 */

/*
*	As loaded, the symbol structures contain offsets to the string
*	table.  Here, we change the offsets to be pointers.
*/

db_sym_init()
{
	char *lsym_p;

	if (scodb_kstruct_info) {
		struct scodb_usym_info *infop = (struct scodb_usym_info *)
							scodb_kstruct_info;

		if (infop->magic != USYM_MAGIC) {
			printf("Bad magic number from unixsyms info at %x\n",
				scodb_kstruct_info);
			return;
		}

		if(infop->lsym_offset == NULL) 
			return;

		lsym_p = (char *)scodb_kstruct_info + infop->lsym_offset;
		lsym_entries = *(int *)lsym_p;
		lsym_symtab = (Elf32_Sym *)((char *)lsym_p + sizeof(lsym_entries));
		lsym_strtab = lsym_p + sizeof(lsym_entries) + (lsym_entries * sizeof(Elf32_Sym));
	}
}

#endif								/* L009 */

STATIC
is_text(unsigned vaddr)
{
	extern char stext[], _etext[];

	return ((char *)vaddr >= stext && (char *)vaddr < _etext);
}

/*
*	find the symbol at or before vaddr
*	if vaddr is before the first symbol, return NULL
*/

NOTSTATIC							/* v L002 v */
struct sent *
findsym(unsigned vaddr)
{
	static struct sent sentstr;
	ulong_t offset;
	char *name;
	Elf32_Sym sym, *sp, *lsp;

	sp = &sym;
	name = (char *)mod_obj_getsymp_byvalue(vaddr, &offset, NOSLEEP, NULL, sp);

	lsp = lsym_byvalue(vaddr);	/* look in local symbols as well */

	if (name == NULL && lsp == NULL) {
		return(NULL);
	}

	/*
	 * Check if local symbol (if any) was closer than global symbol.
	 */

	if ((name == NULL || (lsp != NULL && vaddr - lsp->st_value < offset))) {
		sp = lsp;
		offset = vaddr - lsp->st_value;
		name = lsym_getname(sp);
	}

	/*
	 * Check if the offset is within sensible limits.
	 */

	if (offset > scodb_maxsymoff)
		return(NULL);

	strcpy(&sentstr.se_name[0], name);
	sentstr.se_vaddr = vaddr - offset;

	if (ELF32_ST_TYPE(sp->st_info) == STT_FUNC)
		sentstr.se_flags = SF_TEXT;
	else
		sentstr.se_flags = SF_DATA;
	
	return (&sentstr);
}

/*
 * Find the global symbol at or before vaddr, ignoring static symbols.
 */

NOTSTATIC
struct sent *
findsym_global(unsigned vaddr)
{
	return (findsym(vaddr));
}

/*
*	find symbol given the beginning of a name...
*
*	very expensive, as symbols are sorted by address, not
*	name.
*/

NOTSTATIC
struct sent *
findsymbyname(nm)
char *nm;
{
	static struct sent sentstr;
	unsigned err;
	Elf32_Sym sym, *sp, *lsp;

	sp = &sym;

	err = mod_obj_getsymp_byname(nm, B_FALSE, NOSLEEP, sp);

	lsp = lsym_byname(nm);

	if (err) {
		if (lsp == NULL)
			return(NULL);
		else
			sp = lsp;
	}

	sentstr.se_vaddr = sp->st_value;
	strcpy(&sentstr.se_name[0], nm);

	if (ELF32_ST_TYPE(sp->st_info) == STT_FUNC)
		sentstr.se_flags = SF_TEXT;
	else
		sentstr.se_flags = SF_DATA;
	
	return (&sentstr);
}

/*
 * Given a pointer to a symbol entry, return a pointer to the symbol name.
 * Used to hide the format of the symbol name from the user due to the
 * differences between the old and new symbol table format.
 */

char *								/* L002 v */
sent_name(s)
struct sent *s;
{
	return (&s->se_name[0]);
}								/* L002 ^ */

/*
 * Given a pointer to a symbol entry, return the previous symbol entry.
 */

struct sent *							/* L003 new */
sent_prev(s)
struct sent *s;
{
	char *name;
	ulong_t offset;
	static struct sent sentstr;
	Elf32_Sym sp;

	name = (char *)mod_obj_getsymp_byvalue(s->se_vaddr - 1, &offset, NOSLEEP,
			       &sentstr.se_name[0], &sp);

	sentstr.se_vaddr = s->se_vaddr - offset - 1;

	if (ELF32_ST_TYPE(sp.st_info) == STT_FUNC)
		sentstr.se_flags = SF_TEXT;
	else
		sentstr.se_flags = SF_DATA;
	
	return (&sentstr);
}

/*
*	find the vaddr given by the vaddr of a symbol plus an offset
*/
NOTSTATIC
offset(symn, offs)
	char *symn;
	long offs;
{
	struct sent *findsymbyname();
	struct sent *s;

	s = findsymbyname(symn);
	if (s == NULL)
		return -1;
	return s->se_vaddr + offs;
}

/*
*	prirt out n, a number, nd digits long, with 0 padding
*/
NOTSTATIC
pnz(n, nd)
	register int n, nd;
{
#define		HEXDIGS	"0123456789ABCDEF"
 	for (--nd;nd >= 0;nd--)
 		putchar(HEXDIGS[(n >> (4 * nd)) & 0xF]);
}

/*
*	prirt out n, a number, nd digits long, with 0 padding,
*	to a buffer
*/
NOTSTATIC
pnzb(bp, n, nd)
	register char **bp;
	register int n, nd;
{
#define		HEXDIGS	"0123456789ABCDEF"
 	for (--nd;nd >= 0;nd--)
 		*(*bp)++ = HEXDIGS[(n >> (4 * nd)) & 0xF];
}

/*
 * Print out n, nd digits long, with space padding.
 */

pns(n, nd)							/* v L000 v */
	register int n, nd;
{
	int i, do_padding = 1;

#define		HEXDIGS	"0123456789ABCDEF"
 	for (--nd;nd >= 0;nd--) {
 		i = (n >> (4 * nd)) & 0xF;
		if (i == 0 && do_padding && nd)
			putchar(' ');
		else {
			do_padding = 0;
			putchar(HEXDIGS[i]);
		}
	}
}								/* ^ L000 ^ */

/*
 * Print out n, nd digits long, with space padding, to a buffer.
 */

pnsb(bp, n, nd)							/* L009v */
	register char **bp;
	register int n, nd;
{
	int i, do_padding = 1;

#define		HEXDIGS	"0123456789ABCDEF"
 	for (--nd;nd >= 0;nd--) {
 		i = (n >> (4 * nd)) & 0xF;
		if (i == 0 && do_padding && nd)
			*(*bp)++ = ' ';
		else {
			do_padding = 0;
			*(*bp)++ = HEXDIGS[i];
		}
	}
}								/* L009^ */

/*
*	print the number l into a buffer.
*	non-zero padded, up to 8 digits long
*/
NOTSTATIC
pn(s, l)
	register char *s;
	register unsigned long l;
{
	register int nc;
	char c, *bs = s;

	if (l == 0)
		*s++ = '0';
	else {
		nc = 0;
		while ((l & 0xF0000000) == 0) {
			l <<= 4;
			nc++;
		}
		for (;nc < 8;nc++) {
			c = (l & 0xF0000000) >> 28;
			if (c > 9)
				*s++ = c - 10 + 'A';
			else
				*s++ = c + '0';
			l <<= 4;
		}
	}
	*s = '\0';
	return s - bs;
}

/*
 * Convert number n into decimal and place into buffer s.
 * Number is not padded and is null terminated.
 */

pnd(s, n)							/* L007v */
char *s;
unsigned n;
{
	register unsigned i, j, power;
	unsigned leading;

	if (n == 0) {
		*s++ = '0';
		*s++ = '\0';
		return;
	}

	power = 1000000000;
	leading = 1;

	i = n;
	while (power > 0) {
		j = i / power;
		if (j == 0) {
			if (!leading)
				*s++ = j + '0';
		} else {
			leading = 0;
			*s++ = j + '0';
		}
		i = i - j * power;
		power /= 10;
	}
	*s = '\0';
}								/* L007^ */

/*
*	print a string, space padded to l length
*/
NOTSTATIC
prst(s, l)
	register char s[];
	register int l;
{
	register int i;

	for (i = 0;i < l && s[i];i++)
		putchar(s[i]);
	for (;i < l;i++)
		putchar(' ');
}

/*
*	print a string, space padded to l length, to a buffer
*/
NOTSTATIC
prstb(bp, s, l)
	register char **bp;
	register char s[];
{
	register int i;

	for (i = 0;i < l && s[i];i++)
		*(*bp)++ = s[i];
	for (;i < l;i++)
		*(*bp)++ = ' ';
}

/*
 *	print a "symbol+offset" string in a space of length l,
 *	truncating the symbol as necessary.
 */

NOTSTATIC							/* L004 v */
prst_sym(s, l)
	char s[];
	register int l;
{
	register int i;
	register char *p;
	char *plus_ptr = NULL;

	/*
	 * Search for a "+" in the string, and also for the end of the string.
	 */
	
	p = s;
	while (*p) {
		if (*p == '+')
			plus_ptr = p;
		p++;
	}

	/*
	 * No symbol truncation is done if the string fits within
	 * the field length, or there is no '+offset'
	 * part to the string. 
	 */

	i = p - s;
	if (i <= l || plus_ptr == NULL) {		
		p = s;
		if (i >= l) {
			i = l;			/* truncate the string */
			while (i-- > 0) {
				putchar(*p);
				p++;
			}
		} else {
			while (*p) {		/* post pad the string */
				putchar(*p);
				p++;
			}
			i = l - i;
			while (i-- > 0)
				putchar(' ');
		}
		return;
	}

	i = l - (p - plus_ptr);
	p = s;
	while (i-- > 0) {		/* truncate symbol */
		putchar(*p);
		p++;
	}

	p = plus_ptr;			/* output "+offset" untruncated */
	while (*p) {
		putchar(*p);
		p++;
	}
		
}

/*
 *	print a "symbol+offset" string into a buffer of length l,
 *	truncating the symbol as necessary.
 */

NOTSTATIC
prstb_sym(bp, s, l)
	register char **bp;
	char s[];
	int l;
{
	register int i;
	register char *p;
	char *plus_ptr = NULL;

	/*
	 * Search for a "+" in the string, and also for the end of the string.
	 */
	
	p = s;
	while (*p) {
		if (*p == '+')
			plus_ptr = p;
		p++;
	}

	/*
	 * No special truncation is necessary if the string
	 * fits within the field length, or there is no '+offset'
	 * part to the string.
	 */

	i = p - s;
	if (i <= l || plus_ptr == NULL) {		
		p = s;
		if (i >= l) {
			i = l;			/* truncate the string */
			while (i-- > 0)
				*(*bp)++ = *p++;
		} else {
			while (*p)		/* post pad the string */
				*(*bp)++ = *p++;
			i = l - i;
			while (i-- > 0)
				*(*bp)++ = ' ';
		}
		return;
	}

	i = l - (p - plus_ptr);
	p = s;
	while (i-- > 0)			/* output symbol (truncated) */
		*(*bp)++ = *p++;

	p = plus_ptr;			/* output "+offset" */
	while (*p)
		*(*bp)++ = *p++;
}								/* L004 ^ */

#ifndef USER_LEVEL						/* L009 */

/*
 * Initialise the line number table.  The format of the table at db_linetable[]
 * is as follows:
 *
 *	virtual address	1		(int)
 *	line number			(short)
 *	virtual address	2		(int)
 *	line number			(short)
 *	...
 */

db_line_init()							/* L007v */
{
	if (scodb_kstruct_info) {
		struct scodb_usym_info *infop = (struct scodb_usym_info *)
							scodb_kstruct_info;

		if (infop->magic != USYM_MAGIC) {
			printf("Bad magic number from unixsyms info at %x\n",
				scodb_kstruct_info);
			return;
		}

		if (infop->lineno_size) {
			db_lineno = scodb_kstruct_info + infop->lineno_offset;
			db_nlineno = infop->lineno_size / sizeof(struct scodb_lineno);
			if (db_nlineno * sizeof(struct scodb_lineno) != infop->lineno_size) {
				printf("Inconsistent line number size - %d\n", infop->lineno_size);
				db_nlineno = 0;
				return;
			}
		}
	}
}

#endif								/* L009 */

int
db_search_lineno(addr)
unsigned addr;
{
	register int	start, end, middle;

	if (db_nlineno == 0)			/* no line number table */
		return(0);

	start = 0;
	end = db_nlineno - 1;

	if (addr < db_lineno[start].l_addr || addr > db_lineno[end].l_addr)
		return(0);

        middle = (end + start) / 2;

        while (start != middle && end != middle) {
                if (db_lineno[middle].l_addr == addr)
                        return(db_lineno[middle].l_lnno);
                else
                if (db_lineno[middle].l_addr > addr)
                        end = middle;
                else
                        start = middle;
                middle = (end + start) / 2;
        }

        if (db_lineno[start].l_addr == addr)
                return(db_lineno[start].l_lnno);
        else
        if (db_lineno[end].l_addr == addr)
                return(db_lineno[end].l_lnno);

	return(0);
}								/* L007^ */

								/* L009v */
/*
*	get the name of an element, based on
*		md	0: field is a name array
*			1:            value to be passed to a function
*				      which will return name
*			2:	      pointer to the name
*		ep	element pointer
*		off	offset to name field
*		func	the function for (md == 1)
*/
#define	NAM(md, ep, off, func)	(			\
		(md) == 0 ?				\
			((ep) + (off))			\
			:				\
		(					\
		(md) == 1 ?				\
			(*(func))((ep) + (off))		\
			:				\
			*((char **)((ep) + (off)))	\
		)					\
		)
#define	INFNAMEL	(nsz == -1)

/*
*	Array search:
*		nm	find this name in some element
*		ar	array
*		elsz	element size (sizeof array[0])
*		nel	number of elements in array (NMEL(array))
*		offset	offset of "name" in element
*			((int)&el.namefield - (int)&el)
*		func	if (func) find name using func(element+offset)
*			else use name at element+offset
*		nsz	sizeof(el.namefield)
*		rnum	where to return element number matching
*/
NOTSTATIC
ufind(md, nm, ar, elsz, nel, offset, nsz, func, rnum)
	char *nm, *ar;
	int elsz, nel, offset, nsz;
	char *(*func)();
	int *rnum;
{
	int i, j, l, r, strncmp(), strcmp();
	char c, *ep, *epx, bf[NAMEL];
	char *elnm, *elnmx;

	ep = ar;
	for (i = 0;i < nel;i++, ep += elsz) {
		elnm = NAM(md, ep, offset, func);
		if (INFNAMEL) {
			r = strcmp(nm, elnm);
		}else
			r = strncmp(nm, elnm, nsz);
		if (!r) {
			*rnum = i;
			return FOUND_BP;
		}
	}

	l = strlen(nm);
	if ((!INFNAMEL && l > nsz) || !ufind_do_query)		/* L009 */
		return FOUND_NOTFOUND;
	ep = ar;
	for (i = 0;i < nel;i++, ep += elsz) {
		elnm = NAM(md, ep, offset, func);
		if (!strncmp(nm, elnm, l))
			for (;;) {
				elnmx = NAM(md, ep, offset, func);
				if (INFNAMEL)
					strcpy(bf, elnmx);
				else {
					strncpy(bf, elnmx, nsz);
					bf[nsz] = '\0';		/* L000 */
				}
				printf("%s? ", bf);
				c = getrchr();
				cleariline(2 + strlen(elnmx));
				if (k_accept(c)) {
					*rnum = i;
					return FOUND_BP;
				}
				else if (k_backup(c)) {
					/*
					*	find last partial match
					*/
					epx = ep - elsz;
					for (j = i - 1;j >= 0;j--, epx -= elsz) {
						elnmx = NAM(md, epx, offset, func);
						if (!strncmp(nm, elnmx, l))
							break;
					}
					if (j < 0)	/* no previous partial */
						dobell();
					else {
						i = j;
						ep = ar + i * elsz;
					}
				}
				else if (quit(c))
					return FOUND_USERQ;
				else if (!k_next(c))
					dobell();
				else
					break;
			}
	}
	return FOUND_NOTFOUND;
}								/* L009^ */

char *
lsym_getname(Elf32_Sym *sp)
{
	if(lsym_strtab == NULL)
		return NULL;
	return (lsym_strtab + sp->st_name);
}

Elf32_Sym *
lsym_byname(char *name)
{
	Elf32_Sym *sp, *last_sp;

	if(lsym_symtab == NULL)
		return NULL;

	sp = lsym_symtab;
	last_sp = sp + lsym_entries - 1;

	while (sp <= last_sp) {
		if (strcmp(lsym_getname(sp), name) == 0)
			return(sp);
		else
			sp++;
	}
	return(0);
}

Elf32_Sym *
lsym_byvalue(unsigned value)
{
	Elf32_Sym *lower_sp, *upper_sp, *middle_sp;
	int i;

	if(lsym_symtab == NULL)
		return NULL;
	lower_sp = lsym_symtab;
	upper_sp = lower_sp + lsym_entries - 1;

	if (value < lower_sp->st_value)
		return(0);

	if (value >= upper_sp->st_value)
		return(upper_sp);

	while (lower_sp + 1 != upper_sp) {
		i = ((upper_sp - lsym_symtab) + (lower_sp - lsym_symtab)) / 2;
		middle_sp = lsym_symtab + i;
		if (middle_sp->st_value == value)
			return(middle_sp);
		else
		if (value < middle_sp->st_value)
			upper_sp = middle_sp;
		else
			lower_sp = middle_sp;
	}

	/*
	 * The lower pointer will always point to the entry less than or
	 * equal to the value required (except in the case of a value greater
	 * than or equal to the very last entry, which has been taken care of
	 * at the top of the function).
	 */

	return(lower_sp);
}

#ifdef NEVER

Elf32_Sym *
lsym_byvalue(unsigned value)
{
	Elf32_Sym *sp, *last_sp;

	sp = lsym_symtab;
	last_sp = sp + lsym_entries - 1;

	if (value < sp->st_value)
		return(0);

	if (value >= last_sp->st_value)
		return(last_sp);

	while (sp <= last_sp) {
		if (sp->st_value < value)
			sp++;
		else {
			if (sp->st_value > value && sp != lsym_symtab)
				return(sp-1);
			else
				return(sp);
		}
	}
	return(0);
}

#endif
