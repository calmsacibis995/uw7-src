/*		copyright	"%c%" 	*/

#ident	"@(#)cvtomf:proc_typ.c	1.2"

/*
 *	Copyright (C) The Santa Cruz Operation, 1984-1988, 1989.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/* Enhanced Application Compatibility Support */
/*	MODIFICATION HISTORY
 *
 *	S000	sco!kai		Oct 2, 1989
 *	- bitfield offsets were not computed properly: need the BITFIELD offset
 *	(bit displacement) and the structure field offset * 8
 *	S001	sco!kai		Mar 30, 1990
 *	- MSC permits the following kind of contruct:  "(*foo)[]". The code
 *	shows that this is treated as "*foo" but the symbolic debug records
 *	say "POINTER to ARRAY" which rcc does not allow, nor is there a way
 *	to express this in COFF symbolic debug format. If we encount this
 *	skip over the array to get to the next type. Currently, the "size"
 *	field will be 0
 *	

/* BEGIN SCO_DEVSYS */

/*
 *	Known diversions from "true" COFF format 
 *
 * 	1) stucture, union and enumeration templates that are not used
 *	   in variable declarations *will* still have symbol records in
 *	   pcc COFF files. MSC produces no $$TYPES or $$SYMBOLS data in
 *	   these cases.
 *	   Insufficient data from the compiler.
 *
 *	2) "typedefs" produce symbol records, even though tag indices in 
 *	   symbol aux records never seem to reference them. MSC does not
 *	   produce $$TYPES records for "typedef" statements.
 *	   Insufficient data from the compiler.
 *
 *	3) "struct UNKNOWN *ptr" - pcc will produce the type "*struct" even
 *	   though it supplies no tag index or aux record. OMF $$TYPES data
 *	   defines this as "*null". 
 *	   Insufficient data from the compiler.
 *
 *	4) Currently, a bug in MSC: $$TYPES info for arrays of unsigned types:
 *	   the element type is given as signed rather than unsigned. 
 *	   Incorrect compiler data.
 *
 *	5) variables local to a block (other than main function block) seem to
 *	   have an auc record in COFF, perhaps giving the line number of the
 *	   variable declaration?  Unavailable information from MSC $$TYPES
 *	   or $$SYMBOLS. Not clear on this, COFF doc does not discuss this.
 *
 *	6) arrays with large indices (e.g. > 64K) do not seem to always get 
 *	   the proper size in the COMDEF record. This needs investigation . . .
 *	   Incorrect compiler data. 
 */

#include "cvtomf.h"
#include "omf.h"
#include "symbol.h"
#include "leaf.h"

/* map node: COFF intermediate type translated from OMF */
struct typ_info {
	unsigned short ctype;		/* COFF type */
	int size;
	union {
		union auxent *aptr;
		long tagndx;
	}aux;
};

static struct typ_info *o2cmap;		/* map for OMF to COFF types */
static unsigned char *otypbuf;		/* $$TYPES data from omf file */
static unsigned int otypbufsize;
static unsigned char **omftypes;	/* array of OMF index pointers */
static unsigned int  omfmaxidx;

/*
 *  extract number from numeric leaf; advance pointer
 */
static long
numleaf(ptr)
register char **ptr;
{
	unsigned char t;
	long value = 0;

	if((**ptr & 0x80) == 0)
		return((long)(*(unsigned char *)(*ptr)++));
	switch((t = *(unsigned char *)(*ptr)++)){
	case U16:
		value = (long)*(unsigned short *)*ptr;
		*ptr +=2;
		break;
	case U32:
		value = (long)*(unsigned long *)*ptr;
		*ptr +=4;
		break;
	case S8:
		value = (long)*(char *)(*ptr)++;
		break;
	case S16:
		value = (long)*(short *)*ptr;
		*ptr +=2;
		break;
	case S32:
		value = (long)*(long *)*ptr;
		*ptr +=4;
		break;
	default:
		break;
	}
	return(value);

}

/*
 * extract index value from index leaf; advance pointer
 */
