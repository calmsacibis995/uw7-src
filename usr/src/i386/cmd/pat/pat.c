#ident	"@(#)pat:pat.c	1.2"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <errno.h>
#include "pat.h"

/*
 * Exit status
 */
#define SUCCESS		0	/* report to stdout */
#define ALREADY_PATCHED	1	/* report to stdout */
#define DATA_MISMATCH	2	/* report to stdout */
#define SYMBOL_ERROR	3	/* patsym message to stderr */
#define FILE_ERROR	4	/* strerror message to stderr */
#define BAD_SYNTAX	5	/* syntax + usage message to stderr */

#define ANYDIGIT	'.'	/* wildcard digit */

/*
 * Error messages
 */
static const char Usage[] =
"Usage: pat [-trn] binfile sym[+hexoffset] oldhex [= newhex] [sym2...]\n"
"       oldhex and newhex may be in bytes (e.g. b0 b1 b2 00),\n"
"       in shorts (e.g. b1b0 00b2) or in longs (e.g. 00b2b1b0);\n"
"       with . for a wildcard digit (e.g. in embedded addresses).\n"
"       No = newhex patches are made unless all the oldhex matches.\n"
"       -t pretends to patch, for testing pat scripts without writing;\n"
"       -r reverses direction of patches (oldhex<->newhex), to remove;\n"
"       -n patches kmem not binfile.  0xhex for sym avoids sym lookup,\n"
"       otherwise binfile is ELF or COFF object, archive or executable.\n"
"       Exit 0 for success (oldhex matched, newhex patched); 1 newhex\n"
"       match; 2 mismatch; 3 symbol error; 4 file error; 5 bad syntax.\n";

static const char StdFmt[]	= "UX:pat: ERROR: %s: %s\n";
static const char ParFmt[]	= "UX:pat: ERROR: %s (partly patched): %s\n";
static const char BadOption[]	= "Illegal option";
static const char BadBinfile[]	= "Illegal binfile";
static const char BadSymHex[]	= "Illegal sym[+hexoffset]";
static const char BadOldHex[]	= "Illegal oldhex";
static const char BadNewHex[]	= "Illegal newhex";
static const char Truncated[]	= "I/O truncated";
static const char Disallowed[]	= "Assignment disallowed!";
static       char OldWild[]	= "undefined oldhex";
static const char NoReverse[]	= "Patch is irreversible";
static       char Overlap[]	= "inconsistent overlap";
static const char Impossible[]	= "Match is impossible";
static       char Binding[]	= "binding failed";
static       char Missing[]	= "missing argument";
static       char Malloc[]	= "malloc";
static const char *ErrFmt	=  StdFmt;

static void
badfile(char *file)	/* also used for malloc or binding error */
{
	const char *errmsg;
	if (errno && (errno != EINVAL || file == Binding))
		errmsg = strerror(errno);
	else
		errmsg = Truncated;
	fprintf(stderr, ErrFmt, file, errmsg);
	exit(FILE_ERROR);
}

static void
badsyntax(char *arg, const char *errmsg, int showusage)
{
	fprintf(stderr, ErrFmt, arg? arg: Missing, errmsg);
	if (showusage) fprintf(stderr, Usage);
	exit(BAD_SYNTAX);
}

static int
goodsymoff(char *str, unsigned long *offsetp, unsigned long *plusoffsetp)
{
	char *ptr;

	if (*str == '0') {
		if (str[1] != 'x')	/* demand leading 0x */
			return 0;
		*offsetp = strtoul(str, &ptr, 16);
		if (ptr == str + 2 || ptr > str + 10 || errno == ERANGE) {
			errno = 0;
			return 0;
		}
		str = ptr;
	}
	else {
		if (*str != '_' && !isalpha(*str))
			return 0;
		while (*++str == '_' || isalnum(*str))
			;
	}
	if (*str == '\0')
		return 1;
	if (*str++ != '+')
		return 0;
	if (str[0] == '0'		/* reject leading 0x or 0X */
	&& (str[1] == 'x' || str[1] == 'X'))
		return 0;
	*plusoffsetp = strtoul(str, &ptr, 16);
	if (ptr == str || ptr > str + 8 || *ptr != '\0' || errno == ERANGE) {
		errno = 0;
		return 0;
	}
	*--str = '\0';			/* detach offset from symname */
	return 1;
}

