#ident	"@(#)localedef:collate.c	1.1.1.1"
#include <sys/types.h>
#include <stdio.h>
#include <locale.h>
#include <stddef.h>
#include "_colldata.h"
#include "_localedef.h"

/* You must read and understand colldata.h before working with this file */

static CollHead clh;
static struct codent *mcce_start = NULL;  /* list of all candidates for 
											  multbl section of output */
static struct codent *repl_start = NULL;  /* list of all codent entries
											representing one-to-many mappings */
static struct codent ignore_codent = {0,0,0,COLLDEF, 0, 0, 0, 0, 0, 0,0,0, WGHT_IGNORE};
/* special codent to represent all undefined characters */
static struct codent undef_codent = {0, 0, 0, UNDEF_COLL};
static CollElem blank = {0, 0, WGHT_SPECIAL};
static CollElem null_celm = {0, 0, WGHT_IGNORE};

/* head of list of codents as they were physically entered after the 
	order_start */
static struct codent *order = NULL;

/* Checks if to codents are equal for the purpose of combining into a special
	CollMult to save space - the so called "unremarkable collating elements"
	referred to in the colldata.h header */
mboolean_t
isequal_codent(struct codent *one, struct codent *two)
{
	unsigned int i;
	/* two is always further along the chain and 
	   one could be the last element */
	if(two == NULL)
		return(FALSE);

	/* have to be adjacent both in codepoint and in ordering */
	if(one->cd_code + 1 != two->cd_code ||
		(unsigned int) one->cd_weights[0] + 1 != (unsigned int) two->cd_weights[0])
		return(FALSE);

	if(one->cd_subnbeg != two->cd_subnbeg ||
		one->cd_multbeg != two->cd_multbeg)
			return(FALSE);

	for(i = 1; i < clh.nweight; i++) {
		/* self referential weights do not disqualify - WGHT_SPECIAL covers */
		if(one->cd_weights[i] == one && two->cd_weights[i] == two)
			continue;
		if(one->cd_weights[i] != two->cd_weights[i])
			return(FALSE);
	}
	return(TRUE);
}


/* Handle collating symbol keyword */
void 
kf_collsym_func(unsigned char *line_remain, void **args) 
{

	unsigned char *name;
	struct syment *tsym;

	line_remain = skipspace(line_remain);
	if((name = getsymchar(&line_remain)) == NULL) {
		diag(ERROR, TRUE, ":81:Invalid symbolic character name\n");
		return;
	}

	if((tsym = addsym(name)) == NULL)
		return;
	tsym->sy_codent = 
		(struct codent *) getmem(NULL,sizeof(struct codent),TRUE,TRUE);
	tsym->sy_codent->cd_flags |= SYM_COLL;
	
	if(!EOL(line_remain))
		diag(WARN,TRUE,extra_char_warn);

}

/* copy internal data structure to external form */
collcopy(CollElem *target, struct codent *source)
{
	unsigned int i;

	if(source->cd_flags & EQUIV_COLL) {
		target->weight[1] = (wchar_t) source->cd_weights[1];
		for(i = 2; i < clh.nweight; i++)
			target->weight[i] = WGHT_IGNORE;
			
	}
	else
	for(i = 1; i < clh.nweight; i++) {
		/* Only case where this should happen is mcces from the mcce_start
		   list which are not actually used in the collation order */
		if(source->cd_weights[i] == NULL) {
			target->weight[i] = WGHT_IGNORE;
			continue;
		}
		/* If one of the weights is a one-to-many mapping, use the index
			into the repltbl and mark it with the WGHT_SPECIAL to so indicate */
		if(source->cd_weights[i]->cd_flags & REPL_COLL) {
			target->weight[i] = (wuchar_t) source->cd_weights[i]->cd_orderlink| WGHT_SPECIAL;
			continue;
		}

		if(!(source->cd_weights[i]->cd_flags & COLLDEF)) {
			diag(ERROR,FALSE,":153:Weight level %d of codepoint %#x undefined\n", i, source->cd_code);
		}
		target->weight[i] = (wuchar_t) source->cd_weights[i]->cd_weights[0];
	}
	target->weight[0] = (wuchar_t) source->cd_weights[0];
	target->subnbeg = (unsigned short) source->cd_subnbeg;
	target->multbeg = (unsigned short) source->cd_multbeg;
}

/* create (if necessary) and fill in  the data structure to handle equivalence 
   classes */
