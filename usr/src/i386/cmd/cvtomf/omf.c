/*		copyright	"%c%" 	*/

#ident	"@(#)cvtomf:omf.c	1.2"

/*
 * Portions Copyright 1980-1989 Microsoft Corporation Portions Copyright
 * 1983-1989 The Santa Cruz Operation, Inc All Rights Reserved 
 */
/*
 * Copyright (c) Altos Computer Systems, 1987 
 */
/* Enhanced Application Compatibility Support */
/*
 * S000	sco!jeffj	13 Nov 1988 - Added support to pass version control
 * strings to output coff object.  The coment() routine was modified to save
 * comments with the COM_EXESTR subtype.  The modend() routine was modified
 * to create the .comment scnhdr and data sections. 
 *
 * S001 sco!kai		1 May, 1989
 * - folded in seanf's changes for dynamically allocating syms[] and lines[]
 * array. Increased syms allocation unit size from 16 to 256 entires.
 *
 * S002 sco!kai		31 May, 1989
 * - pubdef(): strncmp needs to check for NULL as well to properly match
 * symbol names else "foo" will match "foobar"
 *
 * S003 sco!kai		20 Sep, 1989
 * - major rewrite of cvtomf tool. See comments below. Changes unmarked
 * because they are so comprehensive.
 *
 * S004	sco!kai		30 Sep, 1989
 * - have to process and store $$SYMBOLS fixups to deal with LOCALDATA symbols
 * in proc_sym. Note: this requires partial processing of LEDATA and FIXUPs
 * in PASS1. Unfortunately, there is currently redundant processing
 * but I don't really see a way around it: I need the symbols written before
 * I do fixups, but I need to process fixups before I can properly do symbols!
 * primarily to pick up THREADS in addition to $$SYMBOLS seg. fixups
 * also, allocate lines and symfix records in blocks of 64 instead of 16
 *
 * S005 sco!kai		10 Oct, 1989
 * - never produce additional .file records
 *
 * S006	sco!kai		27 Nov, 1989
 * - string() was always stripping a leading '_'; we don't want to do
 * this for lnames or theadr strings. parameterize string() to request
 * stripping or not.
 *
 * S007 sco!kai		13 Mar, 1990
 * - string() again: for LEXTDEF and LPUBDEF symbols, *don't* strip 
 * the leading underscore
 * - shared library bug motivated a small re-organization of the converter.
 * non-debug  cvtomf was putting out multiple symbols for extdef/pubdef
 * combinations. re-org makes omf basically a 1 pass operation again, with
 * symbols stored in a hashlist, output after all have been collected, and
 * fixup processing deferred until the complete omf file has been scanned once.
 * fixup offsets and context data is saved in a linked list which is then
 * processed at the end of omf()
 * NOTE: the long doc preceeding omf() was deleted because it was no longer
 * relavent. a shorter comment is now in place that reflects the current design.
 * - created "flushsyms" to be called from either omf() or process_syms()
 * to output any leftover symbols in the hash list
 * - deactivate all warnings about multiple theadrs. the problem is certainly
 * stil there, but it always was and the warnings seem to cause more grief than
 * not!
 */

#include	"cvtomf.h"
#include	"omf.h"
#include	"coff.h"
#include	"leaf.h"
#include	"symbol.h"

extern FILE *symentfile;	 /* COFF symbol table temp. file */

static FILE    *objfile;	/* object file */
static int      mods;		/* module count */

/* rectyp, use32, reclen, chksum: fields common to all OMF records */

static int      rectyp, use32, reclen, chksum;

/* lnms, lname: name list counter/array */

static int      lnms;
static char    *lname[MAXNAM];
static char    *comments[MAXCOM];
static int      ncomments, cmntsize;

/* segs, grps, exts: segment/group/external counters */

static int      segs, grps;
long            exts;

/* segment, group, external: arrays of symbol table indices */

static long     segment[MAXSCN];
static long     group[MAXGRP];
long            external[MAXEXT];

struct segdefn {
	int namindx;
	int clsindx;
	int ovrly;
	int scn;
} segindx[MAXNAM];

int             cursegindx;

/* dattype, datscn, datoffset: type/section/offset of last data record */

static int      dattype, datscn;
static long     datoffset;

/*
 * text_scn: text section #, comes from segdef(), used in handling LEXTDEF
 * records 
 */
int      text_scn, data_scn, bss_scn;

/* make sure that we read a modend record */
static int	modend_read;

/* 
 * for picking up $$SYMBOLS and $$TYPES data when -g
 * note: in case someone tries -g on ldr output, we have to worry
 * about multiple $$SYMBOLS and $$TYPES segments!
 */
static int	symbols_seg[6], types_seg[6];
static int	ssegidx, tsegidx;
int types_pres, symbols_pres, lines_pres;

/* save symbol index of aux record for section symbols; patched by coff() */
extern int scnauxidx[];

/* $$SYMBOLS tmpfile */
FILE *syms_tmp;

/* Hash queue for global symbols */
struct sym **hashhead;

static int omfpass;

/* OMF cannot have more than 32k line entries per module */
struct lines    *lines;
int             line_indx;

/* For storing $$SYMBOLS fixup records */

struct symfix *symfixes;		/* S004 */
int symfixidx = 0;			/* S004 */

#define		ALLOCSZ		(64)		/* must be power of 2 */
#define		ALLOCMSK	(ALLOCSZ - 1)

