/*		copyright	"%c%" 	*/

#ident	"@(#)cvtomf:coff.c	1.2"

/*
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1989 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*
 *	Copyright (c) Altos Computer Systems, 1987
 */
/* Enhanced Application Compatibility Support */
/*	MODIFICATION HISTORY
 *
 *	S000	sco!kai		June 1,1989
 *	- rewrite of code that writes strings table. It was incorrect
 *	in several ways.
 *	S001	sco!kai		Sept 21, 1989
 *	- some changes to accompany broad re-write of cvtomf
 *	code for patching in lineno and reloc counts in .text aux records
 *	section(): avoid unnecessary open files
 *	added aux() and re-utilized line()
 *	changes here and there to the DEBUG printf's
 */


#include	"cvtomf.h"
#include	"coff.h"

/*
 *	scnsize: total size of all sections
 */

static long	scnsize;

/*
 *	strsize: total size of the string table
 */

long	strsize;

/*
 *	nscns, nreloc, nlnno, nsyms: section/relocation/line/symbol
 *	counters
 */

long	nscns, nreloc, nlnno, nsyms;

/*
 *	scnhdr, datafile, relocfile, linenofile: parallel section
 *	header/data/relocation/line arrays
 */

struct scnhdr scnhdr[MAXSCN];
int scnauxidx[MAXSCN];	/* S001: for patching #reloc and #line */
FILE *datafile[MAXSCN];
FILE *relocfile[MAXSCN];
FILE *linenofile[MAXSCN];

/*
 *	symentfile, strfile: symbol/string tables
 */

FILE *symentfile;
FILE *strfile;

/*
 *	coff: generate a COFF file
 */

