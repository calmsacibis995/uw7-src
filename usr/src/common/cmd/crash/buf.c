#ident	"@(#)crash:common/cmd/crash/buf.c	1.1.3.1"

/*
 * This file contains code for the crash functions: bufhdr, buffer, od.
 */

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/vnode.h>
#include <sys/fs/s5param.h>
#include <sys/fs/s5inode.h>
#include <sys/user.h>
#include <sys/var.h>
#include <sys/proc.h>
#include <sys/fs/s5ino.h>
#include <sys/buf.h>
#include <sys/vmparam.h>
#include "crash.h"

#define BSZ  1		/* byte size */
#define SSZ  2		/* short size */
#define LSZ  4		/* long size */

static struct syment *Hbuf;	/* namelist symbol pointer */
static struct syment *Nbuf;	/* namelist symbol pointer */
static struct syment *Pageio;

static char bformat = 'x';	/* buffer format */
static int  type = LSZ;		/* od type */
static char hntype = '\0';	/* od h or n subtype */
static char mode = 'x';		/* od mode */

/*
 * The results of parsefieldspec(), kept for next od call
 */
#define MAXDEPTH 16
static int ptrflds[MAXDEPTH];
static int depth, forwfld, strfld, firstfld, lastfld;
static struct offstable *tp, *tpbase;

static void prod(phaddr_t, phaddr_t, int, int, int);

int
getbufhdr()
{
	int full = 0;
	long addr = -1;
	int nbuf;
	struct hbuf hbuf;
	int c;
	int i;
	char *heading = "BUFHDR    MAJ/MIN   BLOCK      BUFFER  BUFSIZE\n\tFOR      BCK      AVF      AVB      FLAGS\n";

	optind = 1;
	while((c = getopt(argcnt,args,"fw:")) !=EOF) {
		switch(c) {
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	if (Nbuf || (Nbuf = symsrch("nbuf"))) {
		readmem(Nbuf->n_value,1,-1,&nbuf,sizeof(nbuf),"nbuf");
		fprintf(fp,"NUMBER OF bio BUFFER HEADERS ALLOCATED = %d\n",nbuf);
	}

	if(!full)
		fprintf(fp,"%s",heading);
	if(args[optind])
		do {
			addr = strcon(args[optind],'h');
			(void)prbufhdr(full,addr,heading);
		}while(args[++optind]);
	else {
		if(!Hbuf)
			if(!(Hbuf = symsrch("hbuf")))
				error("hbuf not found in symbol table\n");

		for(i = 0; i < vbuf.v_hbuf; i++ ) {
			long hbuf_addr = Hbuf->n_value + i * sizeof(hbuf);
			readmem(hbuf_addr,1,-1,&hbuf,sizeof(hbuf),"hbuf");
			for(addr = (long)hbuf.hb_forw; addr != hbuf_addr; )
				addr = (long)prbufhdr(full,addr,heading);
		}

		if(!Pageio)
			if(!(Pageio = symsrch("pageio_out")))
				return;

		fprintf(fp,"pageio_out BUFFERS\n");
		readmem(Pageio->n_value,1,-1,&hbuf,sizeof(hbuf),"pageio_out");
		for(addr = (long)hbuf.hb_forw; addr != Pageio->n_value; )
			addr = (long)prbufhdr(full,addr,heading);
	}
}


/* print buffer headers */
int
prbufhdr(full,addr,heading)
int full;
long addr;
char *heading;
{
	register int b_flags;
	struct buf bhbuf;
	int procslot;

	readmem(addr,1,-1,&bhbuf,sizeof bhbuf,"buffer header");
	if(full)
		fprintf(fp,"%s",heading);
	fprintf(fp,"%8x",addr);
	fprintf(fp," %4u,%-5u %-8x %8x %8x\n",
			getemajor(bhbuf.b_edev)&L_MAXMAJ,
			geteminor(bhbuf.b_edev),
			bhbuf.b_blkno,
			bhbuf.b_un.b_addr,
			bhbuf.b_bufsize);
	fprintf(fp,"\t%8x",bhbuf.b_forw);
	fprintf(fp," %8x",bhbuf.b_back);
	if(!(bhbuf.b_flags & B_BUSY)) {
		fprintf(fp," %8x",bhbuf.av_forw);
		fprintf(fp," %8x",bhbuf.av_back);
	}
	else fprintf(fp," -         -       ");
	b_flags = bhbuf.b_flags;
	fprintf(fp,"%s%s%s%s%s%s%s%s%s%s%s\n",
		b_flags == B_WRITE ? " write" : "",
		b_flags & B_READ ? " read" : "",
		b_flags & B_DONE ? " done" : "",
		b_flags & B_ERROR ? " error" : "",
		b_flags & B_BUSY ? " busy" : "",
		b_flags & B_PHYS ? " phys" : "",
		b_flags & B_WANTED ? " wanted" : "",
		b_flags & B_AGE ? " age" : "",
		b_flags & B_ASYNC ? " async" : "",
		b_flags & B_DELWRI ? " delwri" : "",
		b_flags & B_STALE ? " stale" : "");
	if(full) {
		fprintf(fp,"\tBCNT ERR RESI  START  AVAIL PROC  RELTIME\n");
		fprintf(fp,"\t%4d %3d %4d %8x %4x",
			bhbuf.b_bcount,
			bhbuf.b_error,
			bhbuf.b_resid,
			bhbuf.b_start,
			bhbuf.b_avail.sl_avail);
		procslot = proc_to_slot(bhbuf.b_proc);
		if (procslot == -1)
			fprintf(fp,"   - ");
		else
			fprintf(fp," %4d",procslot);
		fprintf(fp," %8x",bhbuf.b_reltime);
		fprintf(fp,"\n");
	}
	return (int)bhbuf.b_forw;
}

/* get arguments for buffer function */
int
getbuffer()
{
	int fflag = 0;
	vaddr_t addr;
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"bcdxiow:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'b' :	bformat = 'b';
					fflag++;
					break;
			case 'c' :	bformat = 'c';
					fflag++;
					break;
			case 'd' :	bformat = 'd';
					fflag++;
					break;
			case 'x' :	bformat = 'x';
					fflag++;
					break;
			case 'i' :	bformat = 'i';
					fflag++;
					break;
			case 'o' :	bformat = 'o';
					fflag++;
					break;
			default  :	longjmp(syn,0);
					break;
		}
	}
	if(fflag > 1)
		longjmp(syn,0);
	if(!args[optind] || args[optind+1])
		longjmp(syn,0);
	addr = strcon(args[optind],'h');
	prbuffer(addr);
}