static unsigned short
indexleaf(ptr)
register unsigned char **ptr;
{
	unsigned short value;

	if(**ptr != INDEX){
		return(0);
	}
	(*ptr)++;
	value = *(unsigned short *)*ptr;
	*ptr+=2;
	return(value);
}

/*
 * calculate COFF type value from primitive OMF type (i.e. <= PRIM_TYP)
 */
static unsigned short
prim_o2ctyp(type)
register int type;
{
	register unsigned short ctype = 0;
	unsigned int unsign = 0;

	if(!(type & 0x80))	/* valid types have 8th bit high */
		return(T_NULL);
	switch((type >> 2) & 7){
	case 2:		/* real */
		switch(type & 3){
		case 0:
			ctype = T_FLOAT;
			break;
		case 1:
			ctype = T_DOUBLE;
			break;
		default:	/* unexpected */
			return(T_NULL);
		}
		break;
	case 1:
		unsign++;
	case 0:
		switch(type & 3){
		case 0:		/* char */
			ctype = unsign ? T_UCHAR : T_CHAR;
			break;
		case 1:		/* short */
			ctype = unsign ? T_USHORT : T_SHORT;
			break;
		case 2:		/* int or long */
			ctype = unsign ? T_UINT : T_INT;
			break;
		default:	/* not expected */ 
			return(T_NULL);
		}
		break;

	default:
		return(T_NULL);	
	}
	if((type >> 5) & 3)	/* pointer */
		 ctype |= (DT_PTR << N_BTSHFT);
#ifdef DEBUG
	if(debug)
		fprintf(stderr,"prim type: omf = %x\tcoff = %x\n",type,ctype);
#endif
	return(ctype);
}

/* COFF creats "fake" tag names for tagless structs, unions and enums */
static int fakenum = 0;		
static char fakename[12];

/* 
 * write symbol table entries for structures, unions and enums 
 */
static void
maketag(ctyp,typ,info,tindex)
int ctyp, tindex;
unsigned char *typ;
struct typ_info *info;
{
	union auxent ax;
	register union auxent *a = &ax;
	int i,count, size, lbnd, ubnd;
	long endpatch;
	unsigned char *ptr, *tptr, *nptr;
	unsigned char namesz;
	char *name;
	unsigned short index;
	int offset;
	int msclass;
	int n_scnum, n_value,n_numaux,n_sclass, n_type;
	int list;
	extern FILE *symentfile;
	void transtyp();
	void patchtag();

	omftypes[tindex] = (unsigned char *)0;	/* prohibit recursion on this type */
	if(ctyp == T_ENUM){
		/* assume SIGINT and 32 bits; skip to tag name */
		(void)numleaf(&typ); typ++;
		n_sclass = C_ENTAG;
		info->size = size = 4;
		info->ctype = n_type = T_ENUM;
	}
	else{
		info->size = size = numleaf(&typ) / 8;
		count = numleaf(&typ);

		/* type list */
		index = OTYP2IDX(indexleaf(&typ));
		tptr = omftypes[index] + 4;

		/* name list */
		index = OTYP2IDX(indexleaf(&typ));
		nptr = omftypes[index] + 4;

		if(count > 1 && isunion(tptr,nptr)){
			n_sclass = C_UNTAG;
			info->ctype = n_type = T_UNION;
			msclass = C_MOU;
		}
		else{
			n_sclass = C_STRTAG;
			info->ctype = n_type = T_STRUCT;
			msclass = C_MOS;
		}

		/* make sure all member types have been translated first */
		for(ptr = tptr, i=0; i < count; i++){
			index = indexleaf(&ptr);
			if(index > PRIM_TYP){
				transtyp(OTYP2IDX(index), tindex);
			}
		}

	}
	n_numaux = 1;
	n_scnum = N_DEBUG;
	n_value = 0;
	namesz = *++typ;
	name = (char *)++typ;

	typ += namesz;