static int
goodhex(char *str, unsigned long *widthp)
{
	char *ptr;

	if (str == NULL)
		return 0;
	ptr = str - 1;
	while (*++ptr) {
		if (*ptr == ANYDIGIT)	/* wildcard nibble */
			continue;
		if (*ptr >= '0' && *ptr <= '9')
			continue;
		if (*ptr >= 'a' && *ptr <= 'f')
			continue;
		/* accept uppercase but lowercase it for matching */
		if (*ptr >= 'A' && *ptr <= 'F') {
			*ptr += 'a' - 'A';
			continue;
		}
		return 0;		/* reject leading 0x or 0X */
	}
	if ((ptr - str) & 1)
		return 0;
	if (*widthp) {			/* reject change of width */
		if (*widthp != (ptr - str) >> 1)
			return 0;
	}
	else {
		*widthp = (ptr - str) >> 1;
		if (*widthp != sizeof(char)
		&&  *widthp != sizeof(short)
		&&  *widthp != sizeof(long))
			return 0;
	}
	return 1;
}

static int
assignment(char *str)
{
	return (str && *str == '=' && *++str == '\0');
}

static int
matchwilds(patch_t *patp)
{
	char **oargp, **nargp;
	char *ostr, *nstr;
	unsigned long length;
	int reversible;

	reversible = 1;
	oargp = patp->oldhex;
	nargp = patp->newhex;

	for (length = 0; length < patp->length; length += patp->width) {
		for (ostr = *oargp++, nstr = *nargp++; *ostr; ostr++, nstr++) {
			if (*ostr == ANYDIGIT && *nstr != ANYDIGIT)
				reversible = 0;
			else	/* eliminate shorthand use of wildcard */
			if (*nstr == ANYDIGIT && *ostr != ANYDIGIT)
				*nstr = *ostr;
		}
	}

	return reversible;
}

static patch_t *
reverse(patch_t *pats)
{
	patch_t *patp, *nextp;
	char **savhex;

	for (patp = pats, pats = NULL; patp; patp = nextp) {
		nextp = patp->next;
		patp->next = pats;
		pats = patp;
		savhex = patp->oldhex;
		patp->oldhex = patp->newhex;
		patp->newhex = savhex;
	}
	return pats;
}

static char **
copyhex(patch_t *patp)
{
	int arg, nargs, slen;
	char **newhex;

	slen = (2 * patp->width) + 1;
	nargs = patp->length / patp->width;
	if ((newhex = malloc(nargs * (sizeof(char *) + slen))) != NULL) {
		for (arg = 0; arg < nargs; arg++) {
			newhex[arg] = (char *)(newhex + nargs) + (arg * slen);
			strcpy(newhex[arg], patp->oldhex[arg]);
		}
	}
	return newhex;
}

static char *
off2hex(patch_t *patp, unsigned long offset, int new)
{
	char **hargp;
	unsigned long width;

	offset -= patp->offset;
	hargp = new? patp->newhex: patp->oldhex;
	if ((width = patp->width) == 1)
		return hargp[offset];
	return hargp[offset / width] + ((~offset % width) << 1);
}