void
equiv_expand(struct codent *tcode, mboolean_t followorder)
{
	struct codent *common, *existing, *olist;
	mboolean_t iscontig;

	for(;tcode != NULL; tcode = followorder ? tcode->cd_orderlink : tcode->cd_codelink) {
		/* Look for codents which indicate they serve as the definition of 
		   an equivalence class for more than one codepoint.  Normally, 
		   if the codent point to the equiv class has not been marked it is 
		   a case where it is an equivalence class of one (see kf_ostart_func). 
		   Ellipsis codents and the undefined codent represent special cases.
		   They can represent multiple values with one codent structure.
		   In the normal case where there are multiple codepoints in the 
		   equivalence class, the special intermediary codent is used which 
		   is marked as an EQUIV_COLL. */
		if(tcode->cd_equiv != NULL && 
		  (tcode->cd_equiv->cd_flags & (EQUIV_COLL|ELIP_COLL|UNDEF_COLL))) {
			struct codent *lo, *hi;
			/* In all three cases, the high and low values are saved in weights
			   0 and 1. */
			lo = tcode->cd_equiv->cd_weights[0];
			hi = tcode->cd_equiv->cd_weights[1];

			/* should only happen when ellipsis or undefined special cases are
				empty; in no case should hi or lo be NULL unless both are */
			if(hi == lo)
				continue;

			/* Of course, the hi and low of a "normal range" may of themselves
			   be the undefined symbols or an ellipsis range. 
			*/

			if(lo->cd_flags & (UNDEF_COLL|ELIP_COLL)) {
			    /* For the ellipsis and undefined cases, it is possible 
				   that they are empty.  If that is the case, find the next 
				   suitable members of the equivalence class. */
				if(lo->cd_weights[0] == NULL) {
					lo = order;
					do {
						lo = lo->cd_orderlink;
					} while(lo != NULL && lo->cd_weights[1] != tcode);

					if(lo == NULL)
						continue;
				}
				else lo = lo->cd_weights[0];
						
			}

			if(hi->cd_flags & (UNDEF_COLL|ELIP_COLL)) {
			    /* For the ellipsis and undefined cases, it is possible 
				   that they are empty.  If that is the case, find the next 
				   suitable members of the equivalence class. */
				if(hi->cd_weights[1] == NULL) {
					common = NULL;
					hi = lo;
					while(hi != NULL) {
						if(hi->cd_weights[1] == tcode)
							common = hi;
						hi = hi->cd_orderlink;
					}
					/* If no members found after lo, then this is vacuous
						equivalence class.  Otherwise, make sure new
						high point is also not an empty ellipsis or undefined. */
					if(common == NULL)
						continue;
					hi = common;
				}
				else hi = hi->cd_weights[1];
			}

			if(hi == lo)
				continue;


			olist = lo;
			common = NULL;
			iscontig = TRUE;
			while(olist != hi->cd_orderlink) {
			/* If the codepoint is in the equivalence class,
				do the work.
				*/
				if(olist->cd_weights[1] == hi->cd_weights[1]) {
					/* If the codent already starts an mcce, then use
					   the existing null terminator.  In this case, we 
					   mark it as a non-continguous equiv. class 
					   instead of going back to cleanup if we find it
					   is non-contiguous.  Since  there are few mcces, this
					   is not expected to be a big problem.
					*/
					if(olist->cd_multbeg != NULL) {
						existing = olist->cd_multbeg;
						while(existing->cd_code != 0) {
							existing = existing->cd_codelink;
						}
						existing->cd_weights[0] = lo->cd_weights[0];
						existing->cd_weights[1] = hi->cd_weights[0];
						/* marking so treated specially in collcopy */
						existing->cd_flags |= EQUIV_COLL;
						existing->cd_subnbeg =SUBN_SPECIAL;
					}
					/* If the codent is not an mcce start, use a common
					   null terminator created for this purpose. */
					else if(common  != NULL)
							olist->cd_multbeg = common;
					else {
						common = (struct codent *) getmem(NULL,sizeof(struct codent),TRUE,FALSE);
						common->cd_codelink = mcce_start;
						mcce_start = common;
						common->cd_weights[0] = lo->cd_weights[0];
						common->cd_weights[1] = hi->cd_weights[0];
						/* marking so treated specially in collcopy */
						common->cd_flags |= EQUIV_COLL;
						olist->cd_multbeg = common;
					}
				}
				else
					iscontig = FALSE;  /* if we find the class is not
											contiguous mark the flag */
				olist = olist->cd_orderlink;
			}
			/* If the class is not contiguous, mark the common terminator */
			if(common != NULL && !iscontig)
				common->cd_subnbeg = SUBN_SPECIAL;
		}
	}
}
		

