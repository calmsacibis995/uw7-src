#ident	"@(#)lprof:cmd/dwarf2.c	1.4"
/*
* dwarf1.c - process DWARF II information
*/
#include <dwarf2.h>
#include <libdwarf2.h>
#include <stdlib.h>
#include <string.h>
#include "lprof.h"

static const char unexfrm[] = ":1726:unexpected %s form\n";
static const char str_sib[] = "DW_AT_sibling";
static const char str_dcf[] = "DW_AT_decl_file";
static const char str_inl[] = "DW_AT_inline";
static const char str_str[] = "DW_AT_string";
static const char str_xpc[] = "DW_AT_low_pc/DW_AT_high_pc";
static const char str_dir[] = "DW_AT_comp_dir";
static const char str_nam[] = "DW_AT_name";
static const char str_smt[] = "DW_AT_stmt_list";

static const Dwarf2_Abbreviation *abvtab;
static const Dwarf2_Line_entry *lintab;
static const Dwarf2_File_entry *filtab;
static const char **inctab;
static unsigned char *curptr;
static unsigned char *dbgbase;
static unsigned long linlen;
static unsigned long fillen;
static unsigned long inclen;
static unsigned long dbglen;

/*
* These two get functions expect that lprof will be run
* on the same machine as produced the .cnt file.  As such,
* these values are extracted with the "native" byte order.
*/

static unsigned short
get2byte(void)
{
	unsigned short val;
	unsigned char *vp = (unsigned char *)&val;

	vp[0] = curptr[0];
	vp[1] = curptr[1];
	curptr += 2;
	return val;
}

static unsigned long
get4byte(void)
{
	unsigned long val;
	unsigned char *vp = (unsigned char *)&val;

	vp[0] = curptr[0];
	vp[1] = curptr[1];
	vp[2] = curptr[2];
	vp[3] = curptr[3];
	curptr += 4;
	return val;
}

static struct srcfile *
inclsrc(unsigned long file)
{
	const Dwarf2_File_entry *ep;
	const char *dir;

	if (filtab == 0)
	{
		if ((inctab = dwarf2_get_include_table(&inclen)) == 0)
			fatal(":1718:unable to process include table\n");
		if ((filtab = dwarf2_get_file_table(&fillen)) == 0)
			fatal(":1719:unable to process file table\n");
	}
	if (file == 0 || file >= fillen)
		fatal(":1720:bad file number for .debug_line table\n");
	ep = &filtab[file];
	dir = 0;
	if (ep->dir_index != 0)
	{
		if (ep->dir_index >= inclen)
			fatal(":1721:bad include number for .debug_line table\n");
		dir = inctab[ep->dir_index];
	}
	return addsrcf(&incl, dir, ep->name, 0, 0);
}

static void
initlines(unsigned char *line)
{
	unsigned long size;

	if (dwarf2_read_line_header(line, &size) == 0)
		fatal(":1722:unable to process .debug_line header\n");
	if ((lintab = dwarf2_get_line_table(&linlen)) == 0)
		fatal(":1723:unable to process .debug_line\n");
	filtab = 0;
	inctab = 0;
}

static const Dwarf2_Line_entry *
findline(unsigned long addr, unsigned long file)
{
	const Dwarf2_Line_entry *entry;
	unsigned long hi, lo, mid;

	lo = 0;
	hi = linlen;
	while (lo < hi)
	{
		mid = (lo + hi) / 2;
		entry = &lintab[mid];
		if (entry->address > addr)
			hi = mid;
		else if (entry->address < addr)
			lo = mid + 1;
		else
			goto found;
	}
	return 0;
found:;
	/*
	* Locate the first line number entry with this address.
	* Then return the first entry with the matching file index.
	*/
	while (mid != 0 && lintab[--mid].address == addr)
		entry--;
	while (file != 0 && file != entry->file_index)
	{
		if (entry->address != addr || ++mid >= linlen)
			return 0;
		entry++;
	}
	return entry;
}

static struct linenumb *
getlines(const Dwarf2_Line_entry *entry, unsigned long *len, unsigned long past)
{
	const Dwarf2_Line_entry *cur, *end;
	struct linenumb *lnp, *list;
	unsigned long n, file;

	/*
	* Count the number of line numbers for this function.
	* Allocate that many and go back and fill in the numbers.
	*/
	file = entry->file_index;
	n = 0;
	cur = entry;
	end = &lintab[linlen];
	while (cur < end)
	{
		if (past != 0 && cur->address >= past)
			break;
		if (cur->file_index == file)
			n++;
		else if (past == 0)
			break;
		cur++;
	}
	*len = n;
	list = alloc(n * sizeof(struct linenumb));
	lnp = list;
	for (cur = entry;; cur++)
	{
		if (cur->file_index == file)
		{
			lnp->num = cur->line;
			lnp->cnt = 0;
			if (--n == 0)
				break;
			lnp++;
		}
	}
	return list;
}