	/* save this index first, so we can null out name */
	if(ctyp == T_ENUM)
		index = indexleaf(&typ);
	if(namesz)	/* tag name */
		*(name + namesz) = 0;	/* null for strtable */
	else{
		sprintf(fakename,".%1dfake",fakenum++);
		name = fakename;
	}

#ifdef DEBUG
	if(debug)
		fprintf(stderr,"tagname =%s\n",name);
#endif

	/* tag symbol and aux records */
	info->aux.tagndx = 
		symbol(name,n_value,n_scnum,n_sclass,n_type,n_numaux);
	memset(a, 0, SYMESZ);
	a->x_sym.x_misc.x_lnsz.x_size = size;
	endpatch = aux(a) * AUXESZ;

	n_scnum = N_ABS;
	if(ctyp == T_ENUM){
		unsigned char *eptr;

		/* name list */
		index = OTYP2IDX(index);
		nptr = omftypes[index] + 1;
		omftypes[index] = 0;

		lbnd = numleaf(&typ);
		ubnd = numleaf(&typ);
		eptr = nptr + *(unsigned short *)nptr;

		/* collect enum members */
		nptr += 3;
		n_sclass = C_MOE;
		n_type = T_MOE;
		n_numaux = 0;
		while(nptr < eptr){
			namesz = *++nptr;
			name = (char *)++nptr;
			nptr += namesz;
			offset = numleaf(&nptr);
			*(name + namesz) = 0;
			n_value = offset;
			(void)symbol(name,n_value,n_scnum,n_sclass,n_type,n_numaux);

#ifdef DEBUG
			if(debug)
				fprintf(stderr,"enum field: %s namesz = %d value = %x\n",name, namesz, offset);
#endif

		}
	}
	else{	/* structures and unions */

	    /* collect members */
	    while(count--){
		int patch, offset, omftyp;

		patch = 0;
		n_numaux = 0;
		memset(a, 0, SYMESZ);
		n_sclass = msclass;
		index = indexleaf(&tptr);
		namesz = *++nptr;
		name = (char *)++nptr;
		nptr += namesz;
		n_value = offset = numleaf(&nptr); 
		*(name + namesz) = 0;	/* null terminate for str table */

#ifdef DEBUG
if(debug)
	fprintf(stderr,"field: name = %s type = %d offset = %d\n",name, index, offset);
#endif
		if(index <= PRIM_TYP)
			n_type = prim_o2ctyp(index);
		else{
			/* field has complex type */

			struct typ_info *iptr;

			iptr = &o2cmap[OTYP2IDX(index)];
		        n_type = iptr->ctype;
			if(n_type == T_UINT){	/* bitfield */
				n_sclass = C_FIELD;
				n_value = (n_value * 8) + iptr->aux.tagndx; /* S000 */
				n_numaux = 1;
				a->x_sym.x_misc.x_lnsz.x_size = iptr->size;
			}
			else{
			   if(ISARY(n_type)){
				   n_numaux = 1;
				   *a = *iptr->aux.aptr;
			   }
			   switch(n_type & N_BTMASK){
			   case T_STRUCT:
			   case T_UNION:
			   case T_ENUM:
				   n_numaux = 1;
				   if(ISARY(n_type)){	/* array */
				     if(a->x_sym.x_tagndx < 0){
				       if(-a->x_sym.x_tagndx == tindex)
					  a->x_sym.x_tagndx = iptr->aux.aptr->x_sym.x_tagndx = info->aux.tagndx;
				       else{
				          omftyp = -a->x_sym.x_tagndx;
				          patch++;
				       }
				     }
				   }
				   else{	/* non-array */
				     if(iptr->aux.tagndx < 0){
				       if(-iptr->aux.tagndx == tindex)
				         a->x_sym.x_tagndx = iptr->aux.tagndx = info->aux.tagndx;
				       else{
				         omftyp = -iptr->aux.tagndx;
				         patch++;
				       }
				     }
				     else
				        a->x_sym.x_tagndx = iptr->aux.tagndx;
				     a->x_sym.x_misc.x_lnsz.x_size = iptr->size;
				   }
			   }
			}
		}

		/* write member symbol and aux record */
		(void)symbol(name,n_value,n_scnum,n_sclass,n_type,n_numaux);
		if(n_numaux)
			offset = aux(a);
		if(patch)
			patchtag(omftyp, offset * SYMESZ);	

	    }	/* while */
	}