/*
 * open obfile, check for COFF format already; open $$SYMBOLS tmp 
 */
open_omf(filename)
char *filename;
{
	unsigned short  magic;

	objfile = fopen(filename, "r");

	if (!objfile){
		perror("fopen");
		fatal("Cannot open \"%s\" for reading", filename);
	}

	/* if already COFF, skip it */
	if (fread((char *)&magic, sizeof(magic), 1, objfile) != 1){
		perror("read");
		fatal("Cannot read \"%s\"", filename);	/* exit */
	}
	if (magic == MAGIC) {
		fclose(objfile);
		return(0);
	}
	rewind(objfile);

	/* store $$SYMBOLS ledata records in tmp file */
	if(do_debug){
		syms_tmp = tmpfile();
	}

	return(1);
}

/*
 * close omf objfile and $$SYMBOLS tmp file
 */
void
close_omf()
{
	(void)fclose(objfile);
	if(syms_tmp)
		(void)fclose(syms_tmp);
}



/*
 * comdef, coment, extdef, fixupp, grpdef, ledata, lheadr, lidata, linnum,
 * lnames, modend, pubdef, segdef, theadr: process an OMF record 
 */

static void
comdef(sclass)
int sclass;
{
	char           *name;
	int             type, segtype;
	long            value;

#ifdef DEBUG
	if(debug)
		fprintf(stderr," COMDEF\n");
#endif

	while (reclen > 1) {
		name = string(1);		/* S006 */
		type = index();
		segtype = byte();
		value = length();

		if (segtype == COMM_FAR)
			value *= length();

		if (++exts >= MAXEXT)
			fatal("Too many externals");	/* exit */

#ifdef DEBUG
		if (debug)
			fprintf(stderr, "comdef: %s, %d, %08lx\n", name, sclass, value);
#endif
		if (do_debug)
			addsym(name, value, S_COM, N_UNDEF, exts, type);
		else
			external[exts] = symbol(name, value, N_UNDEF, sclass, 0, 0);
	}
}

static void
coment()
{
	unsigned char   flags;
	unsigned char   class;
	char           *commp;

#ifdef DEBUG
	if(debug)
		fprintf(stderr," COMMENT\n");
#endif

	flags = byte();
	class = byte();
	if (class == COM_EXESTR) {
		if (ncomments >= MAXCOM)
			warning("More than %d comments: skipping exestr", MAXCOM);
		else {		/* reclen includes chksum, which is used as
				 * NULL */
			commp = malloc(reclen);
			if (commp) {
				int             tmp_reclen = reclen;
				block(commp, tmp_reclen - 1);	/* side effects on
								 * reclen */
				commp[tmp_reclen - 1] = '\0';
				comments[ncomments++] = commp;
				cmntsize += tmp_reclen;	/* want to include null
							 * in size */
				return;
			} else
				warning("Malloc failed, skipping comment record");
		}
	}
}

#define _ACRTUSED	"_acrtused"
#define __ACRTUSED	"__acrtused"

static void
extdef(sclass)
int sclass;
{
	char           *name;
	int             type;

#ifdef DEBUG
	if(debug)
		fprintf(stderr," EXTDEF\n");
#endif

	while (reclen > 1) {
		/* Begin S007 */
		if(sclass == C_STAT)
			name = string(0);	
		else
			name = string(1);		/* S006 */
		/* End S007 */
		type = index();

		if (++exts >= MAXEXT)
			fatal("Too many externals");	/* exit */

		if (!strcmp(name, _ACRTUSED) || !strcmp(name, __ACRTUSED)) {
			continue;	/* do nothing */
		} else if (sclass == (int) C_STAT) {
			/*
			 * Static text.  Pass along text section number, or
			 * AT&T linker will barf (it doesn't like getting
			 * relocation info for something of class static...).
			 * Skip the matching LPUBDEF rec that goes along with
			 * this LEXTDEF -- we already have the section #. 
			 *
			 * N.B. Assumes LEXTDEF/LPUBDEF recs only emitted for
			 * static text (not data). Cmerge group PROMISES this
			 * will be true forever and ever.  Note that these
			 * recs are only emitted for forward references; the
			 * compiler does self-relative relocation for
			 * backward references... 
			 */
#ifdef DEBUG
			if (debug)
				fprintf(stderr, "lextdef: %s, %d, %08lx\n", name, sclass, 0L);
#endif
			if(do_debug)
				addsym(name, 0, S_LEXT, N_UNDEF, exts, type);
			else{
				/* S007 */
				addsym(name, 0, S_LEXT, N_UNDEF, exts, type);
			}
		} else {
			if(do_debug)
				addsym(name, 0, S_EXT, N_UNDEF, exts, type);
			else
				/* S007 */
				addsym(name, 0, S_EXT, N_UNDEF, exts, type);
		}
	}
}


static long     target[4];	/* threads */