void
write_collate(FILE *cfile)
{
	CollElem dummy, *cel;
	CollMult clm;
	unsigned int i;
	struct codent *tcode, *tcode2, *tcode3, *tcode4;
	mboolean_t iscontig;

	if(fseek(cfile, sizeof(CollHead), SEEK_SET) == -1) {
		diag(ERROR,FALSE, ":15:Cannot seek to end of CollHead in file LC_COLLATE\n");
		return;
	}
	clh.maintbl = sizeof(CollHead);
	clh.elemsize = offsetof(CollElem, weight[clh.nweight]);;
	clh.version = CLVERS;
	/* do equivalence class processing */
	equiv_expand(order,TRUE);
	equiv_expand(repl_start,FALSE);
	equiv_expand(&ignore_codent,TRUE);

	/* set up indexes for multtbl section */
	tcode = mcce_start;
	i = 1;
	while(tcode != NULL) {
		/* This works because all the multbeg references in this list
		   are backward references, so their indexes are already set.
		*/
		if(tcode->cd_multbeg != NULL)
			tcode->cd_multbeg = tcode->cd_multbeg->cd_orderlink;
		tcode->cd_orderlink = (struct codent *) i++;
		tcode = tcode->cd_codelink;
	}
	if(i > USHRT_MAX)
		diag(FATAL_LIMITS,FALSE,":103:multtbl section too large\n");

	/* set up indexes into repltbl section */
	tcode = repl_start;
	i = 1;
	while(tcode != NULL) {
		tcode->cd_orderlink = (struct codent *) i;
		/* leave space for each codepoint plus the null terminator */
		i += tcode->cd_subnbeg+1;
		tcode = tcode->cd_codelink;
		if(i & WGHT_SPECIAL)
			diag(FATAL_LIMITS,FALSE,":121:repltbl section too large\n");
	}

	/* go through the main list and get the indexes into the multbl
	   section */
	tcode = strt_codent.cd_codelink;
	while(tcode != NULL) {
		if(tcode->cd_multbeg)
			tcode->cd_multbeg = tcode->cd_multbeg->cd_orderlink;
		tcode = tcode->cd_codelink;
	}

	tcode = strt_codent.cd_codelink;
	/* decide if the main table will be indexed or not */
	if(density() > .75) {
		clh.flags |= CHF_INDEXED;
		clh.nmain = maxcodept;
	}
	else
		clh.nmain = 256;
	/* 0 codepoint needs to be set this way for library */
	if(tcode->cd_code == 0) {
		for(i = 1; i < clh.nweight; i++) {
			if(tcode->cd_weights[i] != tcode && 
				tcode->cd_weights[i] != &ignore_codent) {
				diag(WARN, FALSE, ":110:<NUL> character weights reset to WGHT_IGNORE\n");
				break;
			}
		}
		tcode = tcode->cd_codelink;
	}
	if(fwrite(&null_celm,1, clh.elemsize,  cfile) != clh.elemsize) {
		diag(ERROR, FALSE, ":17:Cannot write CollElem for codepoint 0 to file LC_COLLATE\n");
		return;
	}

	/* put out all the main table elements, substituting blanks as necessary */
	for(i = 1; i < clh.nmain; i++) {
		if(tcode != NULL && tcode->cd_code == i) {
			collcopy(&dummy, tcode);
			tcode = tcode->cd_codelink;
			cel = &dummy;
		}
		else
			cel = &blank;

		if(fwrite(cel,1, clh.elemsize,  cfile) != clh.elemsize) {
			diag(ERROR, FALSE, ":18:Cannot write CollElem for codepoint %d to file LC_COLLATE\n",i);
			return;
		}
	}
	/* Take care of the rest of the codepoints. */
	while(tcode != NULL) {
		clh.nmain++;
		i = 1;
		tcode2 = tcode;
		/* look for possibilities of combining "unremarkable elements"
			into single ColMult */
		while(isequal_codent(tcode2, tcode2->cd_codelink)) {
			tcode2 = tcode2->cd_codelink;
			if(i == SUBN_SPECIAL)
				break;
			i++;
		}
		clm.ch = tcode->cd_code;
		collcopy(&clm.elem,tcode);
		/* finish setting up unremarkables */
		if(i > 1) {
			clm.elem.subnbeg = SUBN_SPECIAL | (i - 1);
			for(i = 1; i < clh.nweight; i++) 
				if(clm.elem.weight[i] == clm.elem.weight[0])
					clm.elem.weight[i] = WGHT_SPECIAL;
		}
		tcode = tcode2->cd_codelink;
		if(fwrite(&clm,1, offsetof(CollMult,elem.weight[clh.nweight]), cfile) != offsetof(CollMult,elem.weight[clh.nweight])) {
			diag(ERROR, FALSE, ":19:Cannot write CollMult for codepoint %d to file LC_COLLATE\n",tcode->cd_code);
			return;
		}
	}
	/* put out multtbl section */
	if(mcce_start != NULL) {
		/* fake it out to not waste space */
		clh.multtbl = ftell(cfile) - offsetof(CollMult,elem.weight[clh.nweight]);
		tcode = mcce_start;
		while(tcode != NULL) {
			clm.ch = tcode->cd_code;
			collcopy(&clm.elem,tcode);
			tcode = tcode->cd_codelink;
			if(fwrite(&clm,1, offsetof(CollMult,elem.weight[clh.nweight]), cfile) != offsetof(CollMult,elem.weight[clh.nweight])) {
				diag(ERROR, FALSE, ":19:Cannot write CollMult for codepoint %d to file LC_COLLATE\n",tcode->cd_code);
				return;
			}
		}
	}
	/* put out repltbl section */
	if(repl_start != NULL) {
		/* fake it out to not waste space */
		clh.repltbl = ftell(cfile) - sizeof(wchar_t);
		tcode = repl_start;
		while(tcode != NULL) {
			for(i = 0; i < tcode->cd_subnbeg; i++) {
				if(!(tcode->cd_weights[i]->cd_flags & COLLDEF)) {
					diag(ERROR, FALSE, ":137:Undefined weight for codepoint %#x\n", tcode->cd_weights[i]->cd_code);
					return;
				}
				if(fwrite(&tcode->cd_weights[i]->cd_weights[0],1, sizeof(wchar_t), cfile) != sizeof(wchar_t)) {
					diag(ERROR, FALSE, ":27:Cannot write Repltbl for codepoint %d to file LC_COLLATE\n",tcode->cd_weights[i]->cd_weights[0]);
					return;
				}
			}
			/* add null terminator */
			dummy.weight[0] = WGHT_IGNORE;
			if(fwrite(&dummy.weight[0],1, sizeof(wchar_t), cfile) != sizeof(wchar_t)) {
				diag(ERROR, FALSE, ":30:Cannot write terminator for replacement string to file LC_COLLATE\n");
				return;
			}
			tcode = tcode->cd_codelink;
			/* free(tcode) */
		}
	}

	fseek(cfile, 0, SEEK_SET);
	if(fwrite(&clh, 1, sizeof(CollHead),  cfile) != sizeof(CollHead)) {
		diag(ERROR, FALSE, ":136:Unable to write CollHead to LC_COLLATE file\n");
		return;
	}
}