static void
inclfcn(struct function *fp, struct srcfile *cusfp)
{
	if (fp->slin <= ADJUSTSLIN) /* heuristic to try to include start */
		fp->slin = 1;
	else
		fp->slin -= ADJUSTSLIN;
	if (cusfp->tail == 0)
		cusfp->incl = fp; /* first one */
	else
		cusfp->tail->incl = fp;
	cusfp->tail = fp;
}

static void
inlines(struct function *cfp, struct srcfile *cusfp)
{
	const Dwarf2_Line_entry *entry;
	unsigned long nset, i, j, file;
	caCOVWORD **setp, *cp;
	struct srcfile *sfp;
	struct function *fp;
	struct rawfcn *rfp;
	struct codeseq {
		const Dwarf2_Line_entry	*entry;
		struct linenumb		*lnp;
		caCOVWORD		*cp;
		unsigned long		nline;
		unsigned long		file;
	} *list, *csp;

	/*
	* It is impossible to recognize the start and end of
	* inline expansions based solely on the DWARF II line
	* information:  Nested expansions are not distinguished
	* from sequential inline expansions in the containing
	* function.
	*
	* The same problem exists for basicblk, so we use the
	* same approach which is to take each change of source
	* file as a separate inline expansion.  This wastes
	* some space, but is straightforward.
	*
	* To ameliorate the problem, here we merge all the
	* expansions from the same file, so that we have for
	* each originating function, a single list of line
	* numbers for each file with inserted code.
	*/
	rfp = cfp->rawp;
	nset = rfp->nset - 1;
	setp = rfp->set;
	list = alloc(nset * sizeof(struct codeseq));
	csp = list;
	i = nset;
	/*
	* First pass: find the first line number entry for
	* each coverage set, ignoring those sets that do not
	* have a match (which shouldn't happen).
	*/
	do {
		cp = *setp++;
		if ((csp->entry = findline(cp[1], 0)) == 0)
		{
			error(":1724:no line number matching inline code\n");
			nset--;
			continue;
		}
		csp->file = csp->entry->file_index;
		csp->cp = cp;
		csp++;
	} while (--i != 0);
	/*
	* Insertion sort the list by file number.
	* Hopefully nset is small enough so that this
	* isn't a bottleneck.
	*/
	for (i = 1; i < nset; i++)
	{
		entry = list[i].entry;
		file = list[i].file;
		cp = list[i].cp;
		for (j = i; j > 0 && file < list[j - 1].file; j--)
		{
			list[j].entry = list[j - 1].entry;
			list[j].file = list[j - 1].file;
			list[j].cp = list[j - 1].cp;
		}
		list[j].entry = entry;
		list[j].file = file;
		list[j].cp = cp;
	}
	/*
	* Get the list of line numbers and apply the execution
	* counts to them.  For each set that comes from the same
	* file, merge into the first line number set.
	*/
	file = 0;
	i = 0;
	csp = list;
	do {
		csp->lnp = getlines(csp->entry, &csp->nline, 0);
		applycnts(csp->cp, csp->lnp, csp->nline);
		if (csp->file != file)
		{
			file = csp->file;
			j = csp - list;
			continue;
		}
		mergecnts(&list[j].lnp, &list[j].nline, csp->lnp, csp->nline);
		csp->lnp = 0;
	} while (++csp, ++i != nset);
	/*
	* Run through the list one final time entering the results
	* into the containing source file.  Then free the list.
	*/
	i = 0;
	csp = list;
	do {
		if (csp->lnp == 0)
			continue;
		sfp = inclsrc(csp->file);
		fp = addfunc(sfp, cfp->name, 0, 0, 0, csp->lnp, csp->nline);
		inclfcn(fp, cusfp);
	} while (++csp, ++i != nset);
	free(list);
}

