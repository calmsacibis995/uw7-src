#ident	"@(#)lprof:cmd/dwarf1.c	1.2"
/*
* dwarf1.c - process DWARF I information
*/
#include <dwarf.h>
#include <string.h>
#include "lprof.h"

static unsigned char *curptr;
static unsigned char *linptr;
static unsigned char *linend;
static unsigned long linbase;
static unsigned long linsize;

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
get4val(unsigned char *ptr)
{
	unsigned long val;
	unsigned char *vp = (unsigned char *)&val;

	vp[0] = ptr[0];
	vp[1] = ptr[1];
	vp[2] = ptr[2];
	vp[3] = ptr[3];
	return val;
}

static unsigned long
get4byte(void)
{
	unsigned long val;
	
	val = get4val(curptr);
	curptr += 4;
	return val;
}

static void
initlines(unsigned char *line)
{
	unsigned long addr, size;
	unsigned char *end;

	end = objf.lno1->d_size + (unsigned char *)objf.lno1->d_buf;
	if ((linptr = line + 8) > end)
		fatal(":1705:unable to get .line header\n");
	size = get4val(line);
	linbase = get4val(line + 4);
	if ((linend = line + size) > end)
		fatal(":1706:insufficient .line information\n");
	size -= 8;
	linsize = size / (4 + 2 + 4);
	if (linsize * (4 + 2 + 4) != size)
		fatal(":1707:incomplete .line records\n");
	/*
	* Although in practice all the line number records are
	* in address order, there is no requirement that they
	* be such.  Since we must keep these records in the
	* current order (to match the coverage information),
	* we run through the entries now to verify that they
	* are sorted.
	*/
	line += 8; /* to the first record */
	addr = 0;
	while (line < end)
	{
		size = get4val(line + 4 + 2);
		if (addr > size)
			fatal(":1708:misordered .line records\n");
		addr = size;
		line += 4 + 2 + 4;
	}
}

static unsigned char *
findline(unsigned long addr)
{
	unsigned long lo, hi, mid, pc;
	unsigned char *entry;

	if (addr < linbase)
		return 0;
	addr -= linbase;
	lo = 0;
	hi = linsize;
	while (lo < hi)
	{
		mid = (lo + hi) / 2;
		entry = linptr + mid * (4 + 2 + 4);
		pc = get4val(entry + 4 + 2);
		if (pc > addr)
			hi = mid;
		else if (pc < addr)
			lo = mid + 1;
		else
			goto found;
	}
	return 0;
found:;
	/*
	* Locate the first line number record with this address.
	*/
	while (entry > linptr)
	{
		if (get4val(entry - 4) != addr)
			break;
		entry -= 4 + 2 + 4;
	}
	return entry;
}

static struct linenumb *
getlines(unsigned char *entry, unsigned long *len, unsigned long past)
{
	struct linenumb *lnp, *list;
	unsigned long n, addr;
	unsigned char *ptr;

	/*
	* Count the number of line numbers for this function.
	* Start from one since we always include the record that
	* entry points to.  Allocate that many and go back and
	* fill in the numbers.
	*/
	ptr = entry;
	n = 1;
	past -= linbase;
	while ((ptr += 4 + 2 + 4) < linend && get4val(ptr + 4 + 2) < past)
		n++;
	*len = n;
	list = alloc(n * sizeof(struct linenumb));
	ptr = entry;
	lnp = list;
	do {
		lnp->num = get4val(ptr);
		lnp->cnt = 0;
	} while (ptr += 4 + 2 + 4, ++lnp, --n != 0);
	return list;
}

static void
skipat(unsigned long at)
{
	switch (at & FORM_MASK)
	{
	default:
		fatal(":1709:unknown DWARF I form (%#x)\n",
			(unsigned int)(at & FORM_MASK));
	case FORM_DATA2:
		curptr += 2;
		return;
	case FORM_ADDR: /* presumes that addresses are 4 bytes */
	case FORM_REF:
	case FORM_DATA4:
		curptr += 4;
		return;
	case FORM_DATA8:
		curptr += 8;
		return;
	case FORM_BLOCK2:
		curptr += get2byte();
		return;
	case FORM_BLOCK4:
		curptr += get4byte();
		return;
	case FORM_STRING:
		curptr += strlen((void *)curptr) + 1;
		return;
	}
}