/* find all undefined codepoints and put into linked list */
struct codent *
unordered_codes(struct codent *undef)
{
	
	struct codent *begin = NULL;
	struct codent *end = NULL;
	struct codent *tcode, **ttcode;
	unsigned int count;
	unsigned int lpstrt = 1;

	ttcode = &begin;
	tcode = strt_codent.cd_codelink;
	/* All UNDEFINED codepoints are in same equivalence class
	   by default.  If user gave UNDEFINED explicitly and with explict
	   weights or with ellipsis, the loop does the right thing.
	   Otherwise, it must be special cased. */
	if(!(undef->cd_flags & COLLDEF) || (undef->cd_flags & NOWEIGHTS))
		lpstrt = 2;
	while(tcode != NULL) {
		if(!(tcode->cd_flags & COLLDEF)) {
			*ttcode = tcode;
			ttcode = &(tcode->cd_orderlink);
			if(lpstrt > 1)
				tcode->cd_weights[1] = begin;
			for(count=lpstrt; count < clh.nweight; count++) {
				if(!(undef->cd_flags & COLLDEF) || undef->cd_weights[count] == undef)
					tcode->cd_weights[count] = tcode;
				else
					tcode->cd_weights[count] = undef->cd_weights[count];
			}
			tcode->cd_flags |= COLLDEF;
			end = tcode;
		}
		tcode = tcode->cd_codelink;
	}
	/* Make sure equivalence class processing is done for UNDEFINED equivalence
	   class */
	if(lpstrt && begin != NULL)
		begin->cd_equiv = undef;
	undef->cd_weights[0] = begin;
	undef->cd_weights[1] = end;
	if(end != NULL)
		end->cd_orderlink = undef->cd_orderlink;
	return(begin);
}

