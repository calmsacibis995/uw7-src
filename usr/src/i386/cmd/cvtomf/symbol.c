#ident	"@(#)cvtomf:symbol.c	1.1"

/*
 *	Copyright (C) The Santa Cruz Operation, 1984-1988, 1989.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/* Enhanced Application Compatibility Support */
/*	MODIFICATION HISTORY
 *
 *	S000	sco!kai		May 1,1989
 *	- folded in seanf's changes to dynamically allocate type_index[]
 *	S001 	sco!kai		May 31, 1989
 *	- static functions will have scnum of 0; set it to 1
 *	- isfunc() call corrected as well as isfunc() itself
 *	S002	sco!kai		Aug 10, 1989
 *	- a few bug fixes: line numbers were just begin generated as a
 *	linear sequence. Use the real omf line numbers but relative to
 *	procedure start; autos and args should have scnum -1
 */

#include "cvtomf.h"
#include "omf.h"
#include "symbol.h"

extern long external[];
extern int exts;
extern char *strdup(char *);
extern struct scnhdr scnhdr[];
extern long nlnno;

long *type_index;	/* S000 */
static short cur_indx;
unsigned char buffer[SYMBOL_COUNT];	/* S000 */
struct sym_struct *syms;	/* S000 */

int sym_indx = 0;

struct Functions {
  long	start;
  long	end;
  long	symndx;
  long  lineno;
  char	*name;
} func_array[1024];

#pragma pack(1)
struct Coff_line {
	long	addr;
	unsigned short	lineno;
} line_entry;
#pragma pack()

int fun_indx = 0;

static int block_lev = 0;

struct func_info {
  char	*name;
  int	start_line;
  long	start_addr;
  long	end_addr;
  int	end_line;
  int	scn;
  int	sym_indx;
};

struct func_info info[1024];
int info_indx;
  
int find_name(name)
     char *name; {
       int a;
       
       for (a=sym_indx-1; a>=0; a--) {
	 if (!strncmp(syms[a].name, "  ", 2)) {
	   if (!strncmp(name, syms[a].name+2, strlen(name)))
	     return(a);
	 }
	 if (!strcmp(name, syms[a].name))
	   return(a);
       }
       return(-1);
     }

struct func_aux {
  long	tagndx;
  long	fsize;
  long	lnnoptr;
  long	endndx;
  unsigned short	tvndx;
} *func_aux;

struct bf_aux {
  long	unused1;
  unsigned short	lnno;
  long	unused2;
  short	unused3;
  long	endndx;
} *bf_aux;

long prev_func = -1;
long prev_symno = -1;

/* find_line:  finds line number closest to, but not greater than,
   addr
*/

int find_line(addr)
long addr; {
	int a;
	long prev_addr = -1;

#ifdef DEBUG
	if (debug)
		fprintf(stderr,"find_line(%08lx)\n",addr);
#endif
/* used to be line_indx-1 */
	for (a=0; a<line_indx; a++) {
#ifdef DEBUG
		if (debug)
			fprintf(stderr,"lines[%d].offset=%08lx, lines[%d].offset=%08lx\n",
				a,lines[a].offset,a+1,lines[a+1].offset);
#endif
		if (lines[a].offset == addr)
			return(lines[a].number);
		if (lines[a+1].offset > addr)
			return(lines[a].number);
		if (lines[a+1].offset < lines[a].offset)
			return(lines[a].number);
	}
#ifdef DEBUG
	if (debug)
		fprintf(stderr,"find_line: cannot find %08lx, returning 0\n",
			addr);
#endif
	return(0);
}

int find_line_index(addr)
	long addr; {
		int a;
		long prev_addr = -1;

		for (a=0; a<line_indx-1; a++) {
			if (lines[a+1].offset > addr)
				return(a);
		}
		return(-1);
	}

/* Begin S001 - fair amount of redesign */ 
int isfunc(indx)
long indx; 
{
       long cur_pos;
       unsigned char x; 

       if(indx < 512)	/* prim types are never functions */
	  return(0);
       indx -= 512;
       cur_pos = lseek(typ_file, 0L, 1);
       lseek(typ_file, type_index[indx] + 3, 0);  /* skip linkage and length */
       read(typ_file, &x, 1);
       lseek(typ_file, cur_pos, 0);
       if(x == 0x75)
		return(1);
       return(0);
}
/* End S001 */