	/* ".eos" symbol and aux records */
	(void)symbol(".eos",size,n_scnum,C_EOS,T_NULL,1);
	memset((char *)a, 0, AUXESZ);
	a->x_sym.x_tagndx = info->aux.tagndx;
	a->x_sym.x_misc.x_lnsz.x_size = size;
	(void)aux(a);

	/* patch endndx into tag aux record */
	fseek(symentfile, endpatch, 0);
	fread((char *)a, SYMESZ, 1, symentfile);
	a->x_sym.x_fcnary.x_fcn.x_endndx = nsyms;
	fseek(symentfile, endpatch, 0);
	fwrite((char *)a, SYMESZ, 1, symentfile);
}

/*
 * translate OMF type to COFF type
 * this maybe invoked recursively; if omftypes pointer is NULL, OMF type
 * has already been translated and recursion stops.
 * sindex is passed by tags when transtyp'ing their members because a 
 * member may refer to the tag before it is done being translated. In this
 * case, the type is assumed to be fully translated. The tagndx (COFF symbol
 * index) is patched in these member symbols by the "parent" tag
 */
void
transtyp(index, sindex)
unsigned int index, sindex;
{
	unsigned char *ptr;
	struct typ_info *map;
	unsigned short tindex, reclen;

	/* already translated */
	if(!(ptr = omftypes[index]))
		return;

	map = &o2cmap[index];
	ptr++;		/* skip linkage */
	reclen = *(unsigned short *)ptr;
	ptr+=2;

	/* currently translating this tag type */
	if(*ptr == STRUCTURE && index == sindex)
		return;

#ifdef DEBUG
	if(debug)
		fprintf(stderr,"%d\n",index + PRIM_TYP + 1);
#endif

	memset((char *)map, 0, sizeof(struct typ_info));

	switch(*ptr++){

	case NIL:	/* nil */

#ifdef DEBUG
		if(debug)
			fprintf(stderr,"nil\n");
#endif
		map->ctype = T_NULL;
		break;

	case BITFIELD:

#ifdef DEBUG
		if(debug)
			fprintf(stderr,"bitfield\n");
#endif
		map->ctype = T_UINT;
		map->size = numleaf(&ptr);
		map->aux.tagndx = *++ptr;	/* offset */

		break;

	case PROCEDURE:	/* procedure */

#ifdef DEBUG
		if(debug)
			fprintf(stderr,"procedure\n");
#endif
		ptr++;	/* skip NIL */
		tindex = indexleaf(&ptr);
		if(tindex <= PRIM_TYP)
			map->ctype = prim_o2ctyp(tindex);	
		else{
			tindex = OTYP2IDX(tindex); 
			transtyp(tindex, sindex);
			*map = o2cmap[tindex];
		}
		map->ctype = (((map->ctype << N_TSHIFT) & ~0x3f) | 
			      (map->ctype & N_BTMASK) | 
			      (DT_FCN << N_BTSHFT));
		break;

	  case STRUCTURE:	/* structure */

#ifdef DEBUG
		if(debug)
			fprintf(stderr,"structure\n"); 
#endif
	  	maketag(T_STRUCT,ptr,map,index);
		break;

	  case POINTER:	/* pointer */

#ifdef DEBUG
		if(debug)
			fprintf(stderr,"pointer\n");
#endif
		ptr++;		/* skip model */
		tindex = indexleaf(&ptr);
		if(tindex <= PRIM_TYP){
			map->ctype = prim_o2ctyp(tindex);
			map->size = psize(tindex);
		}
		else{
			tindex = OTYP2IDX(tindex); 
			transtyp(tindex, sindex);
			*map = o2cmap[tindex];
			/* S001 */
		        if(ISARY(map->ctype)){
				int i;

				switch(map->ctype & N_BTMASK){
				case T_STRUCT:
				case T_UNION:
				case T_ENUM:
					map->aux.tagndx = 
						map->aux.aptr->x_sym.x_tagndx;
					break;
				default:
					map->aux.tagndx = 0;
					break;
				}
				/* clear T_ARY */
				map->ctype = ((map->ctype >> 2) & ~N_BTMASK ) | 
					     (map->ctype & N_BTMASK);
			}
			/* S001 */
			switch(map->ctype & N_BTMASK){
			case T_STRUCT:
			case T_UNION:
				if(map->aux.tagndx == 0)
					map->aux.tagndx = -tindex;
			}
		}
		map->ctype = (((map->ctype << N_TSHIFT) & ~0x3f) |
				 (map->ctype & N_BTMASK) | 
				 (DT_PTR << N_BTSHFT));

		break;

	  case LABEL:	/* label: unexpected but listed as a type */

#ifdef DEBUG
		if(debug)
			fprintf(stderr,"label\n");
#endif
		  break;

	  case ARRAY:	/* array */
	  {
		  register union auxent *aptr;
		  int i, size;
		  struct typ_info *iptr;
#ifdef DEBUG
		if(debug)
			fprintf(stderr,"array\n");
#endif

		  if((aptr = (union auxent *)malloc(SYMESZ)) == NULL)
			fatal("Bad return from malloc: out of memory during type translation");
		  memset(aptr,0,SYMESZ);
		  size = numleaf(&ptr) / 8;	/* in bytes */
		  tindex = indexleaf(&ptr);

		  if(tindex > PRIM_TYP){

		     /* elements have complex type */

		     tindex = OTYP2IDX(tindex); 
		     transtyp(tindex, sindex);
		     iptr = &o2cmap[tindex];	
		     map->ctype = iptr->ctype;
		     if(ISARY(iptr->ctype)){
			  for(i = 0; i < (DIMNUM - 1); i++)
				  aptr->x_sym.x_fcnary.x_ary.x_dimen[i+1] = 
				  iptr->aux.aptr->x_sym.x_fcnary.x_ary.x_dimen[i];	
			  aptr->x_sym.x_fcnary.x_ary.x_dimen[0] = size;
			  /* size can become 0 if lower 16 bits are 0 
 			     e.g. x_size is a short; array sizes larger than
 			     this will low order 16 bits == 0 will show up
			     as 0 after truncation */
			  if(iptr->aux.aptr->x_sym.x_misc.x_lnsz.x_size)
				  aptr->x_sym.x_fcnary.x_ary.x_dimen[0] /= 
			          iptr->aux.aptr->x_sym.x_misc.x_lnsz.x_size;
		          aptr->x_sym.x_tagndx = iptr->aux.aptr->x_sym.x_tagndx;
		     }
		     else {
			  if(ISPTR(iptr->ctype))
			    aptr->x_sym.x_fcnary.x_ary.x_dimen[0] = size/4;
			  else
			    aptr->x_sym.x_fcnary.x_ary.x_dimen[0] = size/iptr->size;
	 	          aptr->x_sym.x_tagndx = iptr->aux.tagndx;
		     }
		  }
		  else {

			/* element type is primitive type */

			map->ctype = prim_o2ctyp(tindex); 
			aptr->x_sym.x_fcnary.x_ary.x_dimen[0] = size/psize(tindex);
		  }

		  map->ctype = (((map->ctype << N_TSHIFT) & ~0x3f) | 
				   (map->ctype & N_BTMASK) | 
 				   (DT_ARY << N_BTSHFT));
		  aptr->x_sym.x_misc.x_lnsz.x_size = size;
		  map->aux.aptr = aptr;
		  break;
	  }

	  case SCALAR:	/* scalar */

#ifdef DEBUG
		if(debug)
			fprintf(stderr,"scalar\n");
#endif
		/* currently, SCALAR types are used exclusively for enums */
	  	maketag(T_ENUM,ptr,map,0);
		break;

	  case LIST:	/* list: skip here, consumed by other types */

#ifdef DEBUG
		if(debug)
			fprintf(stderr,"list\n");
#endif
		return;	/* don't NULL out omftype pointer here */
		break;

	  case NEWTYPE:	/* newtype */ 

		  /* 
		   *  newtype records are unnecessary for either translation or 
		   *  debugging; currently, these are ignored 
		   */

#ifdef DEBUG
		if(debug)
			fprintf(stderr,"newtype\n");
#endif
		  break;

	  default:
		  warning("OMF type index %d: unknown type\n",index + PRIM_TYP + 1);
		  break;
	  }

	omftypes[index] = 0;	/* NULL pointer when translated */
}