static void
routine(unsigned long len, struct srcfile *sfp)
{
	unsigned long lopc = 0, hipc = 0, sib = 0, at, nline;
	unsigned char *end, *line;
	struct linenumb *lnp;
	const char *name = 0;
	struct rawfcn *rfp;

	end = curptr + len;
	while (curptr < end) /* gather immediate debug info */
	{
		switch (at = get2byte())
		{
		case AT_sibling:
			sib = get4byte();
			if (sib < curptr - (unsigned char *)objf.dbg1->d_buf
				|| sib > objf.dbg1->d_size)
			{
				fatal(":1710:bad %s for routine\n",
					(const char *)"AT_sibling");
			}
			break;
		case AT_low_pc:
			lopc = get4byte();
			break;
		case AT_high_pc:
			hipc = get4byte();
			break;
		case AT_name:
			name = (char *)curptr;
			/*FALLTHROUGH*/
		default:
			skipat(at);
			break;
		}
	}
	if (lopc == 0 || hipc == 0 || sib == 0 || name == 0)
		fatal(":1711:missing %s for routine\n", (const char *)"AT_*");
	end = sib + (unsigned char *)objf.dbg1->d_buf;
	/*
	* Get line number information matching this function.
	* Then add it to this source file's list.
	*/
	if ((line = findline(lopc)) == 0)
	{
		error(":1712:no line number matching function %s\n", name);
		goto out;
	}
	if ((rfp = findfunc(lopc)) != 0)
	{
		lnp = getlines(line, &nline, hipc);
		applycnts(rfp->covp, lnp, nline);
		if ((at = rfp->symp->st_name) >= objf.slen)
			fatal(":1713:bad string index for function %s\n", name);
		name = &objf.strt[at];
		(void)addfunc(sfp, name, rfp, lopc, hipc, lnp, nline);
	}
out:;
	curptr = end;
}

static void
compunit(unsigned long len)
{
	unsigned long lopc = 0, hipc = 0, sib = 0, loff, at, tag;
	unsigned char *line = 0, *end;
	const char *name = 0;
	struct srcfile *sfp;

	end = curptr + len;
	while (curptr < end) /* gather immediate debug info */
	{
		switch (at = get2byte())
		{
		case AT_sibling:
			sib = get4byte();
			if (sib < curptr - (unsigned char *)objf.dbg1->d_buf
				|| sib > objf.dbg1->d_size)
			{
				fatal(":1714:bad %s for compile unit\n",
					(const char *)"AT_sibling");
			}
			break;
		case AT_low_pc:
			lopc = get4byte();
			break;
		case AT_high_pc:
			hipc = get4byte();
			break;
		case AT_stmt_list:
			loff = get4byte();
			if (loff >= objf.lno1->d_size)
			{
				fatal(":1714:bad %s for compile unit\n",
					(const char *)"AT_stmt_list");
			}
			line = loff + (unsigned char *)objf.lno1->d_buf;
			break;
		case AT_name:
			name = (char *)curptr;
			/*FALLTHROUGH*/
		default:
			skipat(at);
			break;
		}
	}
	if (sib == 0 || name == 0)
	{
		fatal(":1716:missing %s for compile unit\n",
			(const char *)"AT_sibling/AT_name");
	}
	end = sib + (unsigned char *)objf.dbg1->d_buf;
	if (lopc == 0 || hipc == 0 || line == 0)
	{
		curptr = end; /* just skip it--no functions here */
		return;
	}
	/*
	* Add this source file to the compilation unit list.
	*/
	sfp = addsrcf(&unit, 0, name, lopc, hipc);
	/*
	* Set up information about this unit's line numbers.
	* Handle each function from this compile unit.
	*/
	initlines(line);
	while (curptr < end)
	{
		if ((len = get4byte()) < 4)
			fatal(":1717:.debug entry too short\n");
		len -= 4;
		if (len >= 4) /* 7 or fewer bytes is padding */
		{
			len -= 2;
			if ((tag = get2byte()) == TAG_subroutine
				|| tag == TAG_global_subroutine)
			{
				routine(len, sfp);
				continue;
			}
		}
		curptr += len;
	}
}

void
dwarf1(void)
{
	unsigned char *end;
	unsigned long len;

	curptr = objf.dbg1->d_buf;
	end = curptr + objf.dbg1->d_size;
	/*
	* Handle each compile unit in this object file.
	*/
	while (curptr < end)
	{
		if ((len = get4byte()) < 4)
			fatal(":1717:.debug entry too short\n");
		len -= 4;
		if (len >= 4) /* 7 or fewer bytes is padding */
		{
			len -= 2;
			if (get2byte() == TAG_compile_unit)
			{
				compunit(len);
				continue;
			}
		}
		curptr += len;
	}
}