coff(output)
char	*output;
{
	FILE *objfile;
	struct filehdr filehdr;
	struct reloc reloc;
	struct lineno lineno;
	struct syment syment;
	union auxent auxent;
	long	scnptr, relptr, lnnoptr;
	register long	i, j;
	char	buffer[MAXDAT];

#ifdef DEBUG
	if (debug){
		fprintf(stderr,"\n *** coff() ***\n\n");
		fprintf(stderr,"Trying to open %s for writing...\n",output);
	}
#endif

	objfile = fopen(output, "w");

	if (!objfile){
		perror("fopen");
		fatal("Cannot open \"%s\" for writing", output);
	}

	/* file header */

	filehdr.f_magic = MAGIC;
	filehdr.f_nscns = nscns;
	filehdr.f_timdat = time((long *)0);
	filehdr.f_symptr = FILHSZ + 
	    (SCNHSZ * nscns) + 
	    scnsize + 
	    (RELSZ * nreloc) + 
	    (LINESZ * nlnno);
	filehdr.f_nsyms = nsyms;
	filehdr.f_opthdr = 0;
	filehdr.f_flags = ARCH;

	fwrite((char *)&filehdr, FILHSZ, 1, objfile);

#ifdef DEBUG
	if (debug && verbose)
		fprintf(stderr, "coff filehdr: magic 0x%x, nscns 0x%x, symptr 0x%x, nsyms 0x%x, opthdr 0x%x, flags 0x%x\n",
		     			(int)filehdr.f_magic, (int)filehdr.f_nscns, filehdr.f_symptr,
		    filehdr.f_nsyms, (int)filehdr.f_opthdr, (int)filehdr.f_flags);
#endif

	/* section headers */

	scnptr = FILHSZ + (SCNHSZ * nscns);
	relptr = FILHSZ + (SCNHSZ * nscns) + scnsize;
	lnnoptr = FILHSZ + (SCNHSZ * nscns) + scnsize + (RELSZ * nreloc);

	for (i = 0; i < nscns; i++) {
		if ((scnhdr[i].s_flags & STYP_BSS) != STYP_BSS)
			scnhdr[i].s_scnptr = scnptr;
		else
			scnhdr[i].s_scnptr = 0;
		scnhdr[i].s_relptr = relptr;
		scnhdr[i].s_lnnoptr = lnnoptr;

		fwrite((char *)&scnhdr[i], SCNHSZ, 1, objfile);

		scnptr += scnhdr[i].s_size;
		relptr += RELSZ * scnhdr[i].s_nreloc;
		lnnoptr += LINESZ * scnhdr[i].s_nlnno;
	}

	/* section data */

	for (i = 0; i < nscns; i++) {
		if (scnhdr[i].s_size  && datafile[i]) {
			rewind(datafile[i]);

			while (scnhdr[i].s_size > MAXDAT) {
				fread(buffer, 1, MAXDAT, datafile[i]);
				fwrite(buffer, 1, MAXDAT, objfile);

				scnhdr[i].s_size -= MAXDAT;
			}

			fread(buffer, 1, scnhdr[i].s_size, datafile[i]);
			fwrite(buffer, 1, scnhdr[i].s_size, objfile);
		}

		fclose(datafile[i]);
	}

	/* relocation entries */

	for (i = 0; i < nscns; i++) {
		if (scnhdr[i].s_nreloc && relocfile[i]) {
			rewind(relocfile[i]);

			for (j = 0; j < (unsigned int)scnhdr[i].s_nreloc; j++) {
				fread((char *)&reloc, RELSZ, 1, relocfile[i]);
				fwrite((char *)&reloc, RELSZ, 1, objfile);
			}
		}

		fclose(relocfile[i]);
	}

	/* line numbers */

	for (i = 0; i < nscns; i++) {
	  int temp = scnhdr[i].s_nlnno;
	  int x = 0;

	  if (temp && linenofile[i]) {
	    rewind(linenofile[i]);
	    
	    for (j = 0; j < temp; j++) {
	      fread((char *)&lineno, LINESZ, 1, linenofile[i]) &&
	      fwrite((char *)&lineno, LINESZ, 1, objfile);
#ifdef DEBUG
		  if (debug)
			fprintf(stderr,"scn %d, lineno.l_addr.l_paddr=%08lx, "
				"lineno.l_lnno=%d\n",
				i, lineno.l_addr.l_paddr, lineno.l_lnno);
#endif
	    x++;
	    }
	  }
	  scnhdr[i].s_nlnno = x;
	  fclose(linenofile[i]);
	}

	/* 
	 * Begin S001
	 * patch section symbol aux records with #line and #reloc entires 
	 */
	for (i = 0; i < nscns; i++) {
		if(scnauxidx[i] && symentfile){
			fseek(symentfile, scnauxidx[i] * SYMESZ, 0);
			fread((char *)&auxent, AUXESZ, 1, symentfile);
			auxent.x_scn.x_nreloc = scnhdr[i].s_nreloc; 
			auxent.x_scn.x_nlinno = scnhdr[i].s_nlnno;
			fseek(symentfile, -AUXESZ, 1);
			fwrite((char *)&auxent, AUXESZ, 1, symentfile);
		}
		scnauxidx[i] = 0;
	}
	/* End S001 */
 		

	/* symbol table */
	fseek(objfile, filehdr.f_symptr, 0);

#ifdef DEBUG
	if (debug) {
		fprintf(stderr,"ftell(objfile)=%ld\n",ftell(objfile));
		fprintf(stderr,"filehdr.f_symptr = %ld\n",filehdr.f_symptr);
	}
#endif

	if (nsyms && symentfile) {
		rewind(symentfile);

#ifdef DEBUG
		if (debug && verbose)
			fprintf(stderr, "Hex dump of symbol table:\n");
#endif
		for (i = 0; i < nsyms; i++) {

#ifdef DEBUG
			int aux = 0;
#endif
			fread((char *)&syment, SYMESZ, 1, symentfile);
			fwrite((char *)&syment, SYMESZ, 1, objfile);
#ifdef DEBUG
			  if (debug)
				if(!aux)
				  fprintf(stderr,"index=%d, value=%08lx, scnum=%d, type=%d, "
				  "sclass=%d, numaux=%d\n", i,
				  syment.n_value, syment.n_scnum,
				  syment.n_type, syment.n_sclass,
				  syment.n_numaux);
				else
				  fprintf(stderr,"index=%d, aux\n");

			if (debug && verbose && !aux) {
				if (syment._n._n_n._n_zeroes == 0) {
					fprintf(stderr,"name at %08lx in string table\n\t",
						syment._n._n_n._n_offset);
				} else {
					fprintf(stderr,"name is %s\n\t",
						syment._n._n_name);
				}
			}
			if(!aux)
				aux = syment.n_numaux;
			else
				aux--;
#endif
		}
		fclose(symentfile);

#ifdef DEBUG
		if (debug) {
			fprintf(stderr,"strsize=%d\n",strsize);
			fprintf(stderr,"ftell(objfile)=%d\n",ftell(objfile));
		}
#endif

		/* 
		 * Begin S000
		 * always write the count, even if 0 
		 */
		(void)fwrite((char *)&strsize, sizeof strsize, 1, objfile);
		strsize -= sizeof(strsize);
		if (strfile) {
			rewind(strfile);
			while(strsize){
				int count;

				count = (strsize > MAXDAT) ? MAXDAT : strsize;
				(void)fread(buffer, 1, count, strfile);
				(void)fwrite(buffer, 1, count, objfile);
				strsize -= count;
			}
			fclose(strfile);
			strfile = NULL;
		}
				
		/* End S000 */

	}

	/* clean up */

	fclose(objfile);

	if (verbose) {
		printf("\n\t%d byte(s) of data\n", scnsize);
		printf("\t%d section(s)\n", nscns);
		printf("\t%d relocation entr(y,ies)\n", nreloc);
		printf("\t%d line number(s)\n", nlnno);
		printf("\t%d symbol(s)\n\n", nsyms);
	}

	scnsize = strsize = nscns = nreloc = nlnno = nsyms = 0;
}