/* print buffer */
int
prbuffer(addr)
vaddr_t addr;
{
	struct buf bhbuf;
	char buffer[MAXBSIZE];
	int inum;

	readmem(addr,1,-1,&bhbuf,sizeof bhbuf,"buffer header");
	if ((bhbuf.b_flags & (B_PAGEIO|B_PHYS))
	||  (vaddr_t)bhbuf.b_un.b_addr < kvbase)
		error("b_un.b_addr %08x is not a buffer address\n",
			bhbuf.b_un.b_addr);
	if (bhbuf.b_bufsize < 0 || bhbuf.b_bufsize > MAXBSIZE)
		error("b_bufsize %d is out of range 0-%d\n",
			bhbuf.b_bufsize, MAXBSIZE);
	readmem((long)bhbuf.b_un.b_addr,1,-1,(char *)buffer,
		bhbuf.b_bufsize,"buffer");
	fprintf(fp,"BUFHDR %08x BUFFER %08x SIZE %d:\n",
		addr, bhbuf.b_un.b_addr, bhbuf.b_bufsize);
	switch(bformat) {
		case 'x' :
		case 'd' :
		case 'o' :	prblong((long *)buffer, bhbuf.b_bufsize);
				break;
		case 'b' :
		case 'c' :	prbchar(buffer, bhbuf.b_bufsize);
				break;

		case 'i' :	inum = bhbuf.b_blkno * NBPSCTR;
				inum -= 2 * bhbuf.b_bufsize;
				inum /= sizeof(struct dinode);
				inum++;
				prbinode((struct dinode *)buffer,
					bhbuf.b_bufsize, inum);
				break;
	}
}

/* print buffer in numerical format */
int
prblong(ip, size)
long *ip;
int size;
{
	char *format;
	int i;

	switch(bformat) {
	case 'd':	format = " %10.10u";	break;
	case 'o':	format = " %11.11o";	break;
	default:	format = " %8.8x";	break;
	}

	for(i = 0; i < size; i += sizeof(long), ip++) {
		if((i % 16) == 0)
			fprintf(fp,"\n%5.5x:\t", i);
		raw_fprintf(fp,format,*ip);
		/* show decimal or octal even in hexmode */
	}
	fprintf(fp,"\n");
}


