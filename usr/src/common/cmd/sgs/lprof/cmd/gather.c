#ident	"@(#)lprof:cmd/gather.c	1.3"
/*
* gather - collect full information from cnt file
*/
#include <fcntl.h>
#include <mach_type.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "lprof.h"

struct objfile	objf;	/* current ELF object file */
struct srcfile	*unit;	/* compilation units (.o's) */
struct srcfile	*incl;	/* included source files */

static void
downheap(void **a, unsigned long m,
	int (*f)(const void *, const void *), unsigned long i)
{
	unsigned long c;
	void *t;

	/*
	* Bubble down the value at a[i] as long as it
	* is less than its larger children's value.
	*/
	t = a[i];
	while ((c = i << 1) <= m)
	{
		if (c < m && (*f)(a[c], a[c + 1]) < 0)
			c++;
		if ((*f)(t, a[c]) >= 0)
			break;
		a[i] = a[c];
		i = c;
	}
	a[i] = t;
}

static void
heapsort(void **a, unsigned long m, int (*f)(const void *, const void *))
{
	unsigned long i = (m >> 1) + 1;
	void *t;

	/*
	* Construct the heap in place.
	*/
	while (--i != 0)
		downheap(a, m, f, i);
	/*
	* Keep swapping the largest remaining value (a[1])
	* with the end of the unsorted front piece until
	* the front piece is one item.
	*/
	do {
		t = a[1];
		a[1] = a[m];
		a[m] = t;
		downheap(a, --m, f, 1);
	} while (m > 1);
}

static int
cmpfcn(const void *p1, const void *p2)
{
	const Elf32_Sym *s1 = p1, *s2 = p2;

	if (s1->st_value < s2->st_value)
		return -1;
	if (s1->st_value > s2->st_value)
		return 1;
	return 0;
}

static int
cmpcov(const void *p1, const void *p2)
{
	const caCOVWORD a1 = ((caCOVWORD *)p1)[1]; /* second caCOVWORD... */
	const caCOVWORD a2 = ((caCOVWORD *)p2)[1]; /* ... is the address */

	if (a1 < a2)
		return -1;
	if (a1 > a2)
		return 1;
	return 0;
}

static void
fcnsyms(Elf32_Sym *top, unsigned long nsym, struct caCNTMAP *cmp)
{
	struct rawfcn *rfp, *rfend;
	unsigned long i, n, addr;
	Elf32_Sym *stp, *stend;
	caCOVWORD *cp;
	void **strip;
	int unused;

	/*
	* Walk through the ELF symbol table producing
	* an array of pointers to function symbols.
	* The first element of the array is unused for
	* the convenience of heapsort.  Also note that
	* the first entry in the ELF symbol table is
	* skipped since it is uninteresting.
	*/
	strip = 0;
	i = 0;
	n = 0;
	stend = nsym + top;
	for (stp = top + 1; stp < stend; stp++)
	{
		if (ELF32_ST_TYPE(stp->st_info) != STT_FUNC)
			continue;
		if (stp->st_shndx == SHN_UNDEF)
			continue;
		if (++n >= i) /* need to grow strip */
		{
			i += 200; /* heuristic amount */
			strip = grow(strip, i * sizeof(void *));
		}
		strip[n] = stp;
	}
	strip[0] = 0;
	heapsort(strip, n, cmpfcn);
	/*
	* Create the raw function array and populate its
	* symbol table pointers.
	*/
	objf.nfcn = n;
	objf.fcns = alloc(n * sizeof(struct rawfcn));
	rfp = &objf.fcns[n];
	do {
		rfp--;
		rfp->symp = strip[n];
		rfp->covp = 0;
		rfp->set = 0;
		rfp->nset = 0;
	} while (--n != 0);
	/*
	* Resize strip for use in sorting the coverage data sets.
	* Then make a first pass through the coverage data sets
	* setting pointers for sorting.
	*
	* Each of the coverage data sets looks like this:
	*	caCOVWORD nbb;
	*	caCOVWORD addr;
	*	caCOVWORD cnt[nbb];
	*	caCOVWORD lno[nbb];
	*/
	if ((n = cmp->head->ncov + 1) == 1) /* nevermind */
		goto out;
	strip = grow(strip, n * sizeof(void *));
	strip[0] = 0;
	n = 0;
	cp = cmp->cov;
	while (cp < cmp->aft)
	{
		strip[++n] = cp;
		cp += cp[0] * 2 + 2;
	}
	heapsort(strip, n, cmpcov);
	/*
	* Now run through the two sorted lists, associating the
	* coverage data sets with their functions.
	*/
	rfp = objf.fcns;
	rfend = objf.nfcn + rfp;
	stp = rfp->symp;
	i = 1;
	unused = 0;
	do {
		cp = strip[i];
		addr = cp[1];
		while (addr >= stp->st_value + stp->st_size)
		{
			if (++rfp >= rfend) /* have run out of functions */
			{
				unused = 1;
				goto out;
			}
			stp = rfp->symp;
		}
		if (addr < stp->st_value) /* addr does not match any */
		{
			unused = 1;
			continue;
		}
		/*
		* Add coverage set "cp" to function "rfp".
		*/
		if (rfp->covp == 0)
		{
			rfp->covp = cp;
			rfp->nset = 1;
			continue;
		}
		if (rfp->nset == 1) /* first extra */
			rfp->set = alloc(4 * sizeof(caCOVWORD *));
		else if (rfp->nset % 4 == 1) /* grow it */
		{
			rfp->set = grow(rfp->set,
				(3 + rfp->nset) * sizeof(caCOVWORD *));
		}
		rfp->set[rfp->nset - 1] = cp;
		rfp->nset++;
	} while (++i, --n != 0);
out:;
	if (unused)
		warn(":1715:unused coverage data--not part of any function\n");
	free(strip);
}