/*
 *	section, scndata: create/initialize a section
 */

section(name, vaddr, size, flags)
char	*name;
long	vaddr, size, flags;
{
	register int	i;
	int	fill;

	i = nscns++;

	if (nscns > MAXSCN)
		fatal("Too many COFF sections");	/* exit */

	/* 
	 * S001
	 * avoid unnecesary open files:
 	 * STYP_INFO should only have a data file
	 * only STYP_TEXT should have linenofile
         */
	datafile[i] = tmpfile();
	if(flags != STYP_INFO){
		relocfile[i] = tmpfile();
		if(flags == STYP_TEXT)
			linenofile[i] = tmpfile();
	}

	fseek(datafile[i], size, 0);
	fill = (flags & STYP_TEXT) ? TXTFILL : DATFILL;

	for (; !ALIGNED(size); size++)
		putc(fill, datafile[i]);


	strncpy(scnhdr[i].s_name, name, SYMNMLEN);

	scnhdr[i].s_paddr = vaddr;
	scnhdr[i].s_vaddr = vaddr;
	scnhdr[i].s_size = size;
	scnhdr[i].s_nreloc = 0;
	scnhdr[i].s_nlnno = 0;
	scnhdr[i].s_flags = flags;

	scnsize += size;

	return nscns;
}


#define BUF	0
#define STRINGS	1

union pointers {
	char *u_p;
	char **u_pp;
};

scndata(scn, offset, buffer, size)
int	scn;
long	offset;
char	*buffer;
long	size;
{
	scnwrite(BUF, scn, offset, buffer, size);
}

cmntdata (scn, offset, strings, nstrings )
int scn;
long offset;
char **strings;
int nstrings;
{
	scnwrite(STRINGS, scn, offset, strings, (long)nstrings);
}

scnwrite(type, scn, offset, buffer, size)
int scn;
long offset;
union pointers buffer;
long size;
{
	register int i;

	i = scn - 1;
	fseek(datafile[i], offset, 0);
	if (type == BUF)
		fwrite(buffer.u_p, 1, size, datafile[i]);
	else
	{ register char **pp = buffer.u_pp;
		while (--size >= 0)
		{ 	/* write strlen+1 to include null char */
			fwrite(*pp, 1, strlen(*pp)+1, datafile[i]);
			++pp;
		}
	}
}

/*
 *	relocation: create a relocation table entry
 */

static int flag = 0;