/* print buffer in character format */
int
prbchar(cp, size)
char *cp;
int size;
{
	int i;

	for(i = 0; i < size; i++, cp++) {
		if((i % 16) == 0)
			fprintf(fp,"\n%5.5x:\t ", i);
		if(bformat == 'c') putch(*cp);
		else fprintf(fp,"%2.2x ", *cp & 0xff);
	}
	fprintf(fp,"\n");
}


/* print buffer in inode format */
int
prbinode(dip, size, inum)
struct dinode *dip;
int size, inum;
{
	long	_3to4();
	int	i,j;

	for(i = 0; i < size; i += sizeof(struct dinode), dip++, inum++) {
		fprintf(fp,"\ni#: %ld  md: ", inum);
		switch(dip->di_mode & IFMT) {
		case IFCHR: fprintf(fp,"c"); break;
		case IFBLK: fprintf(fp,"b"); break;
		case IFDIR: fprintf(fp,"d"); break;
		case IFREG: fprintf(fp,"f"); break;
		case IFIFO: fprintf(fp,"p"); break;
		default:    fprintf(fp,"-"); break;
		}
		fprintf(fp,"\n%s%s%s%3x",
			dip->di_mode & ISUID ? "u" : "-",
			dip->di_mode & ISGID ? "g" : "-",
			dip->di_mode & ISVTX ? "t" : "-",
			dip->di_mode & 0777);
		fprintf(fp,"  ln: %u  uid: %u  gid: %u  sz: %ld",
			dip->di_nlink, dip->di_uid,
			dip->di_gid, dip->di_size);
		if((dip->di_mode & IFMT) == IFCHR ||
			(dip->di_mode & IFMT) == IFBLK ||
			(dip->di_mode & IFMT) == IFIFO)
			fprintf(fp,"\nmaj: %d  min: %1.1o\n",
				dip->di_addr[0] & 0377,
				dip->di_addr[1] & 0377);
		else
			for(j = 0; j < NADDR; j++) {
				if(j % 7 == 0)
					fprintf(fp,"\n");
				fprintf(fp,"a%d: %ld  ", j,
					_3to4(&dip->di_addr[3 * j]));
			}

		fprintf(fp,"\nat: %s\n", date_time(dip->di_atime));
		fprintf(fp,"mt: %s\n", date_time(dip->di_mtime));
		fprintf(fp,"ct: %s\n", date_time(dip->di_ctime));
	}
	fprintf(fp,"\n");
}


/* convert 3 byte disk block address to 4 byte address */
long
_3to4(ptr)
register  char  *ptr;
{
	long retval;
	register  char  *vptr;

	vptr = (char *)&retval;
	*vptr++ = 0;
	*vptr++ = *ptr++;
	*vptr++ = *ptr++;
	*vptr++ = *ptr++;
	return(retval);
}