static int
chkoverlaps(patch_t *pats)
{
	patch_t *patp, *patp1, *patp2;
	unsigned long offset, eoffset;
	char *nstr1, *ostr2, *nstr2;
	int backunwild, overlaps;

	overlaps = 0;

	for (patp1 = pats; patp = patp1->next; patp1 = patp1->next) {
		offset = patp1->offset;
		eoffset = offset + patp1->length;
		while (	eoffset <= patp->offset
		||	offset  >= patp->offset + patp->length
		||	patp1->bindcpu != patp->bindcpu)
			if ((patp = patp->next) == NULL)
				break;
		if (patp == NULL)		/* no patp1 overlap */
			continue;
		for (; offset < eoffset; offset++) {
			for (patp2 = patp; patp2; patp2 = patp2->next) {
				if (offset >= patp2->offset
				&&  offset <  patp2->offset + patp2->length
				&&  patp1->bindcpu == patp2->bindcpu)
					break;
			}
			if (patp2 == NULL)	/* no offset overlap */
				continue;
			overlaps++;
			nstr1 = off2hex(patp1, offset, 1);
			ostr2 = off2hex(patp2, offset, 0);
			if (*(short *)ostr2 == *(short *)nstr1)
				continue;	/* no change */
			backunwild = 0;
			nstr2 = off2hex(patp2, offset, 1);
			if (*ostr2 == *nstr1)
				;		/* no change */
			else if (*nstr2 == ANYDIGIT)
				*nstr2 = *ostr2 = *nstr1;
			else if (*ostr2 == ANYDIGIT)
				*ostr2 = *nstr1;
			else if (*nstr1 == ANYDIGIT)
				*nstr1 = *ostr2, backunwild = 1;
			else
				return -1;	/* inconsistent */
			if (*++ostr2 == *++nstr1)
				;		/* no change */
			else if (*++nstr2 == ANYDIGIT)
				*nstr2 = *ostr2 = *nstr1;
			else if (*ostr2 == ANYDIGIT)
				*ostr2 = *nstr1;
			else if (*nstr1 == ANYDIGIT)
				*nstr1 = *ostr2, backunwild |= 2;
			else
				return -1;	/* inconsistent */
			if (!backunwild)
				continue;	/* to next offset */
			/*
			 * adjoverlaps(,,0) will need the now-defined nibbles
			 * propagated back to the first patch which overlaps;
			 * propagate back to all earlier overlaps for clarity
			 */
			--nstr1;
			for (patp2 = pats; nstr2 != nstr1; patp2 = patp2->next){
				if (offset <  patp2->offset
				||  offset >= patp2->offset + patp2->length
				||  patp1->bindcpu != patp2->bindcpu)
					continue;
				ostr2 = off2hex(patp2, offset, 0);
				nstr2 = off2hex(patp2, offset, 1);
				if (backunwild & 1)
					ostr2[0] = nstr2[0] = nstr1[0];
				if (backunwild & 2)
					ostr2[1] = nstr2[1] = nstr1[1];
			}
		}
	}

	return overlaps;
}

static void
adjoverlaps(patch_t *pats, int overlaps, int incrementally)
{
	patch_t *patp, *patp1, *patp2;
	unsigned long offset, eoffset;
	char *hstr1, *ostr2;

	for (patp1 = pats; patp = patp1->next; patp1 = patp1->next) {
		offset = patp1->offset;
		eoffset = offset + patp1->length;
		while (	eoffset <= patp->offset
		||	offset  >= patp->offset + patp->length
		||	patp1->bindcpu != patp->bindcpu)
			if ((patp = patp->next) == NULL)
				break;
		if (patp == NULL)		/* no patp1 overlap */
			continue;
		for (; offset < eoffset; offset++) {
			for (patp2 = patp; patp2; patp2 = patp2->next) {
				if (offset >= patp2->offset
				&&  offset <  patp2->offset + patp2->length
				&&  patp1->bindcpu == patp2->bindcpu)
					break;
			}
			if (patp2 == NULL)	/* no offset overlap */
				continue;
			hstr1 = off2hex(patp1, offset, incrementally);
			ostr2 = off2hex(patp2, offset, 0);
			*(short *)ostr2 = *(short *)hstr1;
			if (--overlaps == 0)
				return;		/* all overlaps done */
		}
	}
}

static int
diffbuf(patch_t *patp, char *bufp)
{
	char *ostr, *nstr, *bstr, *ebufp;
	int width, difference;
	char **oargp, **nargp;
	char sprintbuf[9];
	
	difference = 0;
	ebufp = bufp + patp->length;
	width = patp->width;
	oargp = patp->oldhex;
	nargp = patp->newhex;

	while (bufp < ebufp) {
		sprintf(sprintbuf, "%08x", *(unsigned long *)bufp);
		bstr = (sprintbuf + 8) - width - width;
		for (ostr = *oargp++, nstr = *nargp++;
		     *ostr != '\0'; ostr++, nstr++, bstr++) {
			if (*ostr != *bstr) {
				if (*ostr != ANYDIGIT)
					difference = 1;
				if (*nstr == ANYDIGIT)
					*nstr = *bstr;
				*ostr = *bstr;
			}
		}
		bufp += width;
	}

	return difference;
}

static void
fillbuf(patch_t *patp, char *bufp)
{
	char *ebufp;
	unsigned long value;
	int width;
	char **nargp;

	ebufp = bufp + patp->length;
	width = patp->width;
	nargp = patp->newhex;

	while (bufp < ebufp) {
		value = strtoul(*nargp++, NULL, 16);
		switch (width) {
		case 1:	*(unsigned char  *)bufp = (unsigned char )value; break;
		case 2: *(unsigned short *)bufp = (unsigned short)value; break;
		case 4: *(unsigned long  *)bufp = (unsigned long )value; break;
		}
		bufp += width;
	}
}

