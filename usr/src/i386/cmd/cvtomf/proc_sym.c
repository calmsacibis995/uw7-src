/*		copyright	"%c%" 	*/

#ident	"@(#)cvtomf:proc_sym.c	1.2"

/*
 *	Copyright (C) The Santa Cruz Operation, 1984-1988, 1989.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/* Enhanced Application Compatibility Support */
/*	MODIFICATION HISTORY
 *
 *	S000	sco!kai		Sep 30, 1989
 *	- have to process $$SYMBOLS fixups to correctly deal with LOCALDATA syms
 *	S001	sco!kai		Oct 3,  1989
 *	- o2creg() was incorrect: always returned ESI for BX, AX, CX and DX!
 *	S002	sco!kai		Mar 13, 1990
 *	- moved code that flushes symbols from hash list into a function in
 *	omf.c so it can be shared with omf() for non-debug conversions
 *	S003	sco!kai		Mar 30, 1990
 *	- compiler bug for "const arrays etc.": $$SYMBOLS fixup's sometimes
 *	seem to generate a segment relative fixup (static) with a bad segment
 *	index. these should be skipped.
 */

/* BEGIN SCO_DEVSYS */

#include "cvtomf.h"
#include "omf.h"
#include "symbol.h"
#include "leaf.h"

extern FILE *symentfile;

extern FILE *syms_tmp;
extern int symbols_pres;
extern int text_scn;
extern int data_scn;
extern int bss_scn;

extern int external[];

extern int line_indx;

static unsigned char buffer[S_BUFSIZE];

/*
 * Note: about omf and COFF line number entires
 * COFF line number entries are relative to the opening curley brace of
 * a function definition, i.e. line 1 = line with "{". OMF line numbers
 * are absolute file line numbers. The translation is somewhat inexact
 * so that tools like "list" sometimes produce error messages and strange
 * output on cvtomf COFF binaries. This could be adjusted here except that
 * conversions from COFF back to x.out would then be most likely incorrect.
 * sdb/codeview breakpoints and single stepping function properly nonetheless 
 */

#define LINADJ	1
#define O2CLINE(x)	((x) - pstrtln + LINADJ)

extern struct symfix *symfixes;
extern int symfixidx;

/*
 * comparision function for qsort'ing $$SYMBOLS fixups by offset - S000
 */
sfixcmp(p1,p2)
register struct symfix *p1, *p2;
{
	return(p1->offset - p2->offset);
}

/*
 *	process $$SYMBOLS data: function data (.bf, .ef, .bb, .eb),
 *	statics, locals, args and parameters.
 *	responsable for outputing all remaining symbols.
 *	line number entires are also processed here
 */