/* get arguments for od function */
int
getod()
{
	int c;
	int phys = 0;
	unsigned long count = 1;
	int proc = Cur_proc;
	int struflag = 0;
	int modeflag = 0;
	int typeflag = 0;
	int procflag = 0;
	char *fieldspec;
	phaddr_t origaddr, headaddr, tailaddr;
	unsigned long slot, done;
	static int strulast = 0;

	optind = 1;
	while((c = getopt(argcnt,args,"abcdf:hlnops:tw:x")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					procflag++;
					break;
			case 's' :	proc = setproc();
					procflag++;
					break;

			case 'f' :	fieldspec = optarg;
					struflag++;
					strulast = 0;
					if (!modeflag) {
						mode = 'x';
						if (!typeflag)
							type = LSZ;
					}
					break;

			case 'x' :	mode = 'x';
					modeflag++;
					if (!typeflag)
						type = LSZ;
					strulast = 0;
					break;
			case 'd' :	mode = 'd';
					modeflag++;
					if (!typeflag)
						type = LSZ;
					strulast = 0;
					break;
			case 'o' :	mode = 'o';
					modeflag++;
					if (!typeflag)
						type = LSZ;
					strulast = 0;
					break;
			case 'c' :	mode = 'c';
					modeflag++;
					if (!typeflag) {
						hntype = '\0';
						type = BSZ;
					}
					strulast = 0;
					break;
			case 'a' :	mode = 'a';
					modeflag++;
					if (!typeflag) {
						hntype = '\0';
						type = BSZ;
					}
					strulast = 0;
					break;

			case 'l' :	hntype = '\0';
					type = LSZ;
					typeflag++;
					strulast = 0;
					break;
			case 'h' :	hntype = 'h';
					type = LSZ;
					typeflag++;
					strulast = 0;
					break;
			case 'n' :	hntype = 'n';
					type = LSZ;
					typeflag++;
					strulast = 0;
					break;
			case 't' :	hntype = '\0';
					type = SSZ;
					typeflag++;
					strulast = 0;
					break;
			case 'b' :	hntype = '\0';
					type = BSZ;
					typeflag++;
					strulast = 0;
					break;

			default  :	longjmp(syn,0);
		}
	}

	/*
	 * It's difficult to decide how to treat options h (show both
	 * as long and as 4 chars) and n (show symbols for pointers):
	 * given that we want them to be mutually exclusive (because
	 * h keeps columnar alignment but n abandons it), and the user
	 * must be able to clear them once set, and they're compatible
	 * with more than one mode, treat them as alternatives to type l.
	 */

	if(struflag > 1)
		error("only one structure may be specified\n");
	if(modeflag > 1)
		error("only one mode may be specified: x d o c or a\n");
	if(typeflag > 1)
		error("only one type may be specified: l h n t or b\n");
	if(procflag > 1)
		error("only one of -p or -sproc may be specified\n");
	if((struflag || strulast) &&
	   (mode == 'a' || (type != LSZ && (mode == 'd' || mode == 'o'))))
		error("structure mode %c type %c is unsupported\n",
			mode, hntype? hntype: " bt l"[type]);
		/* because %o and %d digits exceed columnar allowances */

	/*
	 * Don't parsefieldspec() until all the options are in,
	 * because -w redirection needs to print the args, but
	 * parsefieldspec() breaks the fieldspec into fields.
	 * Do it before advancing to address and count in case
	 * the user is intentionally feeding in fieldspecs for
	 * later use, not supplying any addresses yet.
	 */
	if (struflag)
		parsefieldspec(fieldspec);
	struflag = strulast |= struflag;

	if(!args[optind])
		longjmp(syn,0);
	origaddr = headaddr = strcon64(args[optind++],phys);
	if(args[optind]) {
		count = strcon(args[optind],'d');
		if(args[optind+1])
			longjmp(syn,0);
	}

	if((mode == 'a' && hntype != '\0') || (mode == 'c' && hntype == 'n')) {
		prerrmes("mode %c type %c is unsupported: using type l\n",
			mode, hntype);
		hntype = '\0';
	}

	if (!struflag) {
		prod(headaddr,headaddr+count*type,phys,proc,-1);
		return;
	}

	if (forwfld >= 0 && !args[optind])
		count = 0x80000000;

	for (done = slot = 0; slot < count; slot++) {
		tailaddr = headaddr;
		for (c = 0; c < depth; c++) {
			/* note: ptrfld read is virtual even when -p */
			readmem((vaddr_t)tailaddr+ptrflds[c],1,proc,
				&tailaddr,sizeof(vaddr_t),"fieldspec ptrfld");
			if ((vaddr_t)tailaddr == 0 && count != 1)
				break;
		}
		if (tailaddr != 0 || depth == 0 || count == 1) {
			tailaddr += strfld;
			prod(tailaddr+firstfld,tailaddr+lastfld+1,phys,proc,
				(count == 1)? -2: slot);
			done++;
		}
		if (forwfld >= 0) {		/* linked list */
			/* note: forwfld read is virtual even when -p */
			readmem((vaddr_t)headaddr+forwfld,1,proc,
				&headaddr,sizeof(vaddr_t),"fieldspec forwfld");
			if ((vaddr_t)headaddr == 0 || headaddr == origaddr)
				break;
		}
		else
			headaddr -= forwfld;	/* array */
	}
	if (done == 0 && count != 0)
		error("all slots indirected through null ptrfld\n");
}