/* expand ellipsis with all appropriate codents */
static struct codent *
elip_codes(struct codent *elip)
{
	struct codent *tcode, *strt, *end = NULL;
	unsigned int count;

	if(elip->cd_multbeg != NULL)
		tcode = elip->cd_multbeg->cd_codelink;
	else {
		tcode = strt_codent.cd_codelink;
	}
	strt = tcode;
	while(tcode != elip->cd_weights[0]) {
		tcode->cd_orderlink = tcode->cd_codelink;
		if(tcode->cd_flags & COLLDEF) {
			diag(ERROR, FALSE, ":152:Weight for codepoint %#x defined in ellipsis and elsewhere\n",tcode->cd_code);
			return(elip);
		}
		tcode->cd_flags |= COLLDEF;
		for(count=1; count < clh.nweight; count++) {
			if(elip->cd_weights[count] == elip)
				tcode->cd_weights[count] = tcode;
			else
				tcode->cd_weights[count] = elip->cd_weights[count];
		}
		end = tcode;
		tcode = tcode->cd_codelink;
	}

	if(end == NULL) {
		elip->cd_weights[0] = elip->cd_weights[1] = NULL;
		return(NULL);
	}
	elip->cd_weights[0] = strt;
	elip->cd_weights[1] = end;
	end->cd_orderlink = elip->cd_orderlink;
	return(strt);
}
		


#define CWF_FORWARD	0x4	/* fake definition for this to check errors */