static int
objfopen(struct caCNTMAP *cmp)
{
	const char *prog, *strt, *p;
	unsigned long i, nsym, slen;
	Elf_Data *dp, **dpp;
	Elf32_Shdr *shdr;
	Elf32_Sym *symt;
	struct stat st;
	Elf_Scn *scn;
	int bits, fd;

	if ((prog = args.prog) == 0) /* use the recorded pathname */
	{
		prog = cmp->head->name;
		args.prog = strcpy(alloc(strlen(prog) + 1), prog);
	}
	if ((fd = open(prog, O_RDONLY)) < 0)
	{
		error(":1394:can't open file %s\n", prog);
		return -1;
	}
	if (fstat(fd, &st) < 0)
	{
		error(":1684:unable to get status for %s\n", prog);
	err:;
		close(fd);
		return -1;
	}
	if (st.st_mtime != cmp->head->time)
	{
		(*(args.option & OPT_TIMESTAMP ? &warn : &error))(
			":1727:different timestamps for %s and %s\n",
			prog, args.cntf);
		if (!(args.option & OPT_TIMESTAMP))
			goto err; 
	}
	/*
	* Open the elf file, get the header and the string
	* table for the section headers.
	*/
	if ((objf.elfp = elf_begin(fd, ELF_C_READ, 0)) == 0)
	{
		error(":1696:unable to read (begin) file %s\n", prog);
		goto err;
	}
	if ((objf.ehdr = elf32_getehdr(objf.elfp)) == 0)
	{
		error(":1697:unable to get elf header in %s\n", prog);
	elferr:;
		elf_end(objf.elfp);
		goto err;
	}
	if (cmp->head->type != objf.ehdr->e_type 
		|| memcmp(cmp->head->ident, objf.ehdr->e_ident,
			sizeof(objf.ehdr->e_ident)) != 0)
	{
		error(":1730:profile file and elf header mismatch: %s and %s\n",
			args.cntf, prog);
		goto elferr;
	}
	if ((scn = elf_getscn(objf.elfp, objf.ehdr->e_shstrndx)) == 0
		|| (dp = elf_getdata(scn, 0)) == 0)
	{
		error(":1737:unable to get section header strings in %s\n",
			prog);
		goto elferr;
	}
	strt = dp->d_buf;
	slen = dp->d_size;
	/*
	* Search for the interesting sections:
	*   The symbol table (type SHT_SYMTAB) and its string table,
	*   DWARF I  -- .debug, .line
	*   DWARF II -- .debug_abbrev, .debug_info, .debug_line
	*/
	objf.dbg1 = 0;
	objf.lno1 = 0;
	objf.abv2 = 0;
	objf.dbg2 = 0;
	objf.lno2 = 0;
	objf.strt = 0;
	symt = 0;
	scn = 0;
	while ((scn = elf_nextscn(objf.elfp, scn)) != 0)
	{
		if ((shdr = elf32_getshdr(scn)) == 0)
		{
			error(":1695:unable to get section header in %s\n",
				prog);
			goto elferr;
		}
		if (shdr->sh_type == SHT_SYMTAB)
		{
			if ((dp = elf_getdata(scn, 0)) == 0)
			{
				error(":1699:unable to get symbol table of %s\n",
					prog);
				goto elferr;
			}
			symt = (Elf32_Sym *)dp->d_buf;
			nsym = dp->d_size / sizeof(Elf32_Sym);
			if ((dp = elf_getdata(elf_getscn(objf.elfp,
				shdr->sh_link), 0)) == 0)
			{
				error(":1700:unable to get string table of %s\n",
					prog);
				goto elferr;
			}
			objf.strt = dp->d_buf;
			objf.slen = dp->d_size;
			continue;
		}
		if (shdr->sh_type != SHT_PROGBITS)
			continue;
		if ((i = shdr->sh_name) >= slen)
			fatal(":1738:bad section string index in %s\n", prog);
		p = &strt[i];
		if (strcmp(p, ".debug") == 0)
			dpp = &objf.dbg1;
		else if (strcmp(p, ".line") == 0)
			dpp = &objf.lno1;
		else if (strcmp(p, ".debug_abbrev") == 0)
			dpp = &objf.abv2;
		else if (strcmp(p, ".debug_info") == 0)
			dpp = &objf.dbg2;
		else if (strcmp(p, ".debug_line") == 0)
			dpp = &objf.lno2;
		else
			continue;
		if (*dpp != 0)
		{
			error(":1739:too many %s sections in %s\n", p, prog);
			goto elferr;
		}
		if ((dp = elf_getdata(scn, 0)) == 0)
		{
			error(":1740:unable to get section %s in %s\n",
				p, prog);
			goto elferr;
		}
		*dpp = dp;
	}
	/*
	* Verify that we have everything needed for complete processing:
	* A symbol table (with matching string table), and at least a
	* complete DWARF I or DWARF II section set.
	*/
	if (symt == 0)
	{
		error(":1698:cannot find symbol table section in %s\n", prog);
		goto elferr;
	}
	bits = (objf.dbg1 != 0);
	bits |= (objf.lno1 != 0) << 1;
	bits |= (objf.abv2 != 0) << 4;
	bits |= (objf.dbg2 != 0) << 5;
	bits |= (objf.lno2 != 0) << 6;
	if (bits != 0x3 && bits != 0x70 && bits != 0x73)
	{
		error(":1741:incomplete debugging information in %s\n", prog);
		goto elferr;
	}
	/*
	* Select the functions from the symbol table
	* and then associate with those functions their
	* coverage information.
	*/
	fcnsyms(symt, nsym, cmp);
	return 0;
}