static void
fixupp()
{

	register int    i;
	unsigned char   c[3];
	unsigned char   fixdat;
	struct reloc    reloc[MAXREL];
	long            symndx, offset[MAXREL];

#ifdef DEBUG
	if(debug)
		fprintf(stderr," FIXUP\n");
#endif

	/* ignore fixup records for $$SYMBOLS debug ledata records */
	for(i = 0; i < ssegidx; i++)
		if(datscn == symbols_seg[i]){
			if(omfpass == PASS1 && do_debug)
				debugfixup();		/* S004 */
			return;
		}
	if(!do_debug && omfpass == PASS1)
		return;
	i = 0;

	while (reclen > 1) {
		block(c, 1L);

		if (TRD_THRED(c) == -1) {
			block(c + 1, 2L);

			if (i >= MAXREL)
				fatal("Too many relocation entries");

			if (dattype != LEDATA)
				fatal("Bad relocatable reference");	/* exit */

			reloc[i].r_vaddr = datoffset + LCT_OFFSET(c);

			if (!FIX_F(c))
				method(FIX_FRAME(c));

			if (!FIX_T(c))
				reloc[i].r_symndx = method(FIX_TARGT(c));
			else
				reloc[i].r_symndx = target[FIX_TARGT(c)];

			switch (LCT_LOC(c)) {

			case LOBYTE:
				if (LCT_M(c))
					reloc[i].r_type = R_OFF8;
				else
					reloc[i].r_type = R_PCRBYTE;

				break;

			case HIBYTE:
				if (!LCT_M(c))
					fatal("Bad relocation type");	/* exit */

				reloc[i].r_type = R_OFF16;
				break;

			case OFFSET16:
			case OFFSET16LD:
				if (LCT_M(c))
					reloc[i].r_type = R_DIR16;
				else
					reloc[i].r_type = R_PCRWORD;

				break;

			case OFFSET32:
			case OFFSET32LD:
				if (LCT_M(c))
					reloc[i].r_type = R_DIR32;
				else
					reloc[i].r_type = R_PCRLONG;

				break;

			case POINTER48:
				/*
				reloc[i].r_type = R_DIR32;
				*/
				if (LCT_M(c))
					reloc[i].r_type = R_DIR32;
				else
					reloc[i].r_type = R_PCRLONG;
				break;

			case BASE:
			case POINTER32:
				fatal("Segment reference in fixup record");	/* exit */

			default:
				fatal("Bad relocation type");	/* exit */

			}

			if (!FIX_P(c))
				offset[i] = use32 ? dword() : word();
			else
				offset[i] = 0;

			i++;
		} else {
			if (!TRD_D(c))
				target[TRD_THRED(c)] = method(TRD_METHOD(c));
			else
				method(TRD_METHOD(c));
		}
	}

	if(omfpass == PASS1)
		return;

	while (i-- > 0) {

#ifdef DEBUG
		if (debug && verbose)
			fprintf(stderr,"Calling relcation(section %d, vaddr %08lx, "
				"symndx %d, type %d, offset %d)\n",
				segindx[datscn].scn, reloc[i].r_vaddr, reloc[i].r_symndx, reloc[i].r_type,
				offset[i]);
#endif
		relocation(segindx[datscn].scn,
			   reloc[i].r_vaddr,
			   reloc[i].r_symndx,
			   reloc[i].r_type,
			   offset[i]);
	}
}

static void
grpdef()
{
	char           *name;
	int             scn, x;

#ifdef DEBUG
	if(debug)
		fprintf(stderr," GRPDEF\n");
#endif

	name = lname[index()];
	scn = N_ABS;

	while (reclen > 1) {
		x = byte();
		scn = index();

#ifdef NEVER
		/* OMF doc indicates that this value is not guarenteed to
		   be 0xff. We won't expect it to be either */
		if (x != 0xff)
			fatal("Bad group definition");	/* exit */
#endif
	}

	if (++grps >= MAXGRP)
		fatal("Too many groups");	/* exit */

	group[grps] = symbol(name, 0L, segindx[scn].scn, C_STAT, 0, 0);
}


static void
ledata()
{
	long            size;
	unsigned char   buffer[MAXDAT];
	int i;

#ifdef DEBUG
	if(debug)
		fprintf(stderr," LEDATA\n");
#endif

	dattype = LEDATA;
	datscn = index();
	datoffset = use32 ? dword() : word();

	size = reclen - 1;

	if (size > MAXDAT)
		fatal("Bad data record; too large");	/* exit */

	memset(buffer, 0, size);
	block(buffer, size);

	/* collect $$SYMBOLS and $$TYPES ledata records */
	for(i = 0; i< tsegidx; i++){
		if(datscn == types_seg[i]){
			types_pres++;
			if(do_debug)
				save_types(datoffset, buffer, size);
			return;
		}
	}
	for(i = 0; i< ssegidx; i++){
		if(datscn == symbols_seg[i]){
			symbols_pres++;
			if(do_debug)
				save_syms(datoffset, buffer, size);
			return;
		}
	}
	scndata(segindx[datscn].scn, datoffset, buffer, size);
}


static void
lheadr()
{

#ifdef DEBUG
	if(debug)
		fprintf(stderr," LHEADR\n");
#endif

	++mods;

#ifdef NEVER
	/* 
	 * this is commented out to permit  multiple .o files combined with
	 * ldr
	 */
	if (mods++)
		fatal("Too many modules");
#endif

}



static void
lidata()
{

#ifdef DEBUG
	if(debug)
		fprintf(stderr," LIDATA\n");
#endif

	dattype = LIDATA;
	datscn = index();
	datoffset = use32 ? dword() : word();

	expand(datoffset);
}