void
kf_ostart_func(unsigned char *line_remain, void **args)
{
	unsigned char *end, *line, tchar, *strt;
	struct codent *tcode, *tcode2, **ttcode, *elip, *tcode3, hold;
	struct syment *tsym;
	unsigned int  i;
	int j;
	struct codent *prev;
	unsigned char qchar;
	mboolean_t unkcharflg = FALSE;
	/* degenerate flag kept TRUE as long as can collapse to one weight
		in output; e.g., if no weights given or all weights are same as 
		ordering of codepoints
	*/
	mboolean_t degenerate = TRUE;	
	unsigned char *symname;


	prev = NULL;
	elip = NULL;
	ttcode = &order;
	line_remain = skipspace(line_remain);
	end = line_remain;
	while(isalpha(*end)) end++;
	if(end != line_remain) {
		clh.nweight++;
		for(;;) {
			if(strncmp("backward",(char *)line_remain, end - line_remain) == 0) {
				degenerate = FALSE;
				clh.order[clh.nweight] |= CWF_BACKWARD;
			}
			else if(strncmp("position",(char *)line_remain, end - line_remain) == 0) {
				clh.order[clh.nweight] |= CWF_POSITION;
				degenerate = FALSE;
			}
			else if(strncmp("forward",(char *)line_remain, end - line_remain) == 0) {
				clh.order[clh.nweight] |= CWF_FORWARD;
			}
			else {
				diag(ERROR, TRUE, ":140:Unknown collation rule on order_start\n");
				break;
			}
			end = skipspace(end);
			if(*end != ';' && *end != ',') 
				break;
			if(*end == ';') {
				if((clh.order[clh.nweight] & (CWF_BACKWARD|CWF_FORWARD)) == 
				   		(CWF_FORWARD|CWF_BACKWARD)) {
					diag(ERROR, TRUE, ":58:forward and backward rules cannot apply to same collation weight on order_start\n");
				}
				else 
					clh.order[clh.nweight] &= ~CWF_FORWARD;
				if(++clh.nweight == COLL_WEIGHTS_MAX -1) 
					diag(FATAL_LIMITS,TRUE,":111:Number of weights exceeds implementation limit\n");
			}
			end++;
			end = skipspace(end);
			line_remain = end;
			while(isalpha(*end)) end++;
			if(end == line_remain) {
				diag(ERROR, TRUE, ":42:Dangling comma or semi-colon on order_start\n");
				break;
			}
		}
	}

	if(!EOL(end))
		diag(WARN,TRUE,extra_char_warn);

	/* by default, one forward pass is assumed */
	if(clh.nweight == 0)
		clh.nweight = 1;

	/* include space for basic ordering weight; 0th weight */
	clh.nweight++;
	while((line = getline()) != NULL) {
		if(strncmp("order_end", (char *) line,9) == 0) {
			/* if no explict UNDEFINED, put it at the end */
			if(!(undef_codent.cd_flags & COLLDEF)) {
				if(elip != NULL)
					diag(ERROR,TRUE,":45:Ellipsis may not be last in ordering unless UNDEFINED is specified elsewhere in ordering\n");
				else {
					*ttcode = &undef_codent;
					diag(WARN,TRUE,":147:Unspecified (\"UNDEFINED\") codepoints will default to end of ordering\n");
				}
			}
			ttcode = &order;
			while((tcode = *ttcode) != NULL) {
				if(tcode->cd_flags & ELIP_COLL) {
						if((*ttcode = elip_codes(tcode)) == NULL &&
							(*ttcode = tcode->cd_orderlink) == NULL)
								break;
						if(*ttcode == tcode)
							return;
						tcode = *ttcode;
				}
				ttcode = &tcode->cd_orderlink;
			}
					
			ttcode = &order;
			i = WGHT_IGNORE +1;
			/* now assign basic weights to all defined collation elements,
			   expanding the UNDEFINED set of elements when it occurs. */
			while((tcode = *ttcode) != NULL) {
				if(tcode->cd_flags & UNDEF_COLL) {
						if((*ttcode = unordered_codes(tcode)) == NULL
							 && (*ttcode = tcode->cd_orderlink) == NULL)
								break;
						/* If non empty UNDEFINED "class", then equivalence 
							class is created can cannot use degenerate form. 
							Other cases involving UNDEFINED are handled in 
							normal course of code in kf_ostart_func. */
						if(!(tcode->cd_flags & COLLDEF))
							degenerate = FALSE;
						tcode = *ttcode;
				}
				tcode->cd_weights[0] = (struct codent *) i++;
				ttcode = &tcode->cd_orderlink;
			}
			if(degenerate)
				clh.nweight = 1;
			return;
		}
		if(strncmp("...",(char *)line,3) == 0) {
			/* if the codepoint before the ellipsis is character not in the 
			   charmap */
			if(unkcharflg) {
				diag(ERROR, TRUE, ":5:Beginning of ellipsis undefined\n");
				continue;
			}
			if(prev != NULL && prev->cd_flags &  (UNDEF_COLL|MCCE_COLL|SYM_COLL)) {
				diag(ERROR, TRUE,":8:Cannot have multicharacter collating element, collating symbol, or UNDEFINED characters on line before ellipsis\n");
				continue;
			}
			if(elip != NULL) {
				diag(ERROR,TRUE,":41:Contiguous ellipsis without intervening codepoint\n");
				continue;
			}
			tcode = (struct codent *) getmem(NULL,sizeof(struct codent), TRUE, TRUE);
			tcode->cd_flags = ELIP_COLL;
			tcode->cd_multbeg = prev; /* save codent for the lower boundary
										(exclusive) of the ellipsis range */
			elip = tcode;
			line +=3;
		}
		else
		if(strncmp("UNDEFINED",(char *)line,9) == 0) {
			if(elip != NULL) {
				diag(ERROR,TRUE,":9:Cannot have UNDEFINED characters just after ellipsis\n");
				continue;
			}
			if(undef_codent.cd_flags & COLLDEF) {
				diag(ERROR,TRUE,":101:Multiple UNDEFINED statements\n");
				continue;
			}
			prev = tcode = &undef_codent;
			line +=9;
		}
		else {
			if((tcode = getcolchar(&line,'\0',&symname)) == NULL) {
				if(symname != NULL) {
					diag(WARN,TRUE,":129:Symbolic character %s not found in charmap: collation line ignored\n",symname);
					unkcharflg = TRUE;
					if(elip != NULL) 
						diag(ERROR,TRUE,":44:Ellipsis endpoint undefined\n");
				} else
					diag(ERROR, TRUE, ":139:Unknown collating identifier\n");
				continue;
			}
			unkcharflg = FALSE;
			prev = tcode;
			if(elip != NULL) {
				if(tcode->cd_flags & (SYM_COLL|MCCE_COLL))
					diag(FATAL, TRUE, ":84:Line after ellipsis must not contain valid character in codeset\n");
				/* if ellipsis at beginning of list, then multbeg = NULL */
				if(elip->cd_multbeg != NULL
				   && elip->cd_multbeg->cd_code > tcode->cd_code) {
				   	diag(FATAL, TRUE, ":48:Ending codepoint %#x on ellipsis range is less than starting codepoint %#x\n", tcode->cd_code, elip->cd_multbeg->cd_code);
				}
				elip->cd_weights[0] = tcode; /* save codent for upper 
												boundary (exclusive) of the
												ellipsis range */
				elip = NULL;
			}
		}
		if(tcode->cd_flags & COLLDEF) {
			if(tcode->cd_orderlink == NULL) {
				diag(WARN,TRUE,":100:Multiple adjacent definitions for codepoint %#x\nAll definitions after first ignored\n",tcode->cd_code);
				continue;
			}
			diag(ERROR,TRUE, ":102:Multiple weight definitions for codepoint %#x\n", tcode->cd_code);
			continue;
		}
		tcode->cd_flags |= COLLDEF;
		*ttcode = tcode;
		ttcode = &tcode->cd_orderlink;
		line = skipspace(line);
		if(tcode->cd_flags & SYM_COLL) {
			if(!EOL(line))
				diag(WARN,TRUE,":56:Extra characters after collating-symbol ignored\n");
			continue;
		}
		i = 1;
		if(EOL(line))
			tcode->cd_flags |= NOWEIGHTS;
		for(;;) {
			if(strncmp((char *)line,"IGNORE",6) == 0) {
				degenerate = FALSE;
				tcode2 = &ignore_codent;
				line +=6;
				goto eqproc;
			}
				
			if(strncmp((char *)line,"...",3) == 0) {
				tcode->cd_weights[i] = tcode;
				line += 3;
				goto semi;
			}
			if(EOL(line))
				tcode2 = tcode;
			else {
				qchar = ';';
				if(*line == '"') {
					line++;
					qchar = '"';
				}
				strt = line;
				j = 0;
				symname = NULL;
				while((tcode2 = getcolchar(&line, qchar,&symname)) != NULL) {
					if(j >= COLL_WEIGHTS_MAX) {
						diag(LIMITS, TRUE, ":112:One to many mapping has too many weights; Maximum number supported is %d\n",COLL_WEIGHTS_MAX);
						goto skipline;
					}
					hold.cd_weights[j] = tcode2;
					j++;
				}
				if(symname != NULL) {
					diag(ERROR,TRUE,":129:Symbolic character %s not found in charmap: collation line ignored\n",symname);
					goto skipline;
				}
				if(j == 0) {
					diag(ERROR,TRUE,":145:Unknown weight at weight level %d\n",i);
					goto skipline;
				}
				if(j > 1) {
					tchar = *line;
					*line = '\0';
					/* Search through repl_coll list to look for match.  This
					   is necessary to ensure that there is only one codent
					   for each one-to-many mapping, which in turn is necessary
					   for the equivalence class processing to work properly.
					*/
					tcode2 = repl_start;
					while(tcode2 != NULL) {
						if(j == tcode2->cd_subnbeg) {
							while(j-- > 0) {
								if(tcode2->cd_weights[j] !=
									hold.cd_weights[j])
										break;
							}
							if(j < 0) 
								break;
							j = tcode2->cd_subnbeg;
						}
						tcode2 = tcode2->cd_codelink;
					}
					if(tcode2 == NULL) {
						tcode2 = (struct codent *) getmem(NULL, sizeof(struct codent),TRUE,TRUE);
						tcode2->cd_flags |= REPL_COLL;
						tcode2->cd_codelink = repl_start;
						repl_start = tcode2;
						tcode2->cd_subnbeg = j;
						for(--j; j >= 0; j--) 
							tcode2->cd_weights[j] = hold.cd_weights[j];
					}
					*line = tchar;
				}
				else
					/* single quoted character is the same as not
					   quoted */
					tcode2 = hold.cd_weights[0];
				if(qchar == '"') {
					if(*line != '"') {
						diag(ERROR, TRUE, ":61:Illegal string at weight level %d\n",i);
						goto skipline;
					}
					else
						line++;
				}
				if(tcode != tcode2)
					degenerate = FALSE;
			}

eqproc:
			tcode->cd_weights[i] = tcode2; 
			if(i == 1) {
				/* For first one, do not set up whole structure because often
				   will be one character equivalence class, in which case 
				   nothing special will be required in the output. */
				if(tcode2->cd_equiv == NULL) {
					tcode2->cd_equiv = tcode;
				}
				else if(!(tcode2->cd_equiv->cd_flags & EQUIV_COLL)) {
						tcode3 = (struct codent *) getmem(NULL,sizeof(struct codent),TRUE,TRUE);
						/* put lowest member of equiv class in weights[0]
						   and highest in weights[1] */
						tcode3->cd_weights[0] = tcode2->cd_equiv;
						tcode3->cd_weights[1] = tcode;
						tcode3->cd_flags = EQUIV_COLL;
						tcode2->cd_equiv = tcode3;
				}
				else {
					/* fill in new highest member of equiv class */
					tcode2->cd_equiv->cd_weights[1] = tcode;
				}
			}
semi:
			i++;
			if(i >= clh.nweight)
				break;
			if(EOL(line))
				continue;
			line = skipspace(line);
			if(*line != ';') {
				diag(ERROR,TRUE,":89:Malformed weight specifications\n");
				break;
			}
			line++;
			line = skipspace(line);
		}
		if(i >= clh.nweight && !EOL(line)) {
			diag(WARN,TRUE, extra_char_warn);
		}
skipline:;
	}
	diag(ERROR,TRUE, ":92:Missing order_end before EOF");

}