/* print dump */
static void
prod(phaddr_t addr, phaddr_t eaddr, int phys, int proc, int slot)
{
	unsigned long i, j;
	unsigned long precision, value;
	static char format[] = "%.*x";
	char buffer[MMU_PAGESIZE];
	char chars[18];
	char *charp, *bufptr;
	int pad, buflen;
	int currtype;

	format[3] = mode;
	while ((eaddr - addr) & (type - 1))
		++eaddr;
	if (eaddr == addr)
		return;

	chars[16] = '\n';
	chars[17] = '\0';
	buflen = 0;

	for(i = 0; addr < eaddr; addr += currtype, i += currtype) {
		if ((i & 15) == 0 && i != 0 && mode != 'a') {
			/*
			 * Complete the previous output line
			 */
			if(hntype == 'h') {
				prpad(&pad);
				while (--charp >= chars)
					if (!isprint(*charp))
						*charp = '.';
				fputs(chars,fp);
			}
			else
				putc('\n',fp);
		}

		if (buflen < type) {	/* currtype is not yet known */
			/*
			 * Read as far as we can without
			 * incurring premature read error
			 */
			if (((eaddr-1) & ~(MMU_PAGESIZE-1)) ==
				(addr & ~(MMU_PAGESIZE-1)))
				buflen = eaddr - addr;
			else {
				buflen = MMU_PAGESIZE-(addr&(MMU_PAGESIZE-1));
				if (buflen < type)
					buflen = type;
			}
			readmem64(addr,!phys,proc,buffer,buflen,"od");
			bufptr = buffer;
		}

		if ((i & 15) == 0) {
			/*
			 * Prepare the next line, putting types into chars
			 */
			memset(charp = chars,
				(mode=='c' || mode=='a')? BSZ: type, 4*LSZ);
			if (slot != -1) {
				pad = 12;
				if (slot != -2 /* passed if count is 1 */)
					pad -= fprintf(fp,"SLOT%4u",slot);
				for (j = 0; j < 4*LSZ; j += LSZ)
					prfield(i, j, &pad, chars);
				putc('\n',fp);
			}
			if (mode == 'a')
				pad = 0;
			else {
				fprintf(fp,"%08llx:",addr);
				pad = 3;
			}
		}

		/*
		 * Get value from buffer and type from chars,
		 * replacing that type by the chars for -h
		 */

		switch(currtype = (int)(*charp)) {
		case LSZ:	value = *(unsigned long *)bufptr;
				*(unsigned long *)charp = value;
				if (mode == 'd' && (long)value < 0)
					--pad;	/* make way for the sign */
				break;
		case SSZ:	value = *(unsigned short *)bufptr;
				*(unsigned short*)charp = value;
				break;
		case BSZ:	value = *(unsigned char *)bufptr;
				*(unsigned char *)charp = value;
				break;
		}
		charp  += currtype;
		bufptr += currtype;
		buflen -= currtype;

		/*
		 * Print the field
		 */

		if (hntype == 'n' && currtype == LSZ) {
			char *s = db_sym_off(value);
			int len = strlen(s);
			if (mode == 'x' || *(s-1) /* it found a symbol */) {
				if (len > 10) {
					len = 10;
					if (pad > 2)
						pad = 2;
				}
				prpad(&pad);
				fputs(s,fp);
				pad = 12 - len;
				continue;
			}
			/* otherwise show number in decimal or octal */
		}

		prpad(&pad);

		switch(mode) {
			static char dprec[5] = {0,3,5,0,10};
			static char dpadd[5] = {0,1,3,0,02};
			static char oprec[5] = {0,3,6,0,11};
			static char opadd[5] = {0,1,2,0,01};

		case 'x':	precision = (currtype << 1);
				pad = currtype;
				break;
		case 'd':	precision = dprec[currtype];
				pad = dpadd[currtype];
				break;
		case 'o':	precision = oprec[currtype];
				pad = opadd[currtype];
				break;
		case 'c':	putch(value);
				continue;
		case 'a':	putc(value,fp);
				continue;
		}

		raw_fprintf(fp,format,precision,value);
		/* show decimal or octal even in hexmode */
	}

	/*
	 * Complete the last output line
	 */
	if(hntype == 'h') {
		while ((i & 15) != 0) {
			i += BSZ;
			pad += 3;
		}
		prpad(&pad);
		charp[0] = '\n';
		charp[1] = '\0';
		while (--charp >= chars)
			if (!isprint(*charp))
				*charp = '.';
		fputs(chars,fp);
	}
	else
		putc('\n',fp);
}

static
prpad(int *padp)
{
	while (*padp > 0) {
		putc(' ',fp);
		--*padp;
	}
}