static void
linnum()
{
	unsigned short  lineno;
	unsigned long   offset;
	int             grpindx, segindex;

#ifdef DEBUG
	if(debug)
		fprintf(stderr," LINNUM\n");
#endif

	lines_pres++;

	grpindx = index();
	segindex = index();

	/* store lineno data in core until we process entire set */
	while (reclen > 1) {
		lineno = word();
		offset = use32 ? dword() : word();
		lines[line_indx].offset = offset;
		lines[line_indx].number = lineno;
		line_indx++;
		if ((line_indx & ALLOCMSK) == 0) {
			lines = realloc (lines, sizeof(*lines)*(line_indx+ALLOCSZ));
			if (lines == 0)
				fatal("linnum: Out of space");	
		}
	}
	return;
}



static void
lnames()
{
	char           *string();

#ifdef DEBUG
	if(debug)
		fprintf(stderr," LNAMES\n");
#endif

	while (reclen > 1) {
		if (++lnms >= MAXNAM)
			fatal("Too many names in LNAMES record");	/* exit */

		lname[lnms] = string(0);		/* S006 */
	}
}


void
upd_static_ext(name, value)
char *name;
long value;
{
	register struct sym *ptr;
	struct syment syment;

	ptr = findsym(name);
	if(ptr->type != S_LEXT)
		warning("upd_static_ext: findsym() returns non-LEXT symbol for %s",name);

#ifdef DEBUG
	if(debug)
		fprintf(stderr,"upd_static_ext: (kai) adding %d to symindx %d\n",value, ptr->offset / SYMESZ);
#endif

	fseek(symentfile, ptr->offset, 0);
	fread((char *)&syment, SYMESZ, 1, symentfile);
	syment.n_value += value;
	fseek(symentfile, -SYMESZ, 1);
	fwrite((char *)&syment, SYMESZ, 1, symentfile);
	fseek(symentfile, 0L, 2);
}


static void
lpubdef(sclass)
int sclass;
{
	int             grp, scn, frame, type;
	char           *name;
	long            value;

#ifdef DEBUG
	if(debug)
		fprintf(stderr," LPUBDEF\n");
#endif

	grp = index();
	scn = index();

	if (!scn) {
		scn = N_ABS;
		frame = word();
	}
	else
		scn = segindx[scn].scn;
	while (reclen > 1) {
		name = string(0);		/* S007 */
		value = use32 ? dword() : word();
		type = index();

		/*
		 * Update corresponding LEXTDEF symbol table entry's value
		 * field with the offset field from this LPUBDEF.  FIXUPP
		 * will then cause relocation() to do self-relative fixups
		 * for static functions. 
		 */
		if(do_debug)
			updatesym(name, value, S_LPUB, scn, type);
		else
			/* S007 */
			updatesym(name, value, S_LPUB, scn, type);
	}
}


static void
modend()
{
	int             scn;

#ifdef DEBUG
	if(debug)
		fprintf(stderr," MODEND\n");
#endif

	modend_read++;
	if(ncomments){
		scn = section(".comment", 0L, cmntsize, STYP_INFO);
		cmntdata(scn, 0L, comments, ncomments);
		for(scn = 0; scn < ncomments; scn++){
			free(comments[scn]);
			comments[scn] = (char *)0;
		}
	}
}



static void
pubdef(sclass)
int sclass;
{
	int             grp, scn, frame, type;
	char           *name;
	long            value;

#ifdef DEBUG
	if(debug)
		fprintf(stderr," PUBDEF\n");
#endif

	grp = index();
	scn = index();

	if (!scn) {
		scn = N_ABS;
		frame = word();
	}
	else
		scn = segindx[scn].scn;
	while (reclen > 1) {
		/* S007 - no change: note that lpubdefs never come here */
		name = string(1);		/* S006 */
		value = use32 ? dword() : word();
		type = index();

		if (++exts >= MAXEXT)
			fatal("Too many externals");	/* exit */

#ifdef DEBUG
		if (debug)
			fprintf(stderr, "pubdef: %s, %d, %08lx scn = %d\n", name, sclass, value, scn);
#endif
		if (do_debug)
			updatesym(name, value, S_PUB, scn, type);
		else
			/* S007 */
			updatesym(name, value, S_PUB, scn, type);
	}
}