relocation(scn, vaddr, symndx, type, offset)
int	scn;
long	vaddr, symndx;
int	type;
long	offset;
{
	register int	i;
	struct syment syment;
	struct reloc reloc;
	char	byte;
	short	word;
	long	dword;

#ifdef DEBUG
	if (debug && verbose)
	  fprintf(stderr, "relocation: scn %d, vaddr 0x%x, symndx 0x%x, type %d, offset 0x%x\n",
		  scn, vaddr, symndx, type, offset);
#endif

	i = scn - 1;

	fseek(symentfile, (long) symndx * SYMESZ, 0);
	fread((char *)&syment, SYMESZ, 1, symentfile);

#ifdef DEBUG
	if (debug && verbose)
		fprintf(stderr,"\tn_name=%s\n",syment.n_name);
#endif

	if (!strcmp(syment.n_name, ".file")) {
		flag = 1;
	  return;
	}

#if 0
	if (flag != 0) {
	  flag = flag - 1;
#	ifdef DEBUG
		if (debug && verbose)
			fprintf(stderr,"\tflag =%d, returning now\n",flag);
#	endif
	  return;
	}
	if (syment.n_numaux != 0 && flag == 0) {
	  flag = syment.n_numaux;
	}
	
#endif /* 0 */
#ifdef DEBUG
	if (debug)
		fprintf(stderr,"syment.n_name=%s\n",syment.n_name);
#endif
	offset += syment.n_value;

#ifdef DEBUG
	if (debug && verbose)
		fprintf(stderr, "relocation: new offset 0x%x\n",
				offset);
#endif



	nreloc++;
	scnhdr[i].s_nreloc++;

	reloc.r_vaddr = vaddr;
	reloc.r_symndx = symndx;
	reloc.r_type = type;

	fwrite((char *)&reloc, RELSZ, 1, relocfile[i]);

	switch (type) {

	case R_OFF8:
		fseek(datafile[i], vaddr, 0);
		fread(&byte, sizeof byte, 1, datafile[i]);

#ifdef DEBUG
		if (debug && verbose)
			fprintf(stderr, "relocation: R_OFF8, changing byte from 0x%x to 0x%x at vaddr 0x%x\n",
					byte, byte + offset, vaddr);
#endif
		byte += offset;

		fseek(datafile[i], vaddr, 0);
		fwrite(&byte, sizeof byte, 1, datafile[i]);
		break;

	case R_OFF16:
		fseek(datafile[i], vaddr, 0);
		fread(&byte, sizeof byte, 1, datafile[i]);

#ifdef DEBUG
		if (debug && verbose)
			fprintf(stderr, "relocation: R_OFF16, changing byte from 0x%x to 0x%x at vaddr 0x%x\n",
					byte, byte + (offset>> 8), vaddr);
#endif
		byte += offset >> 8;

		fseek(datafile[i], vaddr, 0);
		fwrite(&byte, sizeof byte, 1, datafile[i]);
		break;

	case R_DIR16:
		fseek(datafile[i], vaddr, 0);
		fread((char *)&word, sizeof word, 1, datafile[i]);

#ifdef DEBUG
		if (debug && verbose)
			fprintf(stderr, "relocation: R_DIR16, changing word from 0x%x to 0x%x at vaddr 0x%x\n",
					word, word + offset, vaddr);
#endif
		word += offset;

		fseek(datafile[i], vaddr, 0);
		fwrite((char *)&word, sizeof word, 1, datafile[i]);
		break;

	case R_DIR32:
		fseek(datafile[i], vaddr, 0);
		fread((char *)&dword, sizeof dword, 1, datafile[i]);

#ifdef DEBUG
		if (debug && verbose)
			fprintf(stderr, "relocation: R_DIR32, changing dword from 0x%x to 0x%x at vaddr 0x%x\n",
					dword, dword + offset, vaddr);
#endif
		dword += offset;

		fseek(datafile[i], vaddr, 0);
		fwrite((char *)&dword, sizeof dword, 1, datafile[i]);
		break;

	case R_PCRBYTE:
		fseek(datafile[i], vaddr, 0);
		fread(&byte, sizeof byte, 1, datafile[i]);

#ifdef DEBUG
		if (debug && verbose)
			fprintf(stderr, "relocation: R_PCRBYTE, changing byte from 0x%x to 0x%x at vaddr 0x%x\n",
					byte, byte + offset - (vaddr+1), vaddr);
#endif
		byte += offset - (vaddr + 1);

		fseek(datafile[i], vaddr, 0);
		fwrite(&byte, sizeof byte, 1, datafile[i]);
		break;

	case R_PCRWORD:
		fseek(datafile[i], vaddr, 0);
		fread((char *)&word, sizeof word, 1, datafile[i]);

#ifdef DEBUG
		if (debug && verbose)
			fprintf(stderr, "relocation: R_PCRWORD, changing word from 0x%x to 0x%x at vaddr 0x%x\n",
					word, word + offset - (vaddr+2), vaddr);
#endif
		word += offset - (vaddr + 2);

		fseek(datafile[i], vaddr, 0);
		fwrite((char *)&word, sizeof word, 1, datafile[i]);
		break;

	case R_PCRLONG:
		fseek(datafile[i], vaddr, 0);
		fread((char *)&dword, sizeof dword, 1, datafile[i]);

#ifdef DEBUG
		if (debug && verbose)
			fprintf(stderr, "relocation: R_PCRLONG, changing dword from 0x%x to 0x%x at vaddr 0x%x\n",
					dword, dword + offset - (vaddr+4), vaddr);
#endif
		dword += offset - (vaddr + 4);

		fseek(datafile[i], vaddr, 0);
		fwrite((char *)&dword, sizeof dword, 1, datafile[i]);
		break;

	default:
		fatal("Bad COFF relocation type");	/* exit */

	}
}