void
process_sym() {
    extern FILE *symentfile;
    int len;
    int type;
    int count;
    unsigned char symbuf[SYMESZ];
    unsigned char size;
    unsigned char *bp;
    struct psr psr;
    struct bsr bsr;
    struct bpr bpr;
    struct clr clr;
    struct dsr dsr;
    struct rsr rsr;
    struct csr csr;
    int a, offset;

    offset = syms[0].value;

#ifdef DEBUG
	if (debug)
		fprintf(stderr,"file_name[0] = %s\n",file_name[0]);
#endif

    for (a=0; a < sym_indx; a++) {
	  if (debug&&verbose)
		fprintf(stderr,"Line %d, File %s, a=%d, sym_indx=%d\n",
			__LINE__,__FILE__,a,sym_indx);
      syms[a].lineno = -1;
      if (isfunc(syms[a].type)) {	/* S001 - don't subtract one */
#ifdef DEBUG
		if (debug)
			fprintf(stderr,"\t%s is a function\n",syms[a].name);
#endif
        syms[a].lineno = find_line(syms[a].value) - 1;
      }
#ifdef DEBUG
	if (debug)
      fprintf(stderr,"syms[%d]: %s, %08lx, %d, %d, %d, line %d\n",
	      a, syms[a].name, syms[a].value, syms[a].scnum,
	      syms[a].sclass, syms[a].type, syms[a].lineno);
#endif
    }
#ifdef DEBUG
	if (debug) 
		for (a=0; a< line_indx; a++) {
		  fprintf(stderr,"lines[%d]: %08lx, %d\n", a, lines[a].offset,
			  lines[a].number);
		}
#endif

    lseek(sym_file, 0L, 0);
    while ((count = read(sym_file, &size, 1))>0) {
      char name_buf[1024];
      int scn;
      int sclass;

#ifdef DEBUG
	  if (debug &&verbose)
		fprintf(stderr,"count=%d\n",count);
#endif
      memset(name_buf, 0, sizeof(name_buf));
      memset(buffer, 0, sizeof(buffer));
      buffer[0] = size;
      read(sym_file, &buffer[1], buffer[0]);

      len = (int)buffer[0];
      type = (int)buffer[1];
      switch (type) {
      case 0:
	memcpy(&bsr, &buffer[1], sizeof(bsr));
	if (block_lev != 0) {
#ifdef DEBUG
		if (debug)
			printf("Block Start:\n\toffset=%08lx, length=%08lx, name=%*.*s\n",
			   bsr.offset, bsr.length, bsr.name_len, bsr.name_len,
			   (bsr.name_len > 0) ? &buffer[sizeof(bsr)+1] : "");
#endif
		block_lev++;
		external[++exts] = symbol(".bb", bsr.offset, 1, C_BLOCK, 0, 0);
	}
	break;
      case 1:
	memcpy(&psr, &buffer[1], sizeof(psr));
#ifdef DEBUG
	if (debug)
		printf("Procedure start:\n\toffset=%08lx, type index=%04x,"
	       " length=%08lx, debug start=%04lx, debug end=%04lx,"
	       " reserved = %04x, name = %s %*.*s\n",
	       psr.offset, psr.index, psr.length, psr.d_start, psr.d_end,
	       psr.reserved, (psr.near_or_far_p) ? "Far " : "Near ",
	       psr.name_len, psr.name_len, &buffer[sizeof(psr)+1]);
#endif
	strncpy(name_buf, &buffer[sizeof(psr)+1], psr.name_len);

	info[info_indx].name = strdup(name_buf);
	info[info_indx].name[psr.name_len] = '\0';
	info[info_indx].start_line = find_line(psr.offset);
	info[info_indx].end_line = find_line(psr.offset+psr.length);

	a = find_name(name_buf);
	info[info_indx].start_addr = syms[a].value;
	info[info_indx].end_addr = syms[a].value + psr.length;
	scn = 1;
	sclass = C_EXT;
	if (a != -1) {
	  psr.offset = syms[a].value;
	  if(syms[a].scnum)	/* S001 don't change if this is 0 */
		  scn = syms[a].scnum;
	  sclass = syms[a].sclass;
	}
	if (prev_func != -1) {
	  struct func_aux f_a;
	  long pointer;

	  fseek(symentfile, prev_func, 0);
	  fread(&f_a, SYMESZ, 1, symentfile);
	  f_a.endndx = prev_symno;
	  fseek(symentfile, prev_func, 0);
	  fwrite(&f_a, SYMESZ, 1, symentfile);
	  fseek(symentfile, 0, 2);
	}
	external[++exts] = symbol(name_buf, psr.offset, scn, sclass, 0x24, 1);
	/*external[++exts] = symbol(name_buf, psr.offset, 0, sclass, 0x24, 1);*/
	info[info_indx].scn = scn;
	info[info_indx].sym_indx = ftell(symentfile)/SYMESZ - 1;
#ifdef DEBUG
	if (debug) {
		fprintf(stderr,"name=%s, start_line=%d, end_line=%d, scn=%d, "
			"index=%d, exts=%d\n",
			info[info_indx].name, info[info_indx].start_line,
			info[info_indx].end_line, info[info_indx].scn,
			info[info_indx].sym_indx, exts);
		fprintf(stderr,"ftell(symentfile)/SYMESZ (%d) + 1= %d\n",
			SYMESZ, ftell(symentfile)/SYMESZ+1);
	}
#endif
	func_array[fun_indx++].symndx = external[exts];
	prev_func = ftell(symentfile);
	prev_symno = external[exts];
	func_aux = (struct func_aux *)symbuf;
	func_aux->tagndx = 0;
	func_aux->fsize = psr.length;
	func_aux->lnnoptr = 0;
	func_aux->endndx = -1;
	func_aux->tvndx = 0;
	fwrite(func_aux, SYMESZ, 1, symentfile);
	++nsyms;
	external[++exts] = symbol(".bf", psr.offset+5, scn, C_FCN, 0, 1);
	bf_aux = (struct bf_aux *)symbuf;
/*	bf_aux->lnno = info[info_indx++].start_line;*/
	bf_aux->lnno = find_line(psr.offset+5);
	info_indx++;
	fwrite(bf_aux, SYMESZ, 1, symentfile);
	++nsyms;
	break;
      case 2:
#ifdef DEBUG
	if (debug)
		printf("Procecure, Block, or With end\n");
#endif
	if (block_lev == -1) {
		block_lev = 0;
		break;
	}
	if (block_lev == 0) {
	  external[++exts] = symbol(".ef", 0, 1, C_FCN, 0, 0);
		block_lev --;
	} else {
	  external[++exts] = symbol(".eb", 0, 1, C_BLOCK, 0, 0);
	  --block_lev;
	}
	break;
      case 4:
	memcpy(&bpr, &buffer[1], sizeof(bpr));
#ifdef DEBUG
	if (debug)
		printf("BP-relative symbol:\n\toffset=%08lx, type index=%04x,"
	       " name=%*.*s\n",
	       bpr.offset, bpr.index, bpr.name_len, bpr.name_len,
	       &buffer[sizeof(bpr)+1]);
#endif
	strncpy(name_buf, &buffer[sizeof(bpr)+1], bpr.name_len);
	external[++exts] = symbol(name_buf, bpr.offset, -1, 
		(bpr.offset >= 0) ? C_ARG : C_AUTO, 0, 0);	/* S002 scnum = -1 */
	break;
      case 5:
	memcpy(&dsr, &buffer[1], sizeof(dsr));
#ifdef DEBUG
	if (debug)
		printf("Local (data) Symbol\n\toffset=%08lx, seg=%04x, type"
	       " index=%04x, name=%*.*s\n",
	       dsr.offset, dsr.seg, dsr.index, dsr.name_len, dsr.name_len,
	       &buffer[sizeof(dsr)+1]);
#endif
	strncpy(name_buf, &buffer[sizeof(dsr)+1], dsr.name_len);
	a = find_name(name_buf);
	scn = 1;
	if (a != -1) {
	  dsr.offset = syms[a].value;
	  scn = syms[a].scnum;
	}
	external[++exts] = symbol(name_buf, dsr.offset, scn, C_STAT, 0, 0);
	break;
      case 0xb:
	memcpy(&clr, &buffer[1], sizeof(clr));
#ifdef DEBUG
	if (debug)
		printf("Code label\n\t:offset=%08lx, name = %s %*.*s\n",
	       clr.offset, (clr.near_or_far_p) ? "Far" : "Near",
	       clr.name_len, clr.name_len, &buffer[sizeof(clr)+1]);
#endif
	strncpy(name_buf, &buffer[sizeof(clr)+1], clr.name_len);
	external[++exts] = symbol(name_buf, clr.offset, 1, C_LABEL, 0, 0);
	break;
      case 0xd:
	memcpy(&rsr, &buffer[1], sizeof(rsr));
#ifdef DEBUG
	if (debug)
		printf("Register symbol\n\ttype index=%04x, register=%d, name=%*.*s\n",
	       rsr.index, rsr.reg, rsr.name_len, rsr.name_len,
	       &buffer[sizeof(rsr)+1]);
#endif
	strncpy(name_buf, &buffer[sizeof(rsr)+1], rsr.name_len);
	external[++exts] = symbol(name_buf, rsr.reg, 1, C_REG, 0, 0);
	break;
      case 0xe:
	memcpy(&csr, &buffer[1], sizeof(csr));
#ifdef DEBUG
	if (debug)
		printf("Constant symbol\n\ttype index=%04x\n",
	       csr.index);
#endif
	break;
      default:
#ifdef DEBUG
	if (debug)
		printf("Don't know type %d\n",type);
#endif
	break;
	  }
	}
#ifdef DEBUG
	if (debug&&verbose)
		fprintf(stderr,"count=%d\n",count);
	if (debug) {
		fprintf(stderr,"info_indx = %d\n",info_indx);
	}
#endif

	for (a=0; a< info_indx; a++) {
		scnhdr[info[a].scn-1].s_nlnno = 0;
		rewind(linenofile[info[a].scn-1]);
	}

	for (a=0; a< info_indx; a++) {

		unsigned long line, addr;
		extern FILE *linenofile[];
		unsigned long sline = 0;			/* S002 */

		line_entry.lineno = 0;
		line_entry.addr = info[a].sym_indx;
		fwrite(&line_entry, LINESZ, 1, linenofile[info[a].scn-1]);

		line = find_line_index(info[a].start_addr);
		nlnno++;
		scnhdr[info[a].scn-1].s_nlnno++;
		if (line == -1) {
			continue;	/* S001 - just ignore this one */
			fprintf(stderr,"Whoa!  Unknown line address!\n");
			exit(1);
		}
#ifdef DEBUG
		if (debug) {
			fprintf(stderr,"Starting %s (index %d)\n",
				info[a].name,info[a].sym_indx);
			fprintf(stderr,"lines[%d].offset=%08lx\n",line,lines[line].offset);
			fprintf(stderr,"start_addr=%08lx, end_addr=%08lx\n",
				info[a].start_addr, info[a].end_addr);
		}
#endif
		while (lines[line].offset <= info[a].end_addr &&
			lines[line].offset >= info[a].start_addr &&
			lines[line].number != 0L) {

			if(!sline)		/* S002 */
				sline = lines[line].number - 1;
			line_entry.lineno = lines[line].number - sline;

			line_entry.addr = lines[line].offset;
			fwrite(&line_entry, LINESZ, 1, linenofile[info[a].scn-1]);
#ifdef DEBUG
			if (debug) {
				fprintf(stderr,"lines[%d].offset=%d, info[%d].end_addr=%d, "
					"info[%d].start_addr=%d\n",
					line,lines[line].offset,a,info[a].end_addr,
					a,info[a].start_addr);
				fprintf(stderr,"\tline %d (%d), offset %08lx\n",
					line_entry.lineno, lines[line].number, line_entry.addr);
			}
#endif
			scnhdr[info[a].scn-1].s_nlnno++;
			nlnno++;
			line++;
		}
	}
  }