static
prfield(int i, int j, int *padp, char *types)
{
	static int prevtype;
	char plus[10];
	char *name;
	int extent;
	int offset;
	int len;

	i += firstfld + j;
	if (i > lastfld)
		return;
	if (!tpbase->name) {	/* no structure fields: array offsets only */
		prpad(padp);
		*padp = 12 - fprintf(fp,"%x",i);
		return;
	}

	/*
	 * Locate structure field name
	 */
	while (tp == tpbase || tp->offset < i)
		++tp;
	name = tp->name;
	while ((offset = tp->offset) > i) {
		if (--tp == tpbase) {	/* offset 0 *might* be unnamed? */
			offset = 0;
			name = "0";
			break;
		}
		name = tp->name;
	}

	if ((i - offset) >= LSZ && j > 0) {
		prevtype = type;
		*padp += 12;
		return;
	}
	extent = tp[1].offset - i;

	len = strlen(name);
	if (i == offset)
		plus[0] = '\0';
	else
		len += sprintf(plus,".%x",i-offset);

	if (len > 11		/* name+pad won't fit in 12-char allowance */
	&&  extent <= LSZ	/* following field has a different name */
	&&  i+LSZ <= lastfld	/* following field needs to be printed */
	&&  j < 3*LSZ) {	/* cannot overflow above the -h columns */
		/*
		 * Then we have a problem fitting the field name in:
		 * first see if we can move it left slightly (but
		 * not so far as to misname the previous digits),
		 * then omit initial characters of the name.
		 */
		do {	/* ensure at least one pad space */
			putc(' ',fp);
			--*padp;
		} while (*padp > prevtype && j > 0);
		while (len > 11 + *padp) {
			++name;
			--len;
		}
		*padp -= len - 11;
		len = 11;
	}

	/*
	 * Print justified structure field name
	 */
	prpad(padp);
	fprintf(fp,"%s%s",name,plus);
	*padp = 12 - len;

	if (mode != 'x' || type != LSZ || extent >= LSZ) {
		prevtype = type;
		return;
	}

	/*
	 * If working in hexadecimal mode long type,
	 * we can break longs into chars and shorts:
	 * modify types array according to field sizes
	 */
	types += j;
	j = i + LSZ;
	while (i < j) {
	 	extent = (++tp)->offset - i;
	 	if (extent != BSZ && extent != SSZ)
			extent = j - i;
		*types = extent;
		types += extent;
		i += extent;
	}
	prevtype = extent;
}