/*
 * process_typ:  process OMF $$TYPES records and create a mapping from
 * OMF type index to COFF symbol record data for process_sym()
 * tag definition symbols (structs, enums, unions) are emitted here
 */
void
process_typ()
{
	int omfidx;
	unsigned char *otyp;
	void patchups();

#ifdef DEBUG
	if(debug)
		fprintf(stderr,"\n*** process_typ() ***\n\n otypbufsize = 0x%04x \n", otypbufsize);
#endif

	if(!otypbufsize)
		return;

	omfmaxidx = 0;

	/* build array of omf index:pointer pairs */
	omftypes = malloc(16 * sizeof(char *));
	if(!omftypes)
		fatal("Not enough memory to processes OMF type data");
	omfidx = 0;
	otyp = otypbuf;
	while(otyp < otypbuf + otypbufsize){
		omftypes[omfidx++] = otyp;
		otyp += *(unsigned short *)(otyp + 1) + 3;
		if(!(omfidx & 0xf)){
			omftypes = realloc(omftypes, omfidx * sizeof(char *) + 
						16 * sizeof(char *));
			if(!omftypes)
				fatal("Not enough memory to processes OMF type data");
		}
	}
	omfmaxidx = omfidx;

#ifdef DEBUG
	if(debug)
		fprintf(stderr,"\n%d OMF type(s)\n",omfmaxidx);
#endif
	
	/* initialize map table */
	if((o2cmap = (struct typ_info *)malloc(omfmaxidx * sizeof(struct typ_info))) == NULL)
		fatal("Not enough memory to process type data");

	/* process types */
	for(omfidx = 0; omfidx < omfmaxidx; omfidx++){
		if(omftypes[omfidx])
			(void)transtyp(omfidx, 0);
	}

	patchups();

	/* cleanup */
	free(otypbuf);
	otypbufsize = 0;
	free(omftypes);
}