static void
display(patch_t *patp, int new)
{
	char **hargp;
	unsigned long length;

	if (new) {
		fputs("         = ", stdout);
		hargp = patp->newhex;
	}
	else {
		printf("0x%08x ", patp->offset);
		hargp = patp->oldhex;
	}

	length = 0;
	while (1) {
		putchar(' ');
		fputs(*hargp++, stdout);
		if ((length += patp->width) >= patp->length) {
			putchar('\n');
			return;
		}
		if ((length & 15) == 0)
			fputs("\n           ", stdout);
	}
}

main(int argc, char *argv[])
{
	int optc, status;
	int updating, testing, reversing;
	int namekmem, bindcpu, overlaps, dfd, rv;
	unsigned long bufsize, pieces;
	char *badname, *binfile, *dstfile, *bufp;
	const char *errmsg;
	patch_t *patp, *pats;

	/*
	 * Check options
	 */
	opterr = namekmem = updating = testing = reversing = 0;
	while ((optc = getopt(argc, argv, "trn")) != EOF) {
		switch (optc) {
		case 't':			/* used to be '=' */
			testing = 1;
			break;
		case 'r':
			reversing = 1;
			break;
		case 'n':
			namekmem = UWKMEM;	/* default to UnixWare */
			break;
		default:
			badsyntax((char *)&optopt, BadOption, 1);
		}
	}
	argv += optind;
	if (!(binfile = *argv++))
		badsyntax(NULL, BadBinfile, 1);
	if (!argv[0])
		badsyntax(NULL, BadSymHex, 1);
	if (!argv[1])
		badsyntax(NULL, BadOldHex, 1);

	/*
	 * Check arguments format and build patch list
	 */
	patp = (patch_t *)&pats;
	bufsize = 0;
	bindcpu = 0;
	errno = 0;
	while (*argv) {
		if ((patp->next = malloc(sizeof(*patp))) == NULL)
			badfile(Malloc);
		patp = patp->next;
		memset(patp, 0, sizeof(*patp));
		if (!goodsymoff(*argv, &patp->offset, &patp->plusoffset))
			badsyntax(*argv, BadSymHex, 0);
		patp->symname = *argv;
		if (!goodhex(*++argv, &patp->width))
			badsyntax(*argv, BadOldHex, 0);
		patp->oldhex = argv;
		pieces = 0;
		while (++pieces && goodhex(*++argv, &patp->width))
			;
		patp->length = patp->width * pieces;
		if (bufsize < patp->length)
			bufsize = patp->length;

		if (namekmem
		&& (strcmp(patp->symname, /* UW */ "myengnum") == 0 ||
		    strcmp(patp->symname, /* O5 */ "processor_index") == 0)
		&&  patp->plusoffset == 0) {
			if (assignment(*argv))
				badsyntax(patp->symname, Disallowed, 0);
			for (bufp = patp->oldhex[0]; *bufp == ANYDIGIT; bufp++)
				;
			if (*bufp) {
				for (bufp = patp->oldhex[0]; *bufp; bufp++)
					if (*bufp == ANYDIGIT)
						*bufp = '0';
				bindcpu = 1 + strtoul(patp->oldhex[0],NULL,16);
				if (bindcpu <= 0) /* avoid binds -1 and -2 */
					bindcpu = 0x80000000;
				if (!pats->bindcpu) {
					patch_t *tmpp = (patch_t *)&pats;
					while ((tmpp = tmpp->next) != patp)
						tmpp->bindcpu = bindcpu;
				}
			}
		}
		patp->bindcpu = bindcpu;

		if (!assignment(*argv)) {
			/*
			 * To distinguish ALREADY_PATCHED from DATA_MISMATCH,
			 * we may need a separate copy of the intended newhex.
			 */
			if ((patp->newhex = copyhex(patp)) == NULL)
				badfile(Malloc);
			continue;
		}

		patp->updating = updating = 1;
		patp->newhex = argv + 1;
		while (goodhex(*++argv, &patp->width) && --pieces)
			;
		if (pieces)
			badsyntax(*argv, BadNewHex, 0);
		if (!matchwilds(patp) && reversing)
			badsyntax(OldWild, NoReverse, 0);
		++argv;
	}
	/* allow a little more because we sprintf longs not chars from bufp */
	if ((bufp = malloc(bufsize + sizeof(unsigned long) - 1)) == NULL)
		badfile(Malloc);

	/*
	 * Open dstfile, and convert symnames to file offsets
	 */
	dstfile = namekmem? "/dev/kmem": binfile;
	if ((dfd = open(dstfile, (updating&&!testing)?O_RDWR:O_RDONLY, 0)) < 0
	||  lseek(dfd, 0, SEEK_END) == -1)
		badfile(dstfile);

	badname = binfile;
	if ((errmsg = patsym(binfile, pats, &badname, namekmem)) != NULL) {
		fprintf(stderr, ErrFmt, badname, errmsg);
		exit((badname == binfile)? FILE_ERROR: SYMBOL_ERROR);
	}

	/*
	 * If reversing, reverse the order of patches as well as
	 * interchanging oldhex and newhex, lest there is overlap.
	 * Check for overlap even if not updating, lest impossible.
	 * If there is overlap, "flatten" oldhex from what would
	 * be found incrementally, to what is expected initially.
	 */
	if (reversing)
		pats = reverse(pats);
	if ((overlaps = chkoverlaps(pats)) < 0)
		badsyntax(Overlap, Impossible, 0);
	if (updating && overlaps)
		adjoverlaps(pats, overlaps, 0);

	/*
	 * Check all given patterns match
	 */
	bindcpu = 0;
	status = SUCCESS;
	for (patp = pats; patp; patp = patp->next) {
		if (patp->bindcpu != bindcpu) {
			bindcpu = patp->bindcpu;
			if (processor_bind_me(bindcpu - 1) < 0)
				badfile(Binding);
		}
		errno = 0;
		if (lseek(dfd, patp->offset, SEEK_SET) == -1)
			badfile(dstfile);
		if (read(dfd, bufp, patp->length) != patp->length)
			badfile(dstfile);
		if (diffbuf(patp, bufp))
			status = DATA_MISMATCH;
	}

	/*
	 * Patch if requested and matching succeeded.
	 * Save and restore original mtime, so that
	 * package can be removed once unpatched.
	 * Rightly or wrongly, remake a null patch and
	 * report SUCCESS, instead of ALREADY_PATCHED.
	 */
	if (updating && status == SUCCESS && !testing) {
		if (!namekmem)
			save_mtime(dfd, dstfile);
		for (patp = pats; patp; patp = patp->next) {
			if (!patp->updating)
				continue;
			fillbuf(patp, bufp);
			if (patp->bindcpu != bindcpu) {
				bindcpu = patp->bindcpu;
				if (processor_bind_me(bindcpu - 1) < 0)
					badfile(Binding);
			}
			errno = 0;
			if (lseek(dfd, patp->offset, SEEK_SET) == -1)
				badfile(dstfile);
			if ((rv = write(dfd, bufp, patp->length)) > 0)
				ErrFmt = ParFmt;
			if (rv != patp->length)
				badfile(dstfile);
		}
		if (!namekmem)
			restore_mtime(dfd, dstfile);
		/* but if file was partially modified, mtime is not reset */
	}

	/*
	 * Leave all displays until after patching, in case
	 * we were patching something volatile in /dev/kmem.
	 * If successful, unflatten the oldhex for display.
	 */
	if (updating && overlaps && status == SUCCESS)
		adjoverlaps(pats, overlaps, 1);
	for (patp = pats; patp; patp = patp->next) {
		display(patp, 0);
		if (patp->updating && status == SUCCESS)
			display(patp, 1);
	}

	/*
	 * If they didn't match, check if already patched.
	 * Must flatten overlaps again, because direction
	 * is now reversed, and newhex becomes oldhex.
	 */
	if (updating && status != SUCCESS) {
		pats = reverse(pats);
		if (overlaps)
			adjoverlaps(pats, overlaps, 0);
		status = ALREADY_PATCHED;
		for (patp = pats; patp; patp = patp->next) {
			fillbuf(patp, bufp);
			if (diffbuf(patp, bufp)) {
				status = DATA_MISMATCH;
				break;
			}
		}
	}

	exit(status);
}