static void
skipat(unsigned long form)
{
	unsigned long size;

again:;
	switch (form)
	{
	default:
		fatal(":1725:unknown DWARF II form (%lu)\n", form);
	case DW_FORM_flag:
	case DW_FORM_ref1:
	case DW_FORM_data1:
		curptr++;
		return;
	case DW_FORM_ref2:
	case DW_FORM_data2:
		curptr += 2;
		return;
	case DW_FORM_addr: /* currently addresses are 4 [see dwarf2()] */
	case DW_FORM_ref_addr:
	case DW_FORM_strp:
	case DW_FORM_ref4:
	case DW_FORM_data4:
		curptr += 4;
		return;
	case DW_FORM_ref8:
	case DW_FORM_data8:
		curptr += 8;
		return;
	case DW_FORM_sdata:
		curptr += dwarf2_decode_signed((long *)&size, curptr);
		return;
	case DW_FORM_ref_udata:
	case DW_FORM_udata:
		curptr += dwarf2_decode_unsigned(&size, curptr);
		return;
	case DW_FORM_string:
		curptr += strlen((char *)curptr) + 1;
		return;
	case DW_FORM_indirect:
		curptr += dwarf2_decode_unsigned(&form, curptr);
		goto again;
	case DW_FORM_block:
		curptr += dwarf2_decode_unsigned(&size, curptr);
		break;
	case DW_FORM_block1:
		size = *curptr++;
		break;
	case DW_FORM_block2:
		size = get2byte();
		break;
	case DW_FORM_block4:
		size = get4byte();
		break;
	}
	curptr += size;
}

static void
routine(struct srcfile *sfp, const Dwarf2_Abbreviation *ap)
{
	unsigned long lopc = 0, hipc = 0, file = 1, inl = ~0ul, loc;
	const Dwarf2_Line_entry *entry;
	const Dwarf2_Attribute *atp;
	unsigned char *sib = 0;
	struct srcfile *cusfp;
	struct linenumb *lnp;
	const char *name = 0;
	struct function *fp;
	struct rawfcn *rfp;
	int nat, decl = 0;

	for (nat = ap->nattr, atp = ap->attributes; nat != 0; --nat, ++atp)
	{
		switch (atp->name)
		{
		case DW_AT_sibling:
			if (atp->form != DW_FORM_ref4)
				fatal(unexfrm, str_sib);
			loc = get4byte();
			sib = loc + dbgbase;
			if (loc > dbglen || sib <= curptr)
				fatal(":1710:bad %s for routine\n", str_sib);
			break;
		case DW_AT_decl_file:
			if (atp->form != DW_FORM_udata)
				fatal(unexfrm, str_dcf);
			curptr += dwarf2_decode_unsigned(&file, curptr);
			if (file == 0)
				file = 1;
			break;
		case DW_AT_inline:
			if (atp->form != DW_FORM_data1)
				fatal(unexfrm, str_inl);
			inl = *curptr++;
			break;
		case DW_AT_low_pc: /* DW_FORM_addr */
			lopc = get4byte();
			break;
		case DW_AT_high_pc: /* DW_FORM_addr */
			hipc = get4byte();
			break;
		case DW_AT_declaration: /* DW_FORM_flag */
			decl = *curptr++;
			break;
		case DW_AT_name:
			if (atp->form != DW_FORM_string)
				fatal(unexfrm, str_str);
			name = (char *)curptr;
			/*FALLTHROUGH*/
		default:
			skipat(atp->form);
			break;
		}
	}
	if (sib == 0)
		fatal(":1711:missing %s for routine\n", str_sib);
	if (decl || inl == DW_INL_inlined || inl == DW_INL_declared_inlined)
		goto out; /* nevermind--covered elsewhere */
	if (name == 0 || *name == '\0')
		goto out; /* unnamed routine? */
	if (lopc == 0 || hipc == 0)
		fatal(":1711:missing %s for routine\n", str_xpc);
	/*
	* Get line number information matching this function.
	* If the function comes from a different source file,
	* insert it in the included files list and add it to
	* the side list of included functions.
	*/
	if ((entry = findline(lopc, file)) == 0)
	{
		error(":1712:no line number matching function %s\n", name);
		goto out;
	}
	if ((rfp = findfunc(lopc)) == 0)
		goto out;
	if ((loc = rfp->symp->st_name) >= objf.slen)
		fatal(":1713:bad string index for function %s\n", name);
	name = &objf.strt[loc];
	lnp = getlines(entry, &loc, hipc);
	applycnts(rfp->covp, lnp, loc);
	cusfp = sfp;
	if (file != 1) /* function comes from a different source file */
		sfp = inclsrc(file);
	fp = addfunc(sfp, name, rfp, lopc, hipc, lnp, loc);
	if (file != 1) /* put it on the included functions list */
		inclfcn(fp, cusfp);
	if (rfp->nset > 1)
		inlines(fp, cusfp);
out:;
	curptr = sib;
}

static void
skiptonext(const Dwarf2_Abbreviation *ap)
{
	const Dwarf2_Attribute *atp;
	unsigned long loc;
	unsigned char *p;
	int nat;

	for (nat = ap->nattr, atp = ap->attributes; nat != 0; --nat, ++atp)
	{
		if (atp->name == DW_AT_sibling)
		{
			if (atp->form != DW_FORM_ref4)
				fatal(unexfrm, str_sib);
			loc = get4byte();
			p = loc + dbgbase;
			if (loc > dbglen || p <= curptr)
				fatal(":1714:bad %s for compile unit\n", str_sib);
			curptr = p;
			return;
		}
		skipat(atp->form);
	}
	if (ap->children)
		fatal(":1728:missing DW_AT_sibling for compile unit entry\n");
}