static void
segdef()
{
	int             acbp, frame, offset, scn;
	long            size, flags;
	char           *name, *class, *overlay;
	int             temp;
	int             nam, cls, ovr;
	union		auxent auxent;
	int 		skipseg = 0;
	int		data_seg = 0;
	int		bss_seg = 0;

#ifdef DEBUG
	if(debug)
		fprintf(stderr," SEGDEF\n");
#endif

	if (++segs >= MAXSCN)
		fatal("Too many SEGDEF records");	/* exit */

	acbp = byte();

	if (!ACBP_A(acbp)) {
		frame = word();
		offset = word();
	}
	size = use32 ? dword() : word();

	nam = index();
	name = lname[nam];
	segindx[++cursegindx].namindx = nam;

	cls = index();
	class = lname[cls];
	segindx[cursegindx].clsindx = cls;

	/* remember $$SYMBOLS and $$TYPES seg numbers; ignore them otherwise */
	if(!strcmp(name, SYMBOLS_SEGNAME) &&
	   !strcmp(class, SYMBOLS_CLASS)){
		symbols_seg[ssegidx++] = cursegindx;
		skipseg++;
	}
	else if(!strcmp(name, TYPES_SEGNAME) &&
		!strcmp(class, TYPES_CLASS)){
		types_seg[tsegidx++] = cursegindx;
		skipseg++;
	}

	ovr = index();
	overlay = lname[ovr];
	segindx[cursegindx].ovrly = ovr;

	if (ACBP_B(acbp))
		fatal("Bad segment definition");	/* exit */

	if (!ACBP_P(acbp))
		warning("16-bit segment");

	/* this requires some thinking before just skipping */
	if(skipseg)
		return;

	if (ACBP_A(acbp)) {
		if (!strcmp(class, "CODE")) {
			name = _TEXT;
			flags = STYP_TEXT;
			lname[nam] = _TEXT;
		} else if (!strcmp(class, "DATA") || !strcmp(class, "CONST")) {
			if(!strcmp(class, "DATA"))
				data_seg = cursegindx;
			name = _DATA;
			lname[nam] = _DATA;
			flags = STYP_DATA;
		} else if (!strcmp(class, "BSS")) {
			bss_seg = cursegindx;
			name = _BSS;
			lname[nam] = _BSS;
			flags = STYP_BSS;
		} else
			flags = STYP_INFO;
	} else
		flags = STYP_INFO;

	segindx[cursegindx].scn = scn = section(name, 0L, size, flags);

	if ((flags & STYP_TEXT) == STYP_TEXT)
		text_scn = scn;	/* hold text section # for L*DEF recs */
	if(data_seg)
		data_scn = scn; 
	if(bss_seg)
		bss_scn = scn;

	segment[segs] = symbol(name, 0L, scn, C_STAT, 0, 1);

	/* create aux entry */
	memset((char *)&auxent, 0, sizeof(union auxent));
	auxent.x_scn.x_scnlen = size;
	scnauxidx[scn - 1] = aux(&auxent);
}



#define SEP	'/'

char *
basename(str)
register char *str;
{
	register char *base = str + strlen(str) - 1;

	while(*base == SEP)
		*base-- = '\0';
	while(base > str && *base != SEP)
		base--;
	if(*base == SEP)
		base++;
	return(base);
}


static void
theadr()
{
	static char 		*first_name = (char *)0;
	char           *f_name, *name;
	int 		len = 0;
	union auxent	auxent;

#ifdef DEBUG
	if(debug)
		fprintf(stderr," THEADR\n");
#endif

	mods++;
	f_name = string(0);		/* S006 */
	if(!first_name)
		first_name = f_name;

#ifdef DEBUG
	if (debug)
		fprintf(stderr, "File = %s\n", f_name);
#endif


	/* 
	 * .h files that define variables will cause THEADR records with
	 * "-g" and MSC. A subsequent THEADR re-defines the original .c file.
	 * This is a problem: currently pcc/sdb and MSC/codeview or x.out sdb
	 * are unable to deal with code or variables in a header file. We will
 	 * print a warning and avoid spitting out extra .file symbols.
	 *
	 * a second case of multiple THEADRs comes from /lib/ldr and multiple
	 * .o files. In this case, the symbolic debug data is currently 
	 * not coalesed properly so without some trickery we can't translated
	 * it properly. If we may concessions for an incorrect ldr then when
	 * it is fixed, we will be broken. Leave undone for now . . .
	 *
	 * multiple THEADRs will also be generated by #line directives that
	 * supply filenames. Good case of this is yacc output. Note: this
	 * also screws up line number entires!
   	 */
	if(mods > 1){ 
	   if(do_debug){
		if(strcmp(f_name, first_name) == 0)
			return;
		if(strcmp(f_name + (strlen(f_name) - 2),".h") == 0){
#ifdef NEVER
			warning("Symbolic debug information for code in \"%s\" is ineffective",f_name);
#endif
			return;
		}
		else{
#ifdef NEVER
			warning("Multiple modules in \"%s\": Symbolic debug information may be incorrect",first_name);
#endif

			return;
#ifdef NEVER
			/* 
			 * currently, I don't want to do this. It almost
			 * works but I don't want to modify the general logic
			 * to suit incorrect $$SYMBOLS and $$TYPES stuff from
			 * ldr!! If we fix ldr to suit the x.out ld, this will
			 * become broken.
			 */
			if(do_debug){
				process_typ();
				process_sym();
				typ_cleanup();
			}
#endif

		}
	   }
	   else
		return;
	}

	(void)symbol(".file", 0, N_DEBUG, C_FILE, 0, 1);

	/* .file aux entry 
	 *
	 * filenames are not like symbol names: up to 14 chars right in the
 	 * aux record. filenames are never placed in the strings table
  	 */

	memset((char *)&auxent, 0, AUXESZ);
	name = basename(f_name);
	while(*name && len < FILNMLEN)
		auxent.x_file.x_fname[len++] = *name++;
	(void)aux(&auxent);
}


/*
 * method: read a segment, group or external index, if necessary, and return
 * a symbol table index 
 */