void
gather(void)
{
	struct caCNTMAP cm;
	int fd;

	/*
	* Map in the cnt file and close the file descriptor.
	*/
	if ((fd = _CAmapcntf(&cm, args.cntf, O_RDONLY)) < 0)
		exit(2);
	close(fd);
	/*
	* Get the ELF information for the object file.
	* We permit both DWARF I and II to be present.
	*/
	if (objfopen(&cm) != 0)
		exit(2);
	if (objf.lno1 != 0)
		dwarf1();
	if (objf.lno2 != 0)
		dwarf2();
	munmap(cm.head, cm.nbyte); /* unneed from now on */
}

void
applycnts(caCOVWORD *covp, struct linenumb *list, unsigned long nline)
{
	unsigned long i, j, n, tc, tn;
	caCOVWORD *cntp, *nump;
	struct linenumb *lnp;

	/*
	* The coverage data looks like the following:
	*	caCOVWORD nbb;
	*	caCOVWORD addr;
	*	caCOVWORD cnt[nbb];
	*	caCOVWORD lno[nbb];
	* Its order is the same as the incoming debugging
	* line numbers from the object file.
	*/
	if (covp != 0)
	{
		i = covp[0];
		cntp = &covp[2];
		nump = &covp[2 + i];
		/*
		* A special case is made for the initial lines
		* of a function.  Since the first basic block
		* line number for the function might well be a
		* few lines after its true start, we start with
		* the initial execution count.  Note that this
		* can, at times, be misleading at best.  It is
		* possible to construct code such that the first
		* basic block counter for the function is at the
		* top of a loop.  However, the code seems to
		* need to be pretty unusual and the place to
		* correct this problem is in basicblk, not here.
		*/
		n = nline;
		lnp = list;
		tc = *cntp;
		tn = *nump;
		do {
			if (lnp->num == tn && i != 0)
			{
				i--;
				tc = *cntp++;
				tn = *++nump;
			}
			lnp->cnt = tc;
		} while (++lnp, --n != 0);
		if (i != 0)
			warn(":1742:unused coverage data\n");
	}
	/*
	* Now sort the array by increasing line number.  Because
	* it is most likely already in order, use an insertion
	* sort since it quickly terminates for sorted lists.
	*/
	for (i = 1; i < nline; i++)
	{
		tn = list[i].num;
		tc = list[i].cnt;
		for (j = i; j > 0 && tn < (n = list[j - 1].num); j--)
		{
			list[j].num = n;
			list[j].cnt = list[j - 1].cnt;
		}
		list[j].num = tn;
		list[j].cnt = tc;
	}
}