/*
 * type translation for process_sym
 * provides COFF type for given OMF type and fills aux record if
 * COFF type requires an aux entry 
 * return value indicates whether or not aux record is used
 */
int
omf2coff_typ(omftype, cofftype, aux)
unsigned short omftype;
register int *cofftype;
union auxent *aux;
{
	register struct typ_info *iptr;
	int useaux = 0;

	memset((char *)aux, 0, AUXESZ);
	*cofftype = T_NULL;
	if(omftype <= PRIM_TYP){
		*cofftype = prim_o2ctyp(omftype);
#ifdef DEBUG
	if(debug)
		fprintf(stderr,"omf2coff: incoming type %d\toutgoing  %x\n", omftype,*cofftype);
#endif
		return(useaux);
	}
	omftype = OTYP2IDX(omftype); 
	if(omftype >= omfmaxidx)
		return(useaux);
	iptr = &o2cmap[omftype];
	*cofftype = iptr->ctype; 
	if(ISARY(*cofftype)){
		*aux = *iptr->aux.aptr;
		useaux++;
	}
	else { 
		switch(*cofftype & N_BTMASK){
		case T_STRUCT:
		case T_UNION:
		case T_ENUM: 
		  {
			useaux++;
			if(ISFCN(*cofftype))
			   aux->x_sym.x_tagndx = iptr->aux.tagndx;
			else{
			   aux->x_sym.x_tagndx = iptr->aux.tagndx;
			   aux->x_sym.x_misc.x_lnsz.x_size = iptr->size;
			}
			break;
		  }
		default:
			break;
		}
	}
#ifdef DEBUG
	if(debug)
		fprintf(stderr,"omf2coff: incoming type %d\toutgoing  %x\n", omftype + PRIM_TYP + 1,*cofftype);
#endif