long
method(x)
int x;
{
	int             idx;
	int	temp;

	switch (x) {

	case SEGMENT:
		idx = segment[index()];
#ifdef DEBUG
		if(debug)
			fprintf(stderr,"method = SEGMENT: coffidx = %d\n",idx);
#endif

		break;

	case GROUP:
		idx = group[index()];
		break;

	case EXTERNAL:
		temp = index();
		idx = external[temp];

#ifdef DEBUG
		if (debug && verbose)
			fprintf(stderr,"case EXTERNAL, idx=%d, temp=%d\n",idx, temp);
#endif
		break;

	case LOCATION:
	case TARGET:
		idx = -1;
		break;

	default:
		fatal("Bad method in FIXUP record");	/* exit */
	}

#ifdef DEBUG
	if (debug && verbose)
		fprintf(stderr,"method returning %d\n",idx);
#endif
	return (idx);
}


/*
 * expand: expand an iterated data block 
 */
long
expand(offset)
long offset;
{
	long            repcnt, blkcnt, filptr, i, size;
	unsigned char  *buffer[MAXDAT / 2];

	repcnt = use32 ? dword() : word();
	blkcnt = word();

	if (blkcnt) {
		filptr = ftell(objfile);

		while (repcnt-- > 0) {
			for (i = 0; i < blkcnt; i++)
				offset = expand(offset);

			if (repcnt && fseek(objfile, filptr, 0))
				fatal("Cannot expand iterated data");
		}
	} else {
		size = byte();

		if (size > MAXDAT / 2)
			fatal("Bad iterated data record; too large");

		block(buffer, size);

		while (repcnt-- > 0) {
			scndata(segindx[datscn].scn, offset, buffer, size);
			offset += size;
		}
	}

	return offset;
}


/*
 * recskip: skip an OMF record 
 *	    leave checksum byte in stream
 */
void
recskip()
{
	if(reclen > 1){
		if(fseek(objfile,(long)reclen - 1, 1)){
			perror("fseek");
			fatal("Bad seek on object file");
		}
		reclen = 1;
	}
}


/*
 * byte, word, dword: read a fixed-length field 
 */
byte()
{
	unsigned char   c;

	block(&c, 1L);
	return c;
}


word()
{
	unsigned char   c[2];

	block(c, 2L);
	return c[0] | (c[1] << 8);
}


long
dword()
{
	unsigned char   c[4];

	block(c, 4L);
	return c[0] | (c[1] << 8) | (c[2] << 16) | (c[3] << 24);
}


/*
 * index, length, string: read a variable-length field 
 */
index()
{
	unsigned char   c[2];
	register int    i;

	block(c, 1L);
	i = INDEX_BYTE(c);

	if (i == -1) {
		block(c + 1, 1L);
		i = INDEX_WORD(c);
	}
	return i;
}


long
length()
{
	unsigned char   c[4];

	block(c, 1L);

	switch (c[0]) {

	case LENGTH2:
		block(c, 2L);
		return c[0] | (c[1] << 8);

	case LENGTH3:
		block(c, 3L);
		return c[0] | (c[1] << 8) | (c[2] << 16);

	case LENGTH4:
		block(c, 4L);
		return c[0] | (c[1] << 8) | (c[2] << 16) | (c[3] << 24);

	default:
		return c[0];

	}
}


char *
string(strip)		/* S006 */
int strip;
{
	unsigned char   c;
	register char  *s;

	block(&c, 1L);
	s = malloc(c + 1);

	if (!s)
		fatal("Bad return from malloc: out of space");	/* exit */

	block((unsigned char *) s, (long) c);
	s[c] = '\0';

	if (strip && s[0] == '_')	/* S006 */
		s++;

#ifdef FP_SYMS
	if (!strcmp(s, FP_OMF))
		return FP_COFF;
#endif

	return s;
}


/*
 * block: read a block 
 *	  ** side-effect ** : update reclen
 */
block(buffer, size)
register unsigned char *buffer;
register long   size;
{
	if (fread(buffer, 1, (int) size, objfile) != size){
		perror("read");
		fatal("Bad read of object file");	/* exit */
	}
	reclen -= size;
}


/*
 * hash function for global symbols hash queue
 */
int
symhash(key)
register char *key;
{
	register unsigned int index = 0;

	while(*key)
		index += (index << 1)  + *key++;
	return(index % NBUCKETS);
}

void
addsym(name, offset, type, scn, ext, typ)
char *name;
long offset; 
int type, scn, ext, typ;
{
	register struct sym **list;
	register struct sym *ptr;

	
#ifdef DEBUG
	if(debug)
		fprintf(stderr,"addsym: %s\n",name);
#endif

	if((ptr = (struct sym *)malloc(sizeof(struct sym))) == (struct sym *)0)
		fatal("Bad return from malloc; not enough memory");
	memset(ptr, 0, sizeof(struct sym));
	ptr->name = name;
	ptr->offset = offset;
	ptr->type = type;
	ptr->scn = scn;
	ptr->typ = typ;
	ptr->ext = ext;
	list = &hashhead[symhash(name)];
	if(!*list)
		*list = ptr;
	else{
		ptr->next = *list;
		*list = ptr;
	}
}

struct sym *
findsym(name)
char *name;
{
	int index;
	register struct sym *ptr;

	ptr = hashhead[symhash(name)];
	while(ptr){
		if(!strcmp(ptr->name, name))
			break;
		ptr = ptr->next;
	}
	return(ptr);
}