static
parsefieldspec(char *struc)
{
	char *cp, *np, *firstfp, *lastfp, *sbuf, *dbuf;
	char punct;
	int width;
	static char *currfields, *minmalloc, *maxmalloc;
	static char svar[2], dvar[2]; /* static for easy '\0' termination */
	static struct offstable dummy;
	static int wellparsed;

	/*
	 * Parse od's -ffields argument: first expand initial [$X=][$Y]
	 * $$ $A-$Z here are similar to strcon()'s saved values $$ $a-$z;
	 * but these ones are (maybe complex) character strings not values,
	 * and $A-$Z can be supplied in the environment (or shown by !echo $X)
	 */

	sbuf = NULL;
	*dvar = '$';
	if (*struc == '$') {
		*svar = *++struc;
		if (*svar != '$' && ('A' > *svar || *svar > 'Z'))
			error("$%s is not a fieldspec\n", svar);
		if (*++struc == '=') {
			*dvar = *svar;
			*svar = '\0';
			if (*++struc == '$') {
				*svar = *++struc;
				if (*svar != '$' && ('A' > *svar||*svar > 'Z'))
					error("$%s is not a fieldspec\n", svar);
				++struc;
			}
		}
		if (*svar) {
			/*
			 * If we're doing exactly the same as before,
			 * just use the parsed values already set,
			 * without getting into mallocs.
			 */
			if (*svar == '$' && *dvar == '$'
			&&  *struc == '\0' && wellparsed)
				return;
			cp = (*svar == '$')? currfields: getenv(svar);
			if (cp == NULL)
				error("$%s is not a saved fieldspec\n", svar);
			/* allocate buffer for destructive parsing */
			sbuf = cr_malloc(strlen(cp) + strlen(struc) + 1,
				"fieldspec");
			strcpy(sbuf, cp);
			strcat(sbuf, struc);
			struc = sbuf;
		}
	}

	dbuf = cr_malloc(strlen(struc) + 3, "fieldspec");
	if (dbuf < minmalloc || minmalloc == NULL)
		minmalloc = dbuf;
	if (dbuf > maxmalloc)
		maxmalloc = dbuf;
	dbuf[0] = *dvar;
	dbuf[1] = '=';
	strcpy(dbuf + 2, struc);
	wellparsed = 0;

	/*
	 * Parse od's -ffields argument, now $saves have been expanded:
	 * struct[:forwfld][*ptrflds][+strfld][=struct][:[firstfld]-[lastfld]]
	 * e.g. od -fmodctl:mc_next*mc_modp*mod_obj.md_mcl=modctl_list modhead
  * or od -fmodctl:mc_next*mc_modp+mod_obj=modobj:md_space-md_bss_size modhead
	 *
	 * Start by detaching optional :[firstfld]-[lastfld] from the end,
	 * since its colon would get confused with earlier ones; but leave
	 * its parsing until last because only then do we know its struct.
	 * As in getargs(), allow but don't encourage OS5's ".." for range
	 * (but may want to encourage it in future, if we permit -offsets).
	 */

	firstfp = NULL;
	if ((np = strstr(struc, "..")) != NULL
	||  (np = strrchr(struc, '-')) != NULL) {
		*np++ = '\0';
		if (*np == '.')
			np++;
		lastfp = np;
		if ((np = strrchr(struc, ':')) == NULL)
		    error("fieldspec lacks :[firstfld] before -[lastfld]\n");
		*np++ = '\0';
		firstfp = np;
	}

	if ((np = strpbrk(struc, ":*+=")) == NULL)
		punct = '\0';
	else {
		punct = *np;
		*np++ = '\0';
	}
	tpbase = structfind(struc, "");
	width = tpbase->offset;

	forwfld = -width;		/* default for array */
	if (punct == ':') {
		if ((np = strpbrk(cp = np, "*+=")) == NULL)
			punct = '\0';
		else {
			punct = *np;
			*np++ = '\0';
		}
		if (!*cp)
			error("fieldspec lacks forwfld after :\n");
		forwfld = structfind(struc, cp)->offset;
	}

	depth = 0;
	while (punct == '*') {
		if ((np = strpbrk(cp = np, "*+=")) == NULL)
			punct = '\0';
		else {
			punct = *np;
			*np++ = '\0';
		}
		if (depth >= MAXDEPTH)
			error("fieldspec contains too many *ptrflds\n");
		ptrflds[depth++] = parse1field(struc, cp);
		tpbase = &dummy;
		width = 0;
	}

	strfld = 0;
	if (punct == '+') {
		if ((np = strchr(cp = np, '=')) == NULL)
			punct = '\0';
		else {
			punct = *np;
			*np++ = '\0';
		}
		strfld = parse1field(struc, cp);
		tpbase = &dummy;
		width -= strfld;
	}

	if (punct == '=') {
		tpbase = structfind(struc = np, "");
		width = tpbase->offset;
	}

	firstfld = 0;
	lastfld = width - 1;
	if (firstfp != NULL) {
		if (*firstfp)
			firstfld = structfind(struc, firstfp)->offset;
		if (*lastfp) {
			tp = structfind(struc, lastfp);
			lastfld = tp->name? ((++tp)->offset - 1): tp->offset;
		}
		if (firstfld > lastfld)
			error("%s-%s is an invalid range\n", firstfp, lastfp);
		if (width > 0 && lastfld >= width)
			error("%x is out of range 0-%x\n", lastfld, width - 1);
	}
	if (lastfld < 0)
		error("fieldspec lacks structure or range to determine size\n");

	if ((tp = tpbase)->name && lastfld - firstfld >= LSZ)
		firstfld &= ~(LSZ-1);	/* align fields on longs */

	/*
	 * Parsing was successful so update $saves
	 */

	wellparsed = 1;
	cr_unfree(dbuf);
	cr_free(sbuf);
	if (*dvar != '$') {
		cp = getenv(dvar);
		if (putenv(dbuf) != 0) {
			prerrmes("cannot add $%s to environment\n", dvar);
			dbuf[0] = '$';
			cp = NULL;
		}
		if ((cp -= 2) >= minmalloc && cp <= maxmalloc)
			free(cp);
		/* else it came from the original environment */
	}
	if (currfields) {
		currfields -= 2;
		if (*currfields == '$')
			free(currfields);
		/* else it is saved and freed separately */
	}
	currfields = dbuf + 2;
}

static int
parse1field(char *struc, char *field)
{
	int offset = 0;
	char *np;

	while (field != NULL) {
		if ((np = strpbrk(field, ":.")) != NULL && *np == ':') {
			*struc = '\0';
			struc = field;
			*np++ = '\0';
			np = strchr(field = np, '.');
		}
		if (np != NULL)
			*np++ = '\0';
		if (!*field)
			error("fieldspec contains an empty field\n");
		offset += structfind(struc, field)->offset;
		*struc = '\0';
		field = np;
	}

	return offset;
}