/* if necessary, add structure to support next element of mcce; returns
   codent from mcce structure */
struct codent *
mcce_create(struct codent *head, CODE code)
{
	struct codent *tcode, **ttcode;
	ttcode = &head->cd_multbeg;
	while((tcode = *ttcode) != NULL && tcode->cd_code != 0) {
		if(tcode->cd_code == code)
			return(tcode);
		ttcode = &tcode->cd_codelink;
	}
	*ttcode = (struct codent *) getmem(NULL, sizeof(struct codent),TRUE,TRUE);
	
	(*ttcode)->cd_code = code;
	(*ttcode)->cd_flags |= MCCE_COLL;
	clh.flags |= CHF_MULTICH;
	if(tcode == NULL) {
		/* add null terminator */
		tcode = (struct codent *) getmem(NULL,sizeof(struct codent),TRUE,TRUE);
		(*ttcode)->cd_codelink = tcode;
		tcode->cd_codelink = mcce_start;
		mcce_start = *ttcode;
	}
	else
		(*ttcode)->cd_codelink = tcode;

	return(*ttcode);
}


void
kf_collelm_func(unsigned char *line_remain, void **args)
{

	unsigned char *name;
	struct syment *tsym;
	struct codent *head, *tcode;

	line_remain = skipspace(line_remain);
	if((name = getsymchar(&line_remain)) == NULL) {
		diag(ERROR, TRUE, ":81:Invalid symbolic character name\n");
		return;
	}

	if((tsym = addsym(name)) == NULL)
		return;
	
	line_remain = skipspace(line_remain);

	if(strncmp("from",(char *)line_remain, 4) != 0) {
		diag(ERROR, TRUE, ":91:Missing \"from\" in collating element definition\n");
		return;
	}
	line_remain +=4;
	line_remain = skipspace(line_remain);

	if(*line_remain++ != '"') 
		goto badstring;

	if((head = getlocchar(&line_remain,'"',NULL)) == NULL)
		goto badstring;

	while((tcode = getlocchar(&line_remain, '"',NULL)) != NULL) {
		head = mcce_create(head, tcode->cd_code);
	}

	if(*line_remain == '"') {
		line_remain++;
		line_remain = skipspace(line_remain);
		if(!EOL(line_remain))
			diag(WARN, TRUE, extra_char_warn);
		/* mcce symbol points at codent of last character in mcce structure */
		tsym->sy_codent = head;
		return;
	}

badstring:
		diag(ERROR, TRUE, ":88:Malformed string or unknown character in collating element definition\n");
}

extern void *kf_collate_list[];
struct keyword_func kf_collate_tab[] = {
	{ "END", kf_END_func, kf_collate_list },
	{ "collating-element", kf_collelm_func, NULL},
	{ "collating-symbol", kf_collsym_func, NULL},
	{ "copy", kf_copy_func, kf_collate_list},
	{ "order_start", kf_ostart_func, NULL}
};

struct kf_hdr kfh_collate = {
	5,
	kf_collate_tab
};

void *kf_collate_list[] = { (void *) "LC_COLLATE", (void *)&kfh_collate, (void *)LC_COLLATE, (void *) "LC_COLLATE", (void *) write_collate };