#ifdef DEBUG
symstat()
{
	register struct sym *ptr;
	int bucket, total;
	int average = 0;
	int avcount = 0;
	int unused = 0;
	int max = 0;
	int totsyms = 0;

	fprintf(stderr,"\nHash Table Usage Statistics");
	for(bucket = 0; bucket < NBUCKETS; bucket++){
		total = 0;
		
		ptr = hashhead[bucket];
		while(ptr){
			total++;
			ptr = ptr->next;
		}
		totsyms += total;
		if(total > max)
			max = total;
		if(!total)
			unused++;
		else{
			average += total;
			avcount++;
		}
	}
	fprintf(stderr,"\ntotal symbols %d\n",totsyms);
	if(avcount)
		fprintf(stderr,"\naverage\t%.1f\n",average / (float)avcount);
	fprintf(stderr,"unused \t%d (%%%.1f)\n",unused, (unused / (float)NBUCKETS) * 100);
	fprintf(stderr,"max    \t%d\n", max);
}
#endif

void
updatesym(name, offset, type, scn, typ)
char *name;
long offset;
int type,scn,typ;
{
	register struct sym *ptr;

	ptr = findsym(name);
	if(ptr){
		ptr->offset = offset;
		ptr->type = type;
		ptr->scn = scn;
		ptr->typ = typ;
	}
	else
		addsym(name, offset, type, scn, exts, typ);
}

syminit()
{
	if(!hashhead)
		if((hashhead = (struct sym **)malloc(NBUCKETS * sizeof(struct sym *))) == (struct sym **)0)
			fatal("Bad return from malloc: not enough memory");
	memset(hashhead, 0, NBUCKETS * sizeof(struct sym *));
}



/* 
 *	omf: scan through omf file, processing each record as appropriate
 *
 *	most symbols are loaded into a hash table and not output into COFF
 *	symbols until the entire file has been scanned. fixup record processing
 *	is deferred until after all symbols have been output. Note that some
 *	fixup processing must happen when "do_debug" in order to support
 *	$$SYMBOLS fixup processing for process_sym needs.
 */
void
omf(pass)
int pass;
{

	mods = lnms = segs = grps = exts = 0;

	omfpass = pass;

#ifdef DEBUG
	if(debug)
		fprintf(stderr,"\n*** omf() PASS#%d ***\n\n", omfpass);
#endif

	modend_read = 0;

	/* initialze core storage for external/public symbols */
	syminit();

	/* init core storage for line number entires and $$SYMBOLS fixups */
	if(do_debug){
		if((lines = (struct lines *)malloc(sizeof(struct lines) * ALLOCSZ)) 
					== (struct lines *)NULL){
			fatal("Bad return from malloc: not enough memory to process LINNUM records");
		}
		line_indx = 0;
		if((symfixes = (struct symfix *)malloc(sizeof(struct symfix) * ALLOCSZ)) 
					== (struct symfix *)NULL){
			fatal("Bad return from malloc: not enough memory to process $$SYMBOLS FIXUP records");
		}
		symfixidx = 0;
	}

	/* zero out exts[] table for multiple object files */
	(void)memset(external, 0, MAXEXT * sizeof(long));
	types_pres = lines_pres = symbols_pres = 0;
	text_scn = data_scn = bss_scn = 0;
	cursegindx = ncomments = cmntsize = 0;
	ssegidx = tsegidx = 0;
	symbols_seg[0] = types_seg[0] = 0;

	/* process OMF records */
	(void)rewind(objfile);
	while ((rectyp = getc(objfile)) != EOF) {

		if(modend_read)
			warning("OMF records past MODEND record");

		use32 = USE32(rectyp);
		reclen = word();

		switch (RECTYP(rectyp)) {

			case EXTDEF:
				extdef((int) C_EXT);
				break;

			case FIXUPP:
			case FIXUP2:
				/* deferr until all symbols are read */
				save_fixupp(ftell(objfile) - 3);
				fixupp();
				break;

			case PUBDEF:
				pubdef((int) C_EXT);
				break;

			case LEDATA:
				ledata();
				break;

			case LIDATA:
				lidata();
				break;

			case COMDEF:
				comdef((int) C_EXT);
				break;

			case LEXTDEF:
				extdef((int) C_STAT);
				break;

			case LPUBDEF:
				lpubdef((int) C_STAT);
				break;

			case COMENT:
				coment();
				break;

			case SEGDEF:
				segdef();
				break;

			case LINNUM:
				if(do_debug)
					linnum();
				break;

			case GRPDEF:
				grpdef();
				break;

			case THEADR:
				theadr();
				break;

			case MODEND:
				modend();
				break;

			case LNAMES:
				lnames();
				break;

			case LCOMDEF:
				comdef((int) C_STAT);
				break;

			case LHEADR:
				lheadr();
				break;

			default:

#ifdef DEBUG
				if (debug)
					fprintf(stderr, "Unknown record type!: 0x%02xs\n", rectyp);
#endif
				fatal("Unknown or bad record type %x",RECTYP(rectyp));
				break;
		}

		/* skip over remaining portion of record */
		recskip();
		chksum = byte();
	}

	if(!modend_read)
		warning("Missing MODEND record");

#ifdef DEBUG
	if(debug)
		if(do_debug && omfpass == PASS1)
			symstat();
#endif


	if(do_debug){
		process_typ();
		process_sym();
		typ_cleanup();
	}
	else
		flush_syms();
	omfpass = PASS2;
	proc_fixups();
}

/* Begin S004 */

/*
 * store a subset of fixup data for all $$SYMBOLS fixups. This is used
 * by proc_sym for analyzing LOCALDATA symbols and recognizing statics.
 */