void write_types(file, offset, buf, size)
     int file;
     long offset, size;
     unsigned char *buf; {
       lseek(file, offset, 0);
       while (size>=1) {
	 int a;
	 unsigned short len;
	 len = *((unsigned short *)&buf[1]);
	 type_index[cur_indx] = lseek(file, 0L, 1);
#ifdef DEBUG
	 if (debug) {
		 fprintf(stderr,"index = %d, len=%d, offset=%ld\n",cur_indx, 
			 len, type_index[cur_indx]);
		 fprintf(stderr, "type = \n\t");
		 for (a=0; a<len+3; a++)
		   fprintf(stderr," %02x", buf[a]);
		 fprintf(stderr,"\n");
	  }
#endif
	 cur_indx++;

	 /* Begin S000 */
	 if ((cur_indx & 0xf) == 0) {	/* allocate in groups of 16 */
			type_index = realloc (type_index,sizeof(*type_index)*(cur_indx+16));
			if (type_index == 0) {
				fprintf (stderr, "Out of type index space\n");
				exit (1);
			}
	 }
	/* End S000 */

	 write(file, buf, len+2);
	 size -= len + 3;
	 buf += len + 3;
       }
       return;
     }

void process_typ() {
     int count, bytes, tlist, nlist;
     int a;
     unsigned char linkage;
     unsigned short len;

     for (a=512; a < cur_indx; a++) {
       lseek(typ_file, type_index[a], 0);
       read(typ_file, &linkage, 1);	/* skip linkage byte */
       read(typ_file, &len, 2);	/* read length word */
#ifdef DEUG
	   if (debug)
		   printf("type %d, length=%d, offset=%d\n", a, len, type_index[a]);
#endif
     }
     return;
   }
/* End Enhanced Application Compatibility Support */