/*
 *	line: create a line number entry
 */

line(scn, paddr, lnno)
int	scn;
long	paddr;
int	lnno;
{
	register int	i;
	struct lineno lineno;

	i = scn - 1;

#ifdef DEBUG
	if(debug)
		fprintf(stderr,"line: %5d 0x%08x scn = %d\n",lnno, paddr, scn);
#endif

	nlnno++;
	scnhdr[i].s_nlnno++;
	lineno.l_addr.l_paddr = paddr;
	lineno.l_lnno = lnno;

	fwrite((char *)&lineno, LINESZ, 1, linenofile[i]);
}


/*
 *	symbol: create a symbol table entry
 */

long
symbol(name, value, scnum, sclass, type, aux)
char	*name;
long	value;
int	scnum, sclass, type, aux;
{
	int	len;
	struct syment syment;

	if (!nsyms)
		symentfile = tmpfile();

	len = strlen(name);

	if (len <= SYMNMLEN) {
		strncpy(syment.n_name, name, SYMNMLEN);
#ifdef DEBUG
		if (debug && verbose)
			fprintf(stderr, "SYMBOL: name %s, ", syment.n_name);
#endif
	} else
	 {
		if (!strsize) {
			strfile = tmpfile();
			strsize = sizeof strsize;
		}

		syment.n_zeroes = 0;
		syment.n_offset = strsize;
#ifdef DEBUG
		if (debug && verbose)
			fprintf(stderr, "strtable: offset 0x%x, name %s ", syment.n_offset, name);
#endif

		fseek(strfile, 0L, 2);
		fwrite(name, 1, ++len, strfile);
		strsize += len;
	}

	syment.n_value = value;
	syment.n_scnum = scnum;
	syment.n_type = type;
	syment.n_sclass = sclass;
	syment.n_numaux = aux;

	fseek(symentfile, 0L, 2);
	fwrite((char *)&syment, SYMESZ, 1, symentfile);

#ifdef DEBUG
	if (debug && verbose) {
		fprintf(stderr, "value 0x%x, scnum %d, type 0x%x, sclass 0x%x, numaux %d\n", 
		    syment.n_value, (int) syment.n_scnum,
		    (unsigned) syment.n_type, (int)syment.n_sclass,
		    (int)syment.n_numaux);
	}
#endif

	return nsyms++;
}


/*
 * S001
 * aux: create an auxillary symbol entry
 */
aux(auxent)
union auxent *auxent;
{

#ifdef DEBUG
	if(debug)
		fprintf(stderr,"AUX\n");
#endif

	fseek(symentfile, 0, 2);
	fwrite((char *)auxent, AUXESZ, 1, symentfile);

	return nsyms++;
}
/* End Enhanced Application Compatibility Support */