static 
debugfixup()
{
	long soff, idx;
	int ex = 0, scn = 0;
	unsigned char c[3];


#ifdef DEBUG
	if(debug)
		fprintf(stderr,"debugsym: $$SYMBOLS FIXUP\n");
#endif

	while (reclen > 1) {
		ex = scn = 0;
		block(c, 1L);
		block(c + 1, 2L);
		soff = datoffset + LCT_OFFSET(c);
		if (!FIX_T(c)){
			switch(FIX_TARGT(c)){
			case SEGMENT:
				scn = segindx[index()].scn;
				break;
			case GROUP:
				(void)index();
				scn = data_scn;
				break;
			case EXTERNAL:
				(void)index();
				ex++;
				break;
			default:

#ifdef DEBUG
				if(debug)
					fprintf(stderr,"WARNING: Unknown target type for $$SYMBOLS fixup\n");
#endif

				break;
			}
		}
		else{
			register int i;

			scn = -1;
			idx= target[FIX_TARGT(c)];
			for(i = 0; i < cursegindx; i++)
				if(segment[i] == idx){
					scn = segindx[i].scn;
					break;
				}
			if(scn < 0)
				for(i = 0; i < grps; i++)
					if(group[i] == idx){
						scn = data_scn; 
						break;
					}
		}
		symfixes[symfixidx].offset = soff;
		symfixes[symfixidx].ext = ex;
		symfixes[symfixidx].scn = scn;

		if ((++symfixidx & ALLOCMSK) == 0) {
			symfixes = realloc(symfixes, sizeof(struct symfix)*(symfixidx+ALLOCSZ));
			if (symfixes == 0) 
				fatal("debugfixup: Out of memory");
		}

#ifdef DEBUG 
		if(debug){
			fprintf(stderr,"offset = 0x%08x\t",soff);
			if(ex)
				fprintf(stderr,"external\n");
			else
				fprintf(stderr,"static: scn = 0x%x\n",scn);
		}
#endif

	}
}

/*
 *	locate $$SYMBOLS fixup data for fixup applied at offset 
 */
struct symfix *
findsfix(offset)
long offset;
{
	register int i = 0;
	register int j = symfixidx - 1;
	int indx;
	struct symfix *ptr;

	/* binary search */
	while(i <= j){
		indx = (i + j)/2;
		ptr = &symfixes[indx];
		if(ptr->offset == offset)
			return(ptr);
		else if (ptr->offset > offset)
			j = indx - 1;
		else
			i = indx + 1;
	}
	return((struct symfix *)0);
}

/* End S004 */

/* Begin S007 */
struct sfix {
	unsigned long offset, datoffset, datscn, dattype;
	struct sfix *next;
};

struct sfix *fixlist, *fixlast;

save_fixupp(offset)
long offset;
{
	register struct sfix *new;

	if(!(new = malloc(sizeof(struct sfix))))
		fatal("save_fixup: Out of memory");
	new->offset = offset;
	new->datoffset = datoffset;
	new->datscn = datscn;
	new->dattype = dattype;
	new->next = (struct sfix *)0;
	if(!fixlast)
		fixlist = fixlast = new;
	else{
		fixlast->next = new;
		fixlast = new;
	}
}

proc_fixups()
{
	register struct sfix *f = fixlist;
	register struct sfix *p = fixlist;

	while(f){
		/* prepare the environment for fixupp() */
		fseek(objfile,f->offset,0);
		rectyp = getc(objfile);
		if(RECTYP(rectyp) != FIXUPP && RECTYP(rectyp) != FIXUP2)
			fatal("proc_fixups: not a fixup record");
		use32 = USE32(rectyp);
		reclen = word();
		datscn = f->datscn;
		dattype = f->dattype;
		datoffset = f->datoffset;
		fixupp();
		p = f;
		f = f->next;
		free(p);
	}
	fixlist = fixlast = (struct sfix *)0;
}

/* 
 * process all remaining EXTDEF, PUBDEF, and COMDEFS not in $$SYMBOLS 
 * output all remaining symbols in hash list
 * note: type should be T_NULL for any EXTDEF or LEXTDEF only symbols
 * free dynamic storage
 */
flush_syms()
{
	register struct sym *sptr, *fptr;
	int count, useaux, cofftype;
	union auxent auxent;
	int class;

	cofftype = T_NULL;
	useaux = 0;
	for(count = 0; count < NBUCKETS; count++){
		sptr = hashhead[count];
		while(sptr){
			if(!external[sptr->ext]){
				if(do_debug)
					useaux = omf2coff_typ(sptr->typ, &cofftype, &auxent);
				/* don't output type data if EXTDEF only */
				if(sptr->type == S_EXT){
					useaux = 0;
					cofftype = T_NULL;
				}
				switch(sptr->type){
				default:
					class = C_EXT;
					break;
				case S_LEXT:
				case S_LPUB:
					class = C_STAT;
					break;
				}
				external[sptr->ext] = symbol(sptr->name, 
							     sptr->offset, 
						             sptr->scn, class, 
						             cofftype, useaux);
				if(useaux)
					(void)aux(&auxent);
			}
			fptr = sptr;
			sptr = sptr->next;
			free(fptr);
		}
	}
}
/* End S007 */
/* End Enhanced Application Compatibility Support */
