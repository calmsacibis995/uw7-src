#ident	"@(#)crash:common/cmd/crash/search.c	1.2.2.1"

/*
 * This file contains code for the crash function search.
 */

#include <sys/param.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/immu.h>
#include "crash.h"

/* get arguments for search function */
int
getsearch()
{
	unsigned long mask = 0xffffffff;
	unsigned long pat;
	phaddr_t start, len, limit;
	int phys = 0;
	int proc = Cur_proc;
	int gotproc = 0;
	int gotmask = 0;
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"ps:w:m:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					gotproc++;
					break;
			case 's' :	proc = setproc();
					gotproc++;
					break;
			case 'm' :	mask = strcon(optarg,'h');
					gotmask++;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (gotproc > 1 || gotmask > 1)
		longjmp(syn,0);

	if(args[optind]) {
		if (args[optind][0] == '\"') {
			c = strlen(args[optind]);
			if (c != 6 || args[optind][c-1] != '\"')
				error("string pattern must be 4 characters between quotes\n");
			pat = *(unsigned long *)(1+args[optind]);
			*(unsigned long *)(1+args[optind++]) = 0;
			/* but stdin and redirected fp still have pat */
		}
		else {
			pat = strcon(args[optind++],'h');
			(void)strcon("0x0",'h'); /* erase traces */
		}
	}
	else
		longjmp(syn,0);

	/*
	 * Search is unique in having an argument, pattern, before
	 * the address to which [-p|-sproc] refers; allow for that,
	 * and allow [-mmask] to come after the pattern too, you
	 * tend to want to think about the mask after the pattern.
	 * Allow even -wfilename here, for no better reason than
	 * to avoid the invalid option message if it were entered.
	 */
	while((c = getopt(argcnt,args,"ps:w:m:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					gotproc++;
					break;
			case 's' :	proc = setproc();
					gotproc++;
					break;
			case 'm' :	mask = strcon(optarg,'h');
					gotmask++;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (gotproc > 1 || gotmask > 1) {
		pat = 0;
		longjmp(syn,0);
	}

	limit = (phaddr_t)1 << 32;
	if (phys && pae)
		limit <<= 4;
	if(args[optind])
		start = strcon64(args[optind++],phys);
	else
		start = (phaddr_t)0;
	if(args[optind])
		len = strcon64(args[optind++],phys);
	else
		len = limit;
	if(args[optind]) {
		pat = 0;
		longjmp(syn,0);
	}

	pat &= mask;
	if (start + len > limit)
		len = limit - start;
	len &= ~3;

	/* note: crash32 on OSr5 will never show the correct LENGTH */
	fprintf(fp,"MASK = %x, PATTERN = %x, START = %llx, LENGTH = %llx\n",
		mask, pat, start, len);

	if (len != 0)
		prsearch(mask,&pat,start,len,phys,proc);
	else
		pat = 0;
}

/* print results of search */
int
prsearch(mask,patp,start,len,phys,proc)
register unsigned long mask;
register unsigned long *patp;
phaddr_t start, len;
int phys,proc;
{
	struct {
		unsigned long buf[MMU_PAGESIZE/sizeof(unsigned long)];
		unsigned long carryval;	/* odd bytes to be carried to next */
		unsigned long pat;	/* pat terminating buffer search */
		unsigned char chars[17];/* "od -h"like display of chars */
	}	buf;
	register unsigned long *bufp;
	unsigned long *ebufp, carrymsk;
	size_t extent, limit, extra, carrylen, buffset;
	int i, miscarry, avoidownpat, ownproc;
	phaddr_t addr, paddr, faddr, validlen;
	phaddr_t matchpaddr, ownpatpaddr;
	static char format[] = "%08x:  %08x %08x %08x %08x  %s  phys %08llx\n";
	/*
	 * Only show 8 hex digits for physical address unless PAE enabled;
	 * to avoid column 80, never show the PAE digit at start of line
	 */
	if (pae)
		strcpy(format+36, "%09llx\n");

	/*
	 * Keep just one instance of pat in our address space, so it
	 * doesn't appear repeatedly on an active system.  Avoid this
	 * instance of pat, and wipe out strcon()'s earlier stack use.
	 * Handle the pattern as if it were radioactive!
	 */
	memset(&buf, 0, sizeof(buf));	/* including buf.chars[16] = '\0'; */
	buf.pat = *patp;		/* touch buf.pat before cr_vtop() */
	*patp = 0;
	patp = &buf.pat;

	if (avoidownpat = (active && *patp != 0 && (start & 3) == 0)) {
		ownproc = pid_to_slot(getpid());
		ownpatpaddr = cr_vtop((vaddr_t)patp,ownproc,NULL,B_FALSE);
	}
	else
		ownpatpaddr = -1;

	validlen = 0;
	carrymsk = 0;
	carrylen = 0;
	buffset = 0;
	addr = start;

	/*
	 * Prepare starting conditions for addresses not aligned
	 * on a 16-byte boundary (our display requires preceding
	 * values) and not aligned on a 4-byte boundary (which
	 * requires carry of odd bytes from one page to next).
	 */
	if ((extra = (addr & 15))) {
		if (addr & 3) {
			static unsigned long maskmask[4] =
				{0,0xff,0xffff,0xffffff};
			paddr = 0;
			carrylen = 4 - (addr & 3);
			carrymsk = mask & maskmask[carrylen];
			if (addr < 13) {
				extra = addr;
				buffset = carrylen;
				paddr = -1;
			}
			else if (extra < 13)
				extra += carrylen;
			else
				extra = 0;
		}
		addr -= extra;
		len += extra;
	}

	for (; len != 0; buffset = carrylen, addr += extent, len -= extent) {
		if (miscarry = (carrylen != 0 && paddr == -1))
			buf.carryval = 0;		/* for faddr 0 test */

		extent = (size_t)len;
		if (extent == 0)			/* make it non-zero */
			extent -= MMU_PAGESIZE;
		paddr = phys? addr:
			cr_vtop((vaddr_t)addr, proc, &extent, B_FALSE);
		if (paddr == -1)
			continue;
		faddr = cr_ptof(paddr, &extent);	/* even when active */
		if (faddr == -1) {
			paddr = -1;
			continue;
		}
		if (faddr == 0 && !active
		&&  *patp != 0 && (buf.carryval & carrymsk) != *patp) {
			if (addr >= start)
				validlen += extent;
			else if (extent > 3)
				validlen += addr + extent - start;
			buf.carryval = 0;
			continue;
		}

		extra = 0;
		limit = MMU_PAGESIZE - (paddr & (MMU_PAGESIZE-1));
		if (extent >= limit)
			extent = limit;
		else if ((paddr + extent) & 15)
			extra = 16 - ((paddr + extent) & 15);

		buf.buf[0] = buf.carryval;
		if (faddr == 0 && !active)
			memset(buf.buf + buffset, 0, extent + extra);
		else if (pread64(mem, (char *)buf.buf + buffset,
			extent + extra, (off64_t)faddr) != extent + extra)
			error("search: read error at offset %llx\n", faddr);

		ebufp = (unsigned long *)((char *)buf.buf + (extent & ~3));
		buf.carryval = *ebufp;	/* but it's usually already there */

		if (addr >= start) {
			validlen += extent;
			bufp = buf.buf + miscarry;
		}
		else if (extent > 3) {
			validlen = addr + extent - start;
			bufp = (unsigned long *)((char *)buf.buf
				+ ((start - addr) & ~3));
		}
		else
			continue;

		for (;;) {
			if (mask == 0xffffffff) {
				while (*bufp != *patp)
					++bufp;
			}
			else if (mask) {
				while ((*bufp & mask) != *patp)
					++bufp;
			}
			if (bufp >= ebufp)
				break;
			extra = (char *)bufp - (char *)buf.buf;
			matchpaddr = paddr + extra - buffset;
			if (matchpaddr == ownpatpaddr) {
				avoidownpat++;
				*bufp++ = 0;
				continue;
			}
			/*
			 * You would think we should check for ownpatpaddr
			 * amongst the following longs; but because we keep
			 * wiping out pats from bufp (memset below), and
			 * buf.carryval is zero throughout, no problem.
			 */
			bufp = (unsigned long *)((char *)bufp - (extra & 15));
			memcpy(buf.chars, bufp, 16);
			for (i = 0; i < 16; i++) {
				if (!isprint(buf.chars[i]))
					buf.chars[i] = '.';
			}
			if (miscarry && bufp == buf.buf) {
				fprintf(fp, "%08x:  ", (vaddr_t)addr - buffset);
				for (i = 3; i >= carrylen; i--)
					fprintf(fp, "%02x",
						((unsigned char *)buf.buf)[i]);
				for (; i >= 0; i--) {
					fprintf(fp, "..");
					buf.chars[i] = '.';
				}
				fprintf(fp, format + 11,
					bufp[1], bufp[2], bufp[3],
					buf.chars, matchpaddr);
			}
			else {
				fprintf(fp, format, (vaddr_t)addr - buffset
					+ ((size_t)bufp - (size_t)buf.buf),
					bufp[0], bufp[1], bufp[2], bufp[3],
					buf.chars, matchpaddr);
				/* but a carrylen matchpaddr may be imaginary */
			}
			if (*patp != 0) {
				memset(bufp, 0, 16);
				memset(buf.chars, 0, 16);
			}
			bufp += 4;
		}
	}

	*patp = 0;	/* in case someone else is doing a crash search */
	if (validlen == 0)
		error("no valid addresses within the search range\n");
	if (debugmode > 0)
		fprintf(stderr, "valid length searched = %llx\n", validlen);
	if (avoidownpat == 1) {
		if ((phys && start <= ownpatpaddr
		    && addr - sizeof(*patp) >= ownpatpaddr)
		|| (!phys && proc == ownproc && (vaddr_t)start <= (vaddr_t)patp
		    && (vaddr_t)addr - sizeof(*patp) >= (vaddr_t)patp)) {
			error("search failed to avoid its own pattern: %s\n",
			(cr_vtop((vaddr_t)patp, ownproc, NULL, B_FALSE)
			== ownpatpaddr)? "search error": "buffer was swapped");
			/* unlikely to be swapped unless output pipe filled */
		}
	}
}