	return(useaux);
}

/* 
 * return size in bytes of primitive OMF types 
 */ 
static int
psize(index)
register int index;
{
	
	if((index >> 5) & 3)	/* pointer */
		return(4);
	if(((index >> 2) & 0x7) == 2)	/* real */
		switch((index & 3)){
		default:
		case 0:
			return(4);
		case 1:
			return(8);
		case 2:
			return(10);
		}
	else			/* char, short, int, long */
		switch((index & 3)){
		default:
		case 2:
			return(4);
		case 0:
			return(1);
		case 1:
			return(2);
		}
}

/*
 * used by maketag: determine from type and name/offset list whether
 * a tag is a union. OMF does not explicitly indicate this, COFF does
 */
static int
isunion(tptr,ptr)
unsigned char *tptr, *ptr;
{
	int offset, type;

	/* check for a bitfield */
	type = indexleaf(&tptr);
	if(type > PRIM_TYP) {
		type = OTYP2IDX(type); 
		if(omftypes[type]){
			if(*(omftypes[type] + 3) == BITFIELD)
				return(0);
		}
		else if(o2cmap[type].ctype == T_UINT)
			return(0);
	}

	ptr += *(ptr + 1) + 2; 
	offset = numleaf(&ptr);
	ptr += *(ptr + 1) + 2; 
	if(offset == numleaf(&ptr))
		return(1);
	return(0);
}

/*
 * store $$TYPES records in core for pending translation process
 */
void
save_types(offset, buffer, size)
int offset; 
char *buffer;
long size;
{
	register int expand = (offset - otypbufsize) + size;

	if(!otypbufsize){
		if((otypbuf = (unsigned char *)malloc(expand)) == NULL)
			fatal("Not enough memory to process types data");
	}
	else{
		if((otypbuf = (unsigned char *)realloc(otypbuf,otypbufsize+expand)) == NULL)
			fatal("Not enough memory to process types data");
	}
	otypbufsize += expand;
	memcpy(otypbuf + offset, buffer, size);
	return;
}


/*
 * cleanup possible only after process_sym terminates.
 */
void
typ_cleanup()
{
	if(o2cmap)
		(void)free(o2cmap);
	fakenum = 0;
}

struct patch {
	int omftyp;
	long offset;
	struct patch *next;
};

static struct patch *patchlist;

/*
 *  store patch information: structure members may refer to tag types
 *  that are not completely defined when the structure member symbols are
 *  written. These must be backpatched. (ho hum)
 */
static void
patchtag(omftyp, offset)
int omftyp;
long offset;
{
	struct patch *node;

	if((node = (struct patch *)malloc(sizeof(struct patch))) == NULL)
		fatal("Not enough memory to process type data");
	node->omftyp = omftyp;
	node->offset = offset;
	node->next = patchlist;
	patchlist = node;

#ifdef DEBUG
	if(debug)
		fprintf(stderr,"patchtag: offset = %d omtyp = %d\n",offset, omftyp);
#endif
}

static void
patchups()
{
	extern FILE *symentfile;
	register struct patch *ptr;
	union auxent aux;

	while(ptr = patchlist){

#ifdef DEBUG
		if(debug)
			fprintf(stderr,"patchups: idx = %d tag = %d\n",
					ptr->offset,
					o2cmap[ptr->omftyp].aux.tagndx);
#endif

		patchlist = patchlist->next;
		(void)fseek(symentfile, ptr->offset, 0);
		(void)fread((char *)&aux, AUXESZ, 1, symentfile);
		aux.x_sym.x_tagndx = o2cmap[ptr->omftyp].aux.tagndx;
		(void)fseek(symentfile, -AUXESZ, 1);
		(void)fwrite((char *)&aux, AUXESZ, 1, symentfile);
		free(ptr);
	}
	(void)fseek(symentfile, 0L, 2);
}

/* END SCO_DEVSYS */
/* End Enhanced Application Compatibility Support */