struct rawfcn *
findfunc(unsigned long addr)
{
	unsigned long lo, hi, mid;
	struct rawfcn *rfp;
	Elf32_Sym *stp;

	lo = 0;
	hi = objf.nfcn;
	while (lo < hi)
	{
		mid = (lo + hi) / 2;
		rfp = &objf.fcns[mid];
		stp = rfp->symp;
		if (stp->st_value > addr)
			hi = mid;
		else if (stp->st_value < addr)
			lo = mid + 1;
		else
			return rfp;
	}
	warn(":1743:unable to find function at address %#lx\n", addr);
	return 0;
}

struct function *
addfunc(struct srcfile *sfp, const char *name, struct rawfcn *rfp,
	unsigned long lopc, unsigned long hipc,
	struct linenumb *lnp, unsigned long nline)
{
	struct function *p, *fp, **fpp;

	if (rfp == 0 || rfp->covp != 0)
		sfp->ncov++;
	/*
	* Allocate and populate this function.
	* Apply the cnt file data and sort the resulting lines.
	*/
	fp = alloc(sizeof(struct function));
	fp->name = name;
	fp->srcf = sfp;
	fp->rawp = rfp; /* null for inline expansion code fragment */
	fp->incl = 0; /* handled by caller (dwarf2.c only) */
	fp->line = lnp;
	fp->nlnm = nline;
	fp->slin = lnp->num; /* might be modified by ADJUSTSLIN */
	fp->addr = lopc;
	fp->past = hipc;
	/*
	* Insert it into the line number ordered function list.
	*/
	for (fpp = &sfp->func; (p = *fpp) != 0; fpp = &p->next)
	{
		if (fp->slin < p->slin)
			break;
	}
	fp->next = *fpp;
	*fpp = fp;
	return fp;
}

struct srcfile *
addsrcf(struct srcfile **sfpp, const char *dir, const char *name,
	unsigned long lopc, unsigned long hipc)
{
	struct srcfile *sfp, *p;
	const char *path;
	struct stat st;

	if ((path = search(&st, dir, name)) == 0)
	{
		path = name;
		st.st_ino = 0;
		st.st_dev = 0;
	}
	/*
	* Look for it in the lopc-ordered source file list.
	* Note that all source files on the include list (incl)
	* have the same lopc (0).
	*/
	while ((p = *sfpp) != 0 && lopc >= p->lopc)
	{
		if (st.st_ino == p->inod && st.st_dev == p->flsy)
		{
			if (path != name)
				free((char *)path);
			return p;
		}
		sfpp = &p->next;
	}
	/*
	* New unique file.  Allocate and populate it.
	*/
	sfp = alloc(sizeof(struct srcfile));
	sfp->path = path;
	sfp->func = 0;
	sfp->incl = 0;
	sfp->tail = 0;
	sfp->lopc = lopc;
	sfp->hipc = hipc;
	sfp->inod = st.st_ino;
	sfp->flsy = st.st_dev;
	sfp->ncov = 0;
	sfp->next = *sfpp;
	*sfpp = sfp;
	return sfp;
}