char odhelp[] =
"\n"
"od option -n like option -l, but shows symbol[+hexoffset] if symbol is found.\n"
"od option -ffieldspec for powerful use with structures, arrays, linked lists.\n"
"\n"
"The fieldspec is parsed differently from other command arguments: its general\n"
"form is   struct[:forwfld][*ptrflds][+strfld][=struct][:[firstfld]-[lastfld]]\n"
"Let\'s build that up from simpler examples.\n"
"\n"
"\"od -fproc *l.procp\" displays the default engine\'s current process structure\n"
"(error when idling).  Address *l.procp is handled by normal command argument\n"
"parsing, which can now process * and .fieldname: -fproc puts proc fieldnames\n"
"(every 4 bytes) above the displayed values, and displays to end of structure.\n"
"\n"
"\"od -fproc:p_parent *l.procp\" displays the default engine\'s current process\n"
"structure first, then proceeds up the p_parent chain showing each process\n"
"structure, until it reaches a null p_parent (or the original address, were\n"
"it a circularly linked list); a count argument would limit these iterations.\n"
"\n"
"\"od -f528:44 *l.1a4c\" does the same, except that offsets are shown instead of\n"
"fieldnames: any struct can be replaced by its size (hexadecimal 1-10000), any\n"
"fieldname can be replaced by its offset (hexadecimal 0-ffff).  Use the \"size\"\n"
"command to see sizes of structures known to crash, \"size -f\" to see offsets\n"
"too; use the \"addstruct\" command to add a structure table (which is saved as\n"
"/usr/lib/crash/structoffs.so, automatically loaded with crash thereafter).\n"
"\n"
"\"od -nfvfssw vfssw *nfstype\" displays the virtual filesystem switch table,\n"
"-n supplying symbols (which upset the alignment of fieldnames over values).\n"
"Since the switch table is an array not a linked list, no :forwfld is given,\n"
"and the count would default to one were it not supplied (by *nfstype).\n"
"\n"
"Perhaps the linked or arrayed structures just point to the fields to be shown.\n"
"\"od -fproc:p_next*p_execinfo=execinfo *practive\" displays execinfo for all\n"
"active processes: =execinfo (or =b0) to type the structure reached.\n"
"\n"
"Several such indirections might be needed:\n"
"\"od -chfproc:p_parent*p_execinfo*ei_execsw*8=10 *l.procp\" shows sixteen (0x10)\n"
"characters from *exec_name (offset 8 in struct execsw) for the current process\n"
"and each of its ancestors (omitting process 0 which has null ei_execsw).\n"
"\"**ptrfld\" is not permitted: use \"*ptrfld*0\".\n"
"\n"
"Substructure fields might be needed within any field:\n"
"\"od -fmodctl:mc_next*mc_modp*mod_obj.md_mcl=modctl_list modhead\".\n"
"Or even structfield.substructfield.subsubstructfield...; or hexadecimal\n"
"offsets used instead e.g. \"*mod_obj.3c\" in place of \"*mod_obj.md_mcl\".\n"
"Some fieldnames occur in more than one structure: ambiguous fieldnames are\n"
"rejected, but can be determined by prefixing structure: e.g. \"page:p_next\".\n"
"\n"
"But the final ptrfld (or forwfld or od address) may point, not to the\n"
"structure to be displayed, but to a superstructure of it: use +strfld.\n"
"\"od -fproc:p_next*p_execinfo+ei_exdata=exdata *practive\" displays exdata\n"
"for all active processes: =exdata (or =3c) to type the structure reached.\n"
"\n"
"Having reached the structure or array to be displayed, perhaps only a part\n"
"of it is to be shown: use range :[firstfld]-[lastfld].  If firstfld is not\n"
"given, 0 is assumed; if lastfld is not given, structure size-1 is assumed;\n"
"if lastfld is given, and fieldnames are not wanted, =struct is unnecessary.\n"
"\"od -fproc:p_next*p_execinfo+ei_exdata=exdata:ex_datorg-ex_renv *practive\".\n"
"\n"
"Since a useful fieldspec can be very complex, once it has been accepted,\n"
"subsequent \"od\"s assume it until mode (-xdoca) or type (-lhntb) is reset.\n"
"Even then, the last fieldspec accepted can be retrieved by \"-f$$\" or\n"
"extended by \"-f$$more\": a good way to build up a fieldspec in stages.\n"
"\n"
"The -f option actually has the form -f[$X=][$Y][morefieldspec].  Just as $$\n"
"automatically saves the last fieldspec, $A through $Z can save fieldspecs by\n"
"assignment as they are used e.g. -f$X=$$ or -f$X=fieldspec or -f$X=$Ymore.\n"
"$A through $Z can even be supplied in the environment to crash, and are kept\n"
"there for inspection by \"!echo $X\", or \"!set -f; echo $X\" to stop * expansion.\n"
"\n"
"(Normal command argument parsing supports similar saved values: $$ for the\n"
"last evaluated, and $a through $z for values assigned when used.  These do\n"
"not belong to the environment; but \"od -n\" display may show them as symbols.)\n";