static void
compunit(unsigned char *unitend)
{
	unsigned char *dir = 0, *name = 0, *line = 0;
	unsigned long lopc = 0, hipc = 0;
	const Dwarf2_Abbreviation *ap;
	const Dwarf2_Attribute *atp;
	unsigned long abv, loff;
	struct srcfile *sfp;
	int nat;

	curptr += dwarf2_decode_unsigned(&abv, curptr);
	ap = &abvtab[abv];
	if (ap->tag != DW_TAG_compile_unit)
		fatal(":1729:expecting DW_TAG_compile_unit\n");
	for (nat = ap->nattr, atp = ap->attributes; nat != 0; --nat, ++atp)
	{
		switch (atp->name)
		{
		case DW_AT_comp_dir:
			if (atp->form != DW_FORM_string)
				fatal(unexfrm, str_dir);
			dir = (unsigned char *)strchr((char *)curptr, ':');
			if (dir == 0)
				dir = curptr;
			else
				dir++;
			break;
		case DW_AT_name:
			if (atp->form != DW_FORM_string)
				fatal(unexfrm, str_nam);
			name = curptr;
			break;
		case DW_AT_low_pc: /* DW_FORM_addr */
			lopc = get4byte();
			continue;
		case DW_AT_high_pc: /* DW_FORM_addr */
			hipc = get4byte();
			continue;
		case DW_AT_stmt_list:
			if (atp->form != DW_FORM_data4)
				fatal(unexfrm, str_smt);
			loff = get4byte();
			if (loff >= objf.lno2->d_size)
				fatal(":1714:bad %s for compile unit\n", str_smt);
			line = loff + (unsigned char *)objf.lno2->d_buf;
			continue;
		}
		skipat(atp->form);
	}
	if (lopc == 0 || hipc == 0 || line == 0)
		goto out;
	if (name == 0 || *name == '\0')
		fatal(":1716:missing %s for compile unit\n", str_nam);
	/*
	* Add this source file to the compilation unit list.
	*/
	sfp = addsrcf(&unit, (char *)dir, (char *)name, lopc, hipc);
	/*
	* Set up information about this unit's line numbers.
	* Handle each function from this compile unit.
	*/
	initlines(line);
	for (;;)
	{
		if (curptr >= unitend)
			fatal(":1731:walked too far in .debug_info\n");
		curptr += dwarf2_decode_unsigned(&abv, curptr);
		if (abv == 0) /* null entry marks list end */
			break;
		ap = &abvtab[abv];
		if (ap->tag == DW_TAG_subprogram)
			routine(sfp, ap);
		else
			skiptonext(ap);
	}
	dwarf2_reset_line_table(0);
out:;
	curptr = unitend;
}

void
dwarf2(void)
{
	unsigned char *abvstart, *endptr, *unitend;
	size_t curabvloc, abvloc, abvlen;

	abvstart = objf.abv2->d_buf;
	abvlen = objf.abv2->d_size;
	curabvloc = ~0ul;
	curptr = objf.dbg2->d_buf;
	endptr = curptr + objf.dbg2->d_size;
	/*
	* For each compile unit in this object file.
	*/
	while (curptr < endptr)
	{
		/*
		* At the start of a compile unit.
		*/
		if (endptr - curptr < 4 + 2 + 4 + 1 + 1)
			fatal(":1732:insufficient .debug_info contents\n");
		dbgbase = curptr;
		dbglen = get4byte();
		if (endptr - curptr < dbglen)
			fatal(":1733:compile unit too big for .debug_info\n");
		unitend = curptr + dbglen;
		dbglen += 4;
		if (get2byte() != 2)
			fatal(":1734:unknown .debug_info version\n");
		abvloc = get4byte();
		if (*curptr++ != 4)
			fatal(":1735:invalid .debug_info address size\n");
		if (curabvloc != abvloc)
		{
			if (abvtab != 0)
			{
				dwarf2_delete_abbreviation_table(
					(Dwarf2_Abbreviation *)abvtab);
			}
			if ((abvtab = dwarf2_get_abbreviation_table(
				abvstart + abvloc, abvlen - abvloc, 0)) == 0)
			{
				fatal(":1736:unable to process .debug_abbrev\n");
			}
			curabvloc = abvloc;
		}
		compunit(unitend);
	}
	dwarf2_reset_line_table(1);
}