void
process_sym()
{
	struct sym *sptr, *fptr;
	int type, count, sclass, scn, symidx;
	char name[80];
	union auxent auxent;
	int blocklev = 0;
	int cofftype, useaux;
	int procscn, pblockoff, poffset, pstrtln;
	struct lines *lptr = lines;
	struct lines *elptr = &lines[line_indx];
	long fileoff;		/* S000 */

	/* the following supports a fixed max nesting levels for blocks */
	int bbpatch[MAX_BLOCK_NEST], bbidx = -1;
	int eboffset[MAX_BLOCK_NEST], ebidx = -1;
	
	
#ifdef DEBUG
	if(debug){
		fprintf(stderr,"\n*** process_sym() ***\n\n");
	}
#endif


#ifdef DEBUG
	if(debug){
		register struct sym *ptr;
		register struct lines *lptr = lines;
		int b;

		fprintf(stderr,"Dump of symbols hash list\n");
		for(b = 0; b < NBUCKETS; b++){
			ptr = hashhead[b];
			while(ptr){
				fprintf(stderr,"%-20s\t0x%08x\tscn = %d\t",ptr->name,
					ptr->offset, ptr->scn);
				switch(ptr->type){
				case S_EXT:
					fprintf(stderr,"S_EXT");
					break;
				case S_LEXT:
					fprintf(stderr,"S_LEXT");
					break;
				case S_PUB:
					fprintf(stderr,"S_PUB");
					break;
				case S_LPUB:
					fprintf(stderr,"S_LPUB");
					break;
				case S_COM:
					fprintf(stderr,"S_COM");
					break;
				}
				fputc('\n',stderr);
				ptr = ptr->next;
			}
		}
		fprintf(stderr,"\nLine Numbers\n");
		while(lptr < &lines[line_indx]){
			fprintf(stderr,"%5d 0x%08x\n",lptr->number, lptr->offset);
			lptr++;
		}
		fprintf(stderr,"\n");	
	}
#endif

	pblockoff = poffset = pstrtln = 0;
	procscn = text_scn;


	/* sort $$SYMBOLS fixups to facilitate binary search  - S000 */
	qsort((char *)symfixes, symfixidx, sizeof(struct symfix), sfixcmp);

#ifdef DEBUG
	if(debug){
		struct symfix *ptr;

		fprintf(stderr,"Dump of $$SYMBOLS Fixup Data:\n\n");
		for(ptr = symfixes; ptr < &symfixes[symfixidx]; ptr++)
			fprintf(stderr,"offset = 0x%08x\text = %d\tscn = 0x%08x\n",
				ptr->offset,
				ptr->ext,
				ptr->scn);
		fprintf(stderr,"\n");

	}
#endif

	/* process $$SYMBOLS record */

	fseek(syms_tmp, 0L, 0);
	while ((count = fgetc(syms_tmp)) != EOF){
		fileoff = ftell(syms_tmp) - 1;		/* S000 */
		type = fgetc(syms_tmp);
		if(count > S_BUFSIZE){
			warning("Unexpected: $$SYMBOLS record larger than %d bytes",S_BUFSIZE);
			count = S_BUFSIZE;
		}
		fread(buffer, count - 1, 1, syms_tmp);
		memset((char *)&auxent, 0, AUXESZ);
		name[0] = '\0';

		switch(type){
		case S_BLOCKSTART:
		{
			register struct bsr *bsr = (struct bsr *)buffer;

#ifdef DEBUG
			if(debug)
				fprintf(stderr,"BLOCK START\n");
#endif

			/* 
			 * the following tests for the standard BLOCKSTART
			 * that we have already created (i.e. ".bf"). 
			 */
			if(blocklev == 1 && bsr->offset == pblockoff )
				break;

			/* new block; create ".bb" and aux record */
			blocklev++;
			symbol(".bb", bsr->offset + poffset, 
				procscn, C_BLOCK, T_NULL, 1);

			/* line number processing */
			while(lptr < elptr && lptr->offset < bsr->offset){
				line(procscn, lptr->offset, O2CLINE(lptr->number));
				lptr++;
			}
			if(lptr < elptr)
			      auxent.x_sym.x_misc.x_lnsz.x_lnno = O2CLINE(lptr->number);

			/* 
     			 * ".bb" aux record
			 * save aux offset for patching;
			 * calculate offset that goes with omf end record
			 */
			bbpatch[++bbidx] = aux(auxent);
			eboffset[++ebidx] = bsr->offset + poffset + bsr->length;
			
			break;
		}

		case S_PROCSTART:
		{
			register struct psr *psr = (struct psr *)buffer;
#ifdef NEVER
			register struct lines *lptr2;
#endif 
		

#ifdef DEBUG
			if(debug)
				fprintf(stderr,"PROCEDURE START\n");
#endif

			blocklev = 1;
			strncpy(name, buffer+sizeof(struct psr), psr->name_len);
			name[psr->name_len] = '\0';
			procscn = scn = text_scn;
			sclass = C_EXT;
			sptr = findsym(name);
			if(sptr){
				if(sptr->type == S_PUB)
					sclass = C_EXT;
				else
					sclass = C_STAT;
				psr->offset = sptr->offset;
				scn = sptr->scn;
			}
			else
				warning("Function from $$SYMBOLS not found: %s",name);
			procscn = scn;	/* save for .bf and .ef */

			/* create function and aux record */
			useaux = omf2coff_typ(psr->type, &cofftype, &auxent);
			symidx = symbol(name, psr->offset, scn, 
				        sclass, cofftype, 1);
			if(sptr)
				external[sptr->ext] = symidx; 
			auxent.x_sym.x_misc.x_fsize = psr->length;
			bbpatch[++bbidx] = aux(&auxent); /* save for patching */
		
			/* create ".bf" symbol and aux record */
			symbol(".bf", psr->d_start, procscn, C_FCN, 0, 1);
			memset((char *)&auxent, 0, AUXESZ);

			pblockoff = psr->d_start - psr->offset;
			eboffset[++ebidx] = psr->d_end;

			/* line number processing */

			/* purge any lines from previous function */
			while(lptr < elptr && lptr->offset < psr->offset){
				line(procscn, lptr->offset, O2CLINE(lptr->number));
				lptr++;
			}
			line(procscn,symidx,0);
			poffset = psr->offset;
			if(lptr < elptr)
				pstrtln = lptr->number;
#ifdef NEVER
			for(lptr2 = lptr; 
			    (lptr2 < elptr) && 
                            (lptr2->offset) < (psr->offset + psr->length); 
			    lptr2++)
				if(lptr2->number < pstrtln)
					pstrtln = lptr2->number;
#endif

			/* ".bf" aux entry */
			if(lptr < elptr)
			      auxent.x_sym.x_misc.x_lnsz.x_lnno = pstrtln; 
			aux(&auxent);
			break;
		}

		case S_BLOCKEND:
		{
			int offset,next;

#ifdef DEBUG
			if(debug)
				fprintf(stderr,"BLOCK END\n");
#endif

			if(!blocklev)
				break;
			blocklev--;
			offset = eboffset[ebidx--];

			/* ".ef" or ".eb" symbol */
			if(!blocklev)	/* end of proc */
				(void)symbol(".ef",offset,
					     procscn,C_FCN,T_NULL,1);
			else	/* end of block */
				(void)symbol(".eb",offset,
					      procscn,C_BLOCK,T_NULL,1);

			/* line number processing */
			while(lptr < elptr && lptr->offset < offset){
				line(procscn, lptr->offset, O2CLINE(lptr->number));
				lptr++;
			}

			/* ".eb" or ".ef" aux record */
			if(lptr < elptr)
			      auxent.x_sym.x_misc.x_lnsz.x_lnno = O2CLINE(lptr->number);
			next = aux(&auxent);

			/* need to patch in endndx field of .bf or .bb */
			fseek(symentfile, (long)bbpatch[bbidx--] * SYMESZ, 0);
			fread((char *)&auxent, AUXESZ, 1, symentfile);
			auxent.x_sym.x_fcnary.x_fcn.x_endndx = ++next;
			fseek(symentfile, -SYMESZ, 1);
			fwrite((char *)&auxent, AUXESZ, 1, symentfile);
			fseek(symentfile, 0L, 2);

			break;
		}

		case S_BPREL:
		{
			register struct bpr *bpr = (struct bpr *)buffer;
			int sclass;	

#ifdef DEBUG
			if(debug)
				fprintf(stderr,"BP RELATIVE\n");
#endif

			strncpy(name, buffer+sizeof(struct bpr), bpr->name_len);
			name[bpr->name_len] = '\0';
			useaux = omf2coff_typ(bpr->type, &cofftype, &auxent);
			sclass = (bpr->offset >= 0) ? C_ARG : C_AUTO;

			/* 
			 * $$SYMBOLS data has two seperate records for a
		   	 * register parameter, 1st, BP REL, then REGISTER. 
			 * COFF has a single "regparm" type. Look ahead for
			 * all BP rels
			 */
			if(sclass == C_ARG){
				unsigned char buffer[S_BUFSIZE];
				long offset = ftell(syms_tmp);
				int count;

				count = fgetc(syms_tmp);
				type = fgetc(syms_tmp);
				if(type == S_REGISTER){
					register struct rsr *rsr;
					char *name2;

					fread(buffer, --count, 1, syms_tmp);
					rsr = (struct rsr *)buffer;
					name2 = (char *)(buffer+sizeof(struct rsr));
					*(name2 + rsr->name_len) = '\0';
					if(!strcmp(name, name2)){
						sclass = C_REGPARM;
						bpr->offset = o2creg(rsr->reg);
					} 
					else
						fseek(syms_tmp, offset, 0);
				}
				else
					fseek(syms_tmp, offset, 0);
			}
			(void)symbol(name,bpr->offset, N_ABS, sclass,
				     cofftype, useaux);
			if(useaux)
				(void)aux(&auxent);
		

			break;
		}

		/*
 		 * determining which local data symbols are statics requires
 		 * the corresponding $$SYMBOLS segment fixup record. We examine
		 * the fixup to determine:
		 *	1) if the fixup references an EXTDEF (not static)
		 *	2) if static, which section, BSS or DATA
		 * S000
		 */
		case S_LOCALDATA:
		{
			register struct ldr *ldr = (struct ldr *)buffer;
			struct symfix *fixptr;
#ifdef DEBUG
			if(debug)
				fprintf(stderr,"LOCAL DATA\n");
#endif
			strncpy(name, buffer+sizeof(struct ldr), ldr->name_len);
			name[ldr->name_len] = '\0';
			useaux = omf2coff_typ(ldr->type, &cofftype, &auxent);

			/* Begin S000 */
#ifdef DEBUG
			if(debug)
				fprintf(stderr,"FIXUP offset should be 0x%08x\n",fileoff + 2);
#endif
			sptr = (struct sym *)0;
			if((fixptr = findsfix(fileoff + 2)) == (struct symfix *)0)
				warning("Missing FIXUP for $$SYMBOLS Local: %s",name);
			if(fixptr && !fixptr->ext){	/* static */
				sclass = C_STAT;
				scn = fixptr->scn;
			}
			else{
				if(blocklev)	/* COFF puts these at end */
					break;
				sptr = findsym(name);

				/* if no EXTDEF, COMDEF or PUBDEF, static */
				/* should never happen! */
				if(!sptr){
					sclass = C_STAT;
					scn = data_scn;	/* ??? */
				}	
				else{
					sclass = C_EXT;
					/* don't output type data if EXTDEF only */
					if(sptr->type == S_EXT){
						useaux = 0;
						cofftype = T_NULL;	
					}
					ldr->offset = sptr->offset;
					scn = sptr->scn;
				}
			}

			/* End S000 */

			/* S003 work around compiler bug */
			if(sclass == C_STAT && !scn)
				break;
			/* S003 */
			symidx = symbol(name,ldr->offset, 
				     (scn) ? scn : N_UNDEF,
				     sclass, cofftype, useaux);
			if(useaux)
				(void)aux(&auxent);
			if(sptr)
				external[sptr->ext] = symidx;
			break;
		}

		case S_CODELABEL:
		{
			register struct clr *clr = (struct clr  *)buffer;

#ifdef DEBUG
			if(debug)
				fprintf(stderr,"CODE LABEL\n");
#endif

			strncpy(name, buffer+sizeof(struct clr), clr->name_len);
			name[clr->name_len] = '\0';

			/* ignore near/far for now; 386 code only */
			(void)symbol(name, clr->offset, text_scn,
				     C_LABEL, T_NULL, 0);
			break;
		}

		case S_WITHSTART:
		{
			/* not currently generated by MSC */

#ifdef DEBUG
			if(debug)
				fprintf(stderr,"WITHSTART\n");
#endif

			break;
		}

		case S_REGISTER:
		{
			register struct rsr *rsr = (struct rsr *)buffer;
			int coffreg = 0;

#ifdef DEBUG
			if(debug)
				fprintf(stderr,"REGISTER VARIABLE\n");
#endif

			strncpy(name, buffer+sizeof(struct rsr), rsr->name_len);
			name[rsr->name_len] = '\0';
			coffreg = o2creg(rsr->reg);
			useaux = omf2coff_typ(rsr->type, &cofftype, &auxent);
			(void)symbol(name, coffreg, N_ABS,
				     C_REG, cofftype, useaux);
			if(useaux)
				(void)aux(&auxent);
			break;
		}

		case S_CONSTANT:
		{
			/* not currently generated by MSC */

#ifdef DEBUG
			if(debug)
				fprintf(stderr,"CONSTANT\n");
#endif

			break;
		}

		default:

#ifdef DEBUG
			if(debug)
				fprintf(stderr,"UNKNOWN TYPE!!\n");
#endif

			warning("Unknown $$SYMBOLS record type");
			break;
		}	/* switch */
	}  /* while */

	/* any remaining line numbers belong to the last function */
	while(lptr < elptr){ 
		line(procscn, lptr->offset, O2CLINE(lptr->number));
		lptr++;
	}
	free(lines);

#ifdef NEVER
	/* well, ldr does not deal properly with symbolic debug data
	   cvtomf will not deal properly with ldr files for now
	 */
	/* prepare for multple THEADRs e.g. ldr */
	if((lines = (struct lines *)malloc(sizeof(struct lines) * 16)) 
				== (struct lines *)NULL){
		warning("Bad return from malloc: not enough memory to process line number records");
	}
	line_indx = 0;
#endif

	flush_syms();	/* S002 */
	free(symfixes);
}

/*
 * save $$SYMBOLS ledata records in tmp file
 */
void
save_syms(offset, buffer, size)
int offset, size;
char *buffer;
{
	fseek(syms_tmp, offset, 0);
	fwrite(buffer, size, 1, syms_tmp);
}

/*
 * map omf register number to COFF register number (I386 register set)
 */
static
o2creg(omfreg)
register int omfreg;
{
	switch(omfreg){
	case DL_REG_DI:
		return C_EDI;
	case DL_REG_SI:
		return C_ESI;
	case DL_REG_BX:
		return C_EBX;
	case DL_REG_AX:
		return C_EAX;
	case DL_REG_DX:
		return C_EDX;
	case DL_REG_CX:
		return C_ECX;
	default:
		warning("Unknown OMF register number: %d", omfreg);
		return C_INVALID;
	}
}

/* END SCO_DEVSYS */
/* End Enhanced Application Compatibility Support */
