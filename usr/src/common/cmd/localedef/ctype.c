#ident	"@(#)localedef:ctype.c	1.1.1.1"
#include <sys/types.h>
#include <stdio.h>
#include <locale.h>
#include "_colldata.h"
#include "_localedef.h"

/* wcharm.h is key to  understanding this file */

/* internal version of _lc_ctype_dir structure */
static struct my_lc_ctype {
        unsigned char     *strtab;
        struct t_ctype    *typetab;
        struct t_wctype   *wctype;
        size_t                  nstrtab;
        size_t                  ntypetab;
        size_t                  nwctype;
		unsigned char			codeset;
} lcc;

unsigned char ctype_out[524];
#define	LCC_MAXSTRTAB	4096

/* defined high to avoid compat problems with Japanese use of iswchars */
#define _A	0x20000000
#define _G	0x40000000
static mboolean_t upperflg = FALSE;
static mboolean_t lowerflg = FALSE;

/* save the name of the codeset in the string table */
void
savecodeset(unsigned char *csname,unsigned int length)
{

	lcc.strtab = (unsigned char *) getmem(NULL,LCC_MAXSTRTAB* sizeof(unsigned char),FALSE,TRUE);
		
	/* cannot be at position 0 since a 0 index means it isn't there */
	(void) strncpy((char *)lcc.strtab+1,(char *)csname,length);
	*(lcc.strtab) = *(lcc.strtab+length+1) = '\0';
	lcc.codeset=1;
	lcc.nstrtab=length+2;
}

/* take euc CSWIDTH spec and put it in the magic places in the ctype array */
unsigned char *
setupeuc(unsigned char *line)
{
	int bytes, tmp;
	unsigned char *eucp;
	extern int default_dispwidth;

	line = skipspace(line);
	if(EOL(line)) {
		diag(FATAL,TRUE,":53:EUC requires specification of one or more additional codesets.\n");
	}
	for(eucp=&EUCW1;eucp <= &EUCW3;eucp++) {
		if(sscanf((char *)line,"%d %n",&tmp,&bytes) != 1) {
			diag(FATAL,TRUE,":64:Incorrect cswidth specification for euc encoding.\n");
		}
		if(tmp < 0 || tmp > UCHAR_MAX - 1) 
			diag(FATAL,TRUE,":154:Width specification must be between 0 and %d inclusive\n",UCHAR_MAX-1);

		*eucp = tmp;
		/* second and third codesets values must account for SS2 and SS3 */
		if(eucp != &EUCW1)
			tmp++;
		if(tmp > _MBYTE)
			_MBYTE = tmp;
		line = line+bytes;
		if(*line == ':') {
			line ++;
			if(sscanf((char *)line,"%d %n",&tmp,&bytes) != 1) {
				diag(FATAL,TRUE,":64:Incorrect cswidth specification for euc encoding.\n");
			}
			if(tmp < 0 || tmp > UCHAR_MAX) 
				diag(FATAL,TRUE,":154:Width specification must be between 0 and %d inclusive\n",UCHAR_MAX);
			*(eucp+3) = tmp;
			line = line+bytes;
		}
		else *(eucp+3) = default_dispwidth;
		if(*line != ',')
			break;
		line++;
	}
	if(eucp > &EUCW3) {
		diag(FATAL,TRUE,":64:Incorrect cswidth specification for euc encoding.\n");
	}

	return(line);
}
			
void
write_ctype(FILE *cfile)
{
	struct codent *tcode, *savecode = NULL;
	struct t_wctype *lasttwc;
	int i, j;
	int index=0;
	struct lc_ctype_dir lcd;
	struct codent *space;

	/* localedef only puts out new style files.  So if no encoding directive
		was issued and there were no mb characters, then set encoding to be
		NONE.
	*/
	if(ENCODING == MBENC_OLD) {
		ENCODING = MBENC_NONE;
		_MBYTE = 1;
	}

	/* This is to take care of default rules with respect to
	   toupper and tolower.  If the user has not given a toupper then the
	   default ASCII stuff is used.  Otherwise, the user's stuff is used.  If
	   the user has not given a tolower directive, the opposite of the 
	   upper casing is used.  Otherwise, the user's definition is used. 
	   Having the DEFAULT_DEFN bit on means it is a default definition only.
	   If the user gave a definition for uppercasing from that codepoint, the
	   DEFAULT_DEFN bit is turned off. see kf_conv_func below */
	tcode = strt_codent.cd_codelink;
	while(tcode != NULL) {
		if(upperflg && (tcode->cd_flags & DEFAULT_DEFN))
			tcode->cd_conv = NULL;
		else 
			if(!lowerflg && tcode->cd_conv != NULL) 
				tcode->cd_conv->cd_conv = tcode;

		if(tcode->cd_conv != NULL) {
			if(tcode->cd_flags & TOUPPER) {
				if(!(tcode->cd_classes & _L) || 
					!(tcode->cd_conv->cd_classes & _U)) {
					diag(ERROR,FALSE,":126:Source codepoint (0x%x) of toupper must be lower case and destination (0x%x) must be upper case\n",tcode->cd_code,tcode->cd_conv->cd_code);
				}
			} else {
				if(!(tcode->cd_classes & _U) || 
					!(tcode->cd_conv->cd_classes & _L)) {
					diag(ERROR,FALSE,":125:Source codepoint (0x%x) of tolower must be upper case and destination (0x%x) must be lower case\n",tcode->cd_code,tcode->cd_conv->cd_code);
				}
				
			}
		}

		tcode = tcode->cd_codelink;
	}
				
				
	/* assumes that space will always be there as 
		per default charmap and that value will be 
		same as compiler's notion */
	if((space = findcode(' ')) == NULL)
		diag(FATAL,FALSE,":68:Internal error: space character not present in map\n");
	
	if(space->cd_classes & (_G|_P))
		diag(ERROR,FALSE,":127:Space character may not be in graph or punct classes\n");

	savecode = NULL;
	tcode = strt_codent.cd_codelink;
	for(i = 0; i <= UCHAR_MAX; i++) {
		if(tcode->cd_code == i) {
			unsigned long cclass = tcode->cd_classes;
			/* save lowest eight bit character in case needed to populate
				wctype structures as below */
			if(savecode == NULL & i > CHAR_MAX)
				savecode = tcode;
			if(cclass & _A && !(cclass & (_U|_L))) {
				diag(LIMITS,FALSE,":33:Character 0x%x in class alpha must also be in one of classes upper or lower\n",i);
			}
			else
			if(cclass & _G && !(cclass & (_P|_U|_L|_N))) {
				diag(LIMITS,FALSE,":35:Character 0x%x in class graph must also be in one of classes punct, upper, lower, or digit\n",i);
			}
			else
			if(cclass & _E6 && !(cclass & (_P | _U | _L | _N )) && tcode != space) {
				diag(LIMITS,FALSE,":36:Character 0x%x in class print must also be in one of classes punct, digit, blank, upper, or lower\n",i);
			}
			ctype_out[i+1] = cclass;
			/* 
			constant is the oring of all _E bits but 
			not _A|_G|_E6*/
			if((cclass & 0x1fffdf00))
				diag(LIMITS,TRUE,":16:Cannot use classes with extended bits for single byte character values\n");
			if(tcode->cd_conv != NULL) {
				ctype_out[258+i] = tcode->cd_conv->cd_code;
			}
			else ctype_out[i+258] = tcode->cd_code;
			tcode = tcode->cd_codelink;
		}
		else ctype_out[i+1] = ctype_out[i+258] = 0;
	}
	if(fwrite(&ctype_out,sizeof(unsigned char),sizeof(ctype_out),  cfile) != sizeof(ctype_out)) {
			diag(ERROR, FALSE, ":21:Cannot write ctype array\n");
			return;
	}
	index+=sizeof(ctype_out);

	if(fseek(cfile,sizeof(struct lc_ctype_dir),SEEK_CUR) != 0) {
		diag(ERROR,FALSE,":14:Cannot seek on ctype file\n");
		return;
	}
	index+= sizeof(struct lc_ctype_dir);

	/* This allows inclusion of eight bit characters in wctype structure (as
		well as in the ctype array.  This duplication simplifies some library
		code (wcwidth) and does not cost much */
	if(ENCODING != MBENC_NONE && savecode != NULL)
		tcode = savecode;

	savecode = tcode;


	if(ENCODING == MBENC_EUC) {
		char used[3];

		used[0] = used[1] = used[2] = 0;
		lcc.wctype = (struct t_wctype *) getmem(NULL,sizeof(struct t_wctype)*3,TRUE,FALSE);
		lcc.nwctype = 3; /* tentatively */
		lasttwc = &lcc.wctype[0];
		while(tcode != NULL) {
			if((lasttwc->tmin & EUCMASK) != (tcode->cd_code & EUCMASK)) {
				switch((tcode->cd_code >> EUCDOWN) & DOWNMSK) {
					case DOWNP11:
						used[2] = 1;
						lasttwc = &lcc.wctype[2];
						lasttwc->dispw = SCRW1;
						break;
					case DOWNP01:
						used[0] = 1;
						lasttwc = &lcc.wctype[0];
						lasttwc->dispw = SCRW2;
						break;
					case DOWNP10:
						used[1] = 1;
						lasttwc = &lcc.wctype[1];
						lasttwc->dispw = SCRW3;
						break;
					default:
						diag(FATAL_LIMITS,FALSE,":66:Internal error: euc wide character without EUCMASK: 0x%x.\n",tcode->cd_code);
				}
			}
			if(lasttwc->tmin == 0) {
				lasttwc->tmin = tcode->cd_code;
			}
			lasttwc->tmax = tcode->cd_code;
			if(tcode->cd_conv != NULL) {
				if(lasttwc->cmin == 0) 
					lasttwc->cmin = tcode->cd_code;
				lasttwc->cmax = tcode->cd_code;
			}
			tcode = tcode->cd_codelink;
		}
		/* Trim down to the codesets actually present. */
		if (!used[0]) {
			lcc.nwctype--;
			lcc.wctype++;
			if (!used[1]) {
				lcc.nwctype--;
				lcc.wctype++;
				if (!used[2]) {
					diag(FATAL_LIMITS,FALSE,":156:Internal error: no EUC codesets used!\n");
				}
			} else if (!used[2]) {
				lcc.nwctype--;
			}
		} else if (!used[2]) {
			lcc.nwctype--;
			if (!used[1]) {
				lcc.nwctype--;
			}
		} else if (!used[1]) {
			lcc.nwctype--;
			lcc.wctype[1] = lcc.wctype[2];
		}
	}
	else {
	/* figure out split for UTF-8 encoding */
		struct t_wctype dummy;
		dummy.tmax = 0;
		dummy.cmax = 0;
		lasttwc = &dummy;

		for(;tcode != NULL;tcode = tcode->cd_codelink) {
			/* If the space needed to create if a new t_wctype is created is 
			   less than the space wasted by leaving blanks in the current
			   lasttwc, then make the new t_wctype.  The 2 is a heuristic 
			   guess on how many types from the type array will have to 
			   be duplicated because of the new t_wctype.  (If there were a
			   common type array for all t_wctype, this cost would be 
			   eliminated.) */
			if((sizeof(struct t_wctype) + 2 * sizeof(unsigned long)) <
			   (( tcode->cd_code - lasttwc->tmax) * sizeof(unsigned char)
				+ ((tcode->cd_conv != 0 && lasttwc->cmax != 0) ? 
				   (tcode->cd_code - lasttwc->cmax) * sizeof(wchar_t) : 0))) {
				lcc.wctype = (struct t_wctype *) 
						getmem(lcc.wctype,sizeof(struct t_wctype) * ++lcc.nwctype,FALSE,FALSE);
				lasttwc = &lcc.wctype[lcc.nwctype-1];
				lasttwc->tmin = tcode->cd_code;
				lasttwc->cmin = lasttwc->cmax = 0;
			}		
			lasttwc->tmax = tcode->cd_code;
			if(tcode->cd_conv == NULL)
				continue;
			if(lasttwc->cmin == 0) 
				lasttwc->cmin = tcode->cd_code;
			lasttwc->cmax = tcode->cd_code;

		}
	}
	/* set up auxilliary arrays for character classing and conversion */
	tcode = savecode;
	for(i=0;i < lcc.nwctype; i++) {
		unsigned long itypes = 0;
		unsigned long types[UCHAR_MAX+1];
#define EMPTY	0	/* lasttwc->dispw is unassigned */
#define NOARRAY	1	/* lasttwc->dispw is simple display width */
#define ARRAY	2	/* lasttwc->dispw is pointer to strip */
		unsigned long dispw_indicator = EMPTY;


		lasttwc = &lcc.wctype[i];
		lasttwc->index = (unsigned long) getmem(NULL,(lasttwc->tmax - lasttwc->tmin+1) * sizeof(unsigned char),TRUE,FALSE);
		if(lasttwc->cmin != 0)
			lasttwc->code = (unsigned long) getmem(NULL,(lasttwc->cmax - lasttwc->cmin+1) * sizeof(wchar_t),TRUE,FALSE);
		while(tcode != NULL && tcode->cd_code <= lasttwc->tmax) {
			if(tcode->cd_classes & _G && !(tcode->cd_classes & (_P|_U|_L|_N|_E1|_E2|_E5|_E6))) {
				diag(LIMITS,FALSE,":34:Character 0x%x in class graph must also be in one of classes print, punct, upper, lower, or digit\n",tcode->cd_code);
			}
			else
			if(tcode->cd_classes & _A && !(tcode->cd_classes & (_U|_L))) {
				diag(LIMITS,FALSE,":33:Character 0x%x in class alpha must also be in one of classes upper or lower\n",tcode->cd_code);
			}
			else
				tcode->cd_classes &= ~(_A|_G);
			for(j=0;j < itypes; j++)
				if(tcode->cd_classes == types[j])
					break;
			if(j == itypes) 
				if(itypes++ == UCHAR_MAX+1) {
					diag(LIMITS,FALSE,":117:Overflow of  combinations of character classes\n");
					return;
				}
			*(((unsigned char *)lasttwc->index)+(tcode->cd_code-lasttwc->tmin)) = j;
			types[j] = tcode->cd_classes;
			if(tcode->cd_conv != NULL)
				if(ENCODING == MBENC_EUC)
					*(((wchar_t *)lasttwc->code)+(tcode->cd_code-lasttwc->cmin)) = (tcode->cd_conv->cd_code & ~EUCMASK);
				else
					*(((wchar_t *)lasttwc->code)+(tcode->cd_code-lasttwc->cmin)) = tcode->cd_conv->cd_code;
			if(ENCODING == MBENC_UTF8 && (tcode->cd_classes & _PD_PRINT)) {
				if(dispw_indicator== ARRAY) {
					*(((unsigned char *)lasttwc->dispw)+(tcode->cd_code-lasttwc->tmin)) = tcode->cd_dispwidth;
				} else if(dispw_indicator== NOARRAY) {
					if(lasttwc->dispw != tcode->cd_dispwidth) {
						lasttwc->dispw = (unsigned long) getmem(NULL,lasttwc->tmax - lasttwc->tmin + 1, TRUE, FALSE);
						savecode = findcode(lasttwc->tmin);
						for(;;) {
							if(savecode->cd_classes & _PD_PRINT)
								*(((unsigned char *)lasttwc->dispw)+(savecode->cd_code-lasttwc->tmin)) = savecode->cd_dispwidth;
							if(savecode == tcode)
								break;
							savecode = savecode->cd_codelink;
						}
						dispw_indicator= ARRAY;
					}
				} else {
					lasttwc->dispw = tcode->cd_dispwidth;
					dispw_indicator= NOARRAY;
				}
				
			}
			tcode = tcode->cd_codelink;
		}
		j= lasttwc->tmax-lasttwc->tmin+1;
   	 	if(dispw_indicator == ARRAY) {
			if(fwrite((unsigned char *) lasttwc->dispw,sizeof(unsigned char),j, cfile) != j )  {
				diag(ERROR, FALSE, ":22:Cannot write display width array for t_wctype structure %d\n",i);
				return;
			}
			lasttwc->dispw = index;
			index += j *sizeof(unsigned char);
		}
		j = index % sizeof(unsigned long);
		if(j != 0) {
			j = sizeof(unsigned long) - j;
			dispw_indicator = 0;
			if(fwrite(&dispw_indicator,sizeof(unsigned char),j,cfile) != j) {
				diag(ERROR,FALSE,":25:Cannot write padding\n");
				return;
			}
			index +=j;
		}
		j= lasttwc->tmax-lasttwc->tmin+1;
   	 	if(fwrite((unsigned char *) lasttwc->index,sizeof(unsigned char),j, cfile) != j )  {
			diag(ERROR, FALSE, ":24:Cannot write index array for t_wctype structure %d\n",i);
			return;
		}
		lasttwc->index = index;
		index += j * sizeof(unsigned char);
		j = index % sizeof(unsigned long);
		if(j != 0) {
			j = sizeof(unsigned long) - j;
			dispw_indicator = 0;
			if(fwrite(&dispw_indicator,sizeof(unsigned char),j,cfile) != j) {
				diag(ERROR,FALSE,":25:Cannot write padding\n");
				return;
			}
			index +=j;
		}

   	 	if(fwrite(types,sizeof(unsigned long),itypes, cfile) != itypes )  {
			diag(ERROR, FALSE, ":32:Cannot write type array for t_wctype structure %d\n",i);
			return;
		}
		lasttwc->type = index;
		index += itypes * sizeof(unsigned long);


		if(lasttwc->cmin != 0) {
			j= lasttwc->cmax-lasttwc->cmin+1;
   	 		if(fwrite((wchar_t *) lasttwc->code,sizeof(wchar_t),j, cfile) != j )  {
				diag(ERROR, FALSE, ":20:Cannot write conversion array for t_wctype structure %d\n",i);
				return;
			}
			lasttwc->code = index;
			index += j * sizeof(wchar_t);
		}
	}

	lcd.version = CTYPE_VERSION;
	lcd.codeset = lcc.codeset;
	lcd.nwctype = lcc.nwctype;
	lcd.wctype = index;
   	if(fwrite(lcc.wctype,sizeof(struct t_wctype),lcd.nwctype, cfile) != lcd.nwctype )  {
		diag(ERROR, FALSE, ":31:Cannot write t_wctype structures\n");
		return;
	}
	index += lcd.nwctype * sizeof(struct t_wctype);;

	/* write out arrays for user defined character classes */
	for(i=0;i < lcc.ntypetab;i++) {
		j= 2* (lcc.typetab+i)->npair;
   	 	if(fwrite((wchar_t *)(lcc.typetab+i)->pair,sizeof(wchar_t),j, cfile) != j )  {
			diag(ERROR, FALSE, ":26:Cannot write pair array for t_ctype structure %d\n",i);
			return;
		}
		(lcc.typetab+i)->pair = index;
		index += j * sizeof(wchar_t);
	}

	lcd.ntypetab = lcc.ntypetab;
	lcd.typetab = index;
   	if(fwrite(lcc.typetab,sizeof(struct t_ctype),lcd.ntypetab, cfile) != lcd.ntypetab )  {
		diag(ERROR, FALSE, ":29:Cannot write t_ctype structures\n");
		return;
	}
	index += lcd.ntypetab * sizeof(struct t_ctype);

	lcd.nstrtab = lcc.nstrtab;
	lcd.strtab = index;
  	if(fwrite(lcc.strtab,sizeof(unsigned char),lcd.nstrtab, cfile) != lcd.nstrtab )  {
		diag(ERROR, FALSE, ":28:Cannot write string table\n");
		return;
	}
		
	if(fseek(cfile,sizeof(ctype_out) * sizeof(unsigned char),SEEK_SET) != 0) {
		diag(ERROR,FALSE,":14:Cannot seek on ctype file\n");
		return;
	}

  	if(fwrite(&lcd,sizeof(struct lc_ctype_dir),1, cfile) != 1)  {
		diag(ERROR, FALSE, ":24:Cannot write lc_ctype_dir structure\n");
		return;
	}
}

/* Support routines for kf_class_func to see if character can be
   added to already existing range pair and do appropriate merging
*/
mboolean_t
mergerange(wchar_t *wa)
{
	if(wa[1]+1 >= wa[2]) {
		wa[1] = wa[3];
		return(TRUE);
	}
	return(FALSE);
}

void
copyuprange(wchar_t *wp, wchar_t *endwp)
{
	endwp--;
	while(wp <= endwp) {
		wp[0] = wp[2];
		wp[1] = wp[3];
		wp +=2;
	}
}

mboolean_t
inrange_pair(CODE c, wchar_t *wp, struct t_ctype *tc)
{
	wchar_t *lastwp = (wchar_t *) tc->pair + (tc->npair-1)*2;

	if(c-1 == wp[1]) {
		wp[1]++;
		if(wp != lastwp && mergerange(wp)) {
			copyuprange(wp+2,lastwp);
			tc->npair--;
		}
		return(TRUE);
	}
	if(c+1 == wp[0]) {
		wp[0]--;
		if(wp != (wchar_t *) tc->pair && mergerange(wp-2)) {
			copyuprange(wp,lastwp);
			tc->npair--;
		}
		return(TRUE);
	}
	/* This case should only happen if a codepoint is repeated in the 
		spec for a class. */
	if(c >= wp[0] && c <= wp[1])
		return(TRUE);
	return(FALSE);
}

void
kf_class_func(unsigned char *line, void **args)
{

	struct codent *start_code = NULL;
	struct codent *tcode = NULL;
	wchar_t *wp, *savewp, *endwp;
	unsigned long nwp;
	struct t_ctype *typ = NULL;
	wctype_t x;
	unsigned char *symname;
	unsigned int i;


	line = skipspace(line);
	/* OK to have empty character classification according to spec 
	   It is supposed to mean that no charcaters will be in that class,
	   but this code does not check that the class name is not repeated
	   later on with charcaters. */
	if(EOL(line))
		return;

	/* If this is a classification for a user defined character classification,
	   get the appropriate index into the type array. */
	if((unsigned long)args[0] & XWCTYPE) {
		x = (unsigned long)args[0] & ~XWCTYPE;
		x >>= 8;
		if ((typ = lcc.typetab) == NULL || x >= lcc.ntypetab) 
		{
			diag(ERROR, TRUE, ":67:Internal error in handling character classification\n");
			return;
		}
		typ+=x;
		savewp = (wchar_t *) typ->pair;
	}
 
	/* process all the characters for the type */
	for(;;) {
		/* If it's an ellipsis, then start_code will have the beginning code
		   point and tcode will after another loop have the ending code point. 
		*/
		if(strncmp((char *) line, "...",3) == 0)  {
			if(tcode == NULL) {
				diag(ERROR, TRUE, ":107:No defined starting codepoint for ellipsis in character classification\n");
				return;
			}
			line+=3;
			start_code = tcode;
			tcode = NULL;
		} else {
			if((tcode = getlocchar(&line,';',&symname)) == NULL) {
				if(symname != NULL) {
					diag(WARN,TRUE,":131:Symbolic character %s not found in charmap: Ignored\n",symname);
					/* if unknown symbolic char on end of ellipsis must fail */
					if(start_code != NULL)
						goto eliperr;
					goto skip;
				}
				diag(ERROR, TRUE, ":70:Invalid character to classify\n");
				return;
			}
			/* If not an ellipsis situation start_code will have the codepoint
			   of interest. */
			if(start_code == NULL)
				start_code = tcode;
			else if(start_code->cd_code > tcode->cd_code) {
				diag(ERROR,TRUE,":119:Range start (0x%x) greater than range end (0x%x)\n",start_code->cd_code,tcode->cd_code);
				goto skip;
			}
			do {
				/* handle case of user defined character class */
				if(typ != NULL) {
					if ((nwp = typ->npair) != 0) {
						/* As an optimization 
						   savewp has the last range pair that was used. */
						if(inrange_pair(start_code->cd_code,savewp, typ)  != 0)
							goto middle_loop;
						/* If it cannot be added to the saved range pair,
						   then see if it can be added to another range pair. */
						if(start_code->cd_code > savewp[1]) {
							wp = savewp+2;
							endwp = (wchar_t *) typ->pair+(nwp*2);
						}
						else {
							wp = (wchar_t *)typ->pair;
							endwp = savewp;
						}
						for(;wp<endwp;wp+=2) {
							if(inrange_pair(start_code->cd_code,wp, typ))
								goto middle_loop;
							if(start_code->cd_code < wp[0]) 
								break;
						}
						/* Otherwise, add another range pair in the right place
						   in the set of pairs. */
						i = wp - (wchar_t *)typ->pair;
						typ->pair = (unsigned long) getmem((wchar_t *)typ->pair,++(typ->npair) * 2 * sizeof(wchar_t),FALSE,TRUE);
						wp = ((wchar_t*)typ->pair)+i;
						endwp = ((wchar_t*)typ->pair)+((nwp-1)*2);
						for(;endwp >= wp;endwp-=2) {
							endwp[2] = endwp[0];
							endwp[3] = endwp[1];
						}
						savewp = wp;
					}
					else {
						/* first time around we need to create a range pair */
						typ->pair = (unsigned long) getmem(NULL,++(typ->npair) * 2 * sizeof(wchar_t),FALSE,TRUE);
						savewp = (wchar_t *) typ->pair;
					}
					savewp[0] = savewp[1] = start_code->cd_code;
				}
				else {
					/* in case of predefined class */
					if(start_code->cd_classes & (unsigned long) args[1]) {
						diag(ERROR,TRUE, ":40:Classification conflict for codepoint 0x%x\n",start_code->cd_code);
					}
					else
						start_code->cd_classes |= (unsigned long) args[0];
				}
middle_loop:;
			} while(start_code != tcode && (start_code = start_code->cd_codelink));
			start_code = NULL;
		}
skip:
		line = skipspace(line);
		if(*line != ';')
			break;
		line++;
		line = skipspace(line);
	}
	if(start_code != NULL) {
eliperr:
		diag(ERROR, TRUE, ":62:Incomplete ellipsis specification\n");
		return;
	}
	if(!EOL(line))
		diag(WARN, TRUE, extra_char_warn);


}

void
kf_conv_func(unsigned char *line, void **args)
{
	struct codent *fromcode, *tocode = NULL;
	unsigned char *symname;
	mboolean_t unkcodeflg;

	/* If this is a user definition (as opposed to system defaults) set
	   the flag to show the user has specified  this kind of conversion.
	   see below and write_ctype for more info. */
	if(!softdefn)
		*((mboolean_t *) args[0]) = TRUE;

	for(;;) {
		line = skipspace(line);
		if(*line != '(') {
			diag(ERROR, TRUE, ":82:Invalid syntax specifying character conversion\n");
			return;
		}
		line++;
		line = skipspace(line);
		unkcodeflg = FALSE;
		if((fromcode = getlocchar(&line,',',&symname)) == NULL) {
			if(symname != NULL) {
				diag(WARN,TRUE,":130:Symbolic character %s not found in charmap: conversion specification ignored\n",symname);
				unkcodeflg = TRUE;
			} else {
				diag(ERROR,TRUE,":71:Invalid character used in specifying character conversion\n");
				return;
			}
		}
		line = skipspace(line);
		if(*line != ',') {
			diag(ERROR, TRUE, ":82:Invalid syntax specifying character conversion\n");
			return;
		}
		line++;
		line = skipspace(line);
		if((tocode = getlocchar(&line,')',&symname)) == NULL) {
			if(symname != NULL) {
				diag(WARN,TRUE,":130:Symbolic character %s not found in charmap: conversion specification ignored\n",symname);
				unkcodeflg = TRUE;
			} else {
				diag(ERROR,TRUE,":71:Invalid character used in specifying character conversion\n");
				return;
			}
		}
		
		if(!unkcodeflg) {
			if(ENCODING == MBENC_EUC && 
				(fromcode->cd_code & EUCMASK) != (tocode->cd_code & EUCMASK)) {
					diag(LIMITS,TRUE,":124:Source and Destination codepoints must be in the same EUC codeset\n");
			}
			if(fromcode->cd_code <= UCHAR_MAX && tocode->cd_code > UCHAR_MAX) {
				diag(LIMITS,FALSE,":38:Character conversion for codepoint 0x%x out of range: must be single byte character\n",fromcode->cd_code);
			}

			/* if first definition or only default definition previously ... */
			if(fromcode->cd_conv == NULL || fromcode->cd_flags & DEFAULT_DEFN) {
				/* If this is system default, set the flag. Otherwise, remove
				   the flag if it was there. */
				if(softdefn)
					fromcode->cd_flags |= DEFAULT_DEFN;
				else
					fromcode->cd_flags &= ~DEFAULT_DEFN;
				fromcode->cd_flags |= (unsigned long) args[1];
				fromcode->cd_conv = tocode;
			} else {
				diag(ERROR,TRUE,":120:Redefinition of conversion for codepoint 0x%x\n",fromcode->cd_code);
			}

		}
		line = skipspace(line);
		if(*line != ')') {
			diag(ERROR, TRUE, ":82:Invalid syntax specifying character conversion\n");
			return;
		}
		line++;
		line = skipspace(line);
		if(*line != ';')
			break;
		line++;
	}
	if(!unkcodeflg && tocode == NULL) {
		diag(ERROR, TRUE, ":46:Empty character conversion statement\n");
		return;
	}
	if(!EOL(line))
		diag(WARN, TRUE, extra_char_warn);

}


void
kf_charclass_func(unsigned char *line, void **args)
{
	unsigned char *tline,*tline2;
	unsigned char savechar;
	extern struct kf_hdr kfh_ctype;
	int i;
	int res;
	static struct keyword_func *new_table = NULL;
	struct keyword_func *tkf;
	static unsigned long cclass = 0;
	unsigned long tcclass;
	mboolean_t qflag;

	if(lcc.strtab == NULL) {
		lcc.strtab = (unsigned char *) getmem(NULL,LCC_MAXSTRTAB* sizeof(unsigned char),FALSE,FALSE);
		lcc.nstrtab = 1;
	}

	for (;;) {
		line = skipspace(line);
		tline = line;
		if(*tline == '"') {
			line = tline++;
			qflag = TRUE;
		}
		else
			qflag = FALSE;

		while(isalnum(*tline))
			tline++;
		if(line == tline || !isalpha(*line) || tline-line > CHARCLASS_NAME_MAX) {
			diag(ERROR, TRUE, ":79:Invalid or missing argument to charclass\n");
			return;
		}
		tline2 = tline;
		if(qflag)
			if(*tline == '"')
				tline++;
			else {
				diag(ERROR,TRUE,":146:Unmatched quotation mark\n");
			}
		if(*tline == ':') {
			tline++;
			if(sscanf((char *)tline,"%x%n",&tcclass,&i) != 1) {
				diag(ERROR,TRUE,":69:Invalid character class value\n");
				tcclass = 0;
			}
			else {
				tline +=i;
				if(tcclass & XWCTYPE) {
					diag(ERROR,TRUE,":37:Character class value may not have the following bit set: %#x\n");
					tcclass = 0;
				}
			}
		}
		else tcclass = 0;
		savechar = *tline2;
		*tline2 = '\0';
		for(i=0; i< kfh_ctype.kfh_numkf; i++) {
			if((res = strcmp(kfh_ctype.kfh_arraykf[i].kf_keyword, (char *)line)) > 0 ) {
				break;
			}
			if(res == 0) 
				diag(ERROR, TRUE, ":10:Cannot name new character class using existing LC_CTYPE keyword: %s\n",kfh_ctype.kfh_arraykf[i].kf_keyword);
					
		}
		if(cclass > CLASS_MAX) {
			diag(LIMITS,TRUE,":133:Too many charclass specifiers\n");
			return;
		}
		if(tcclass == 0) {
			tcclass = (XWCTYPE | cclass);
		}
			
		/* make a new table entry for future parsing */
		tkf = (struct keyword_func *) getmem(NULL,sizeof(struct keyword_func),FALSE,TRUE);
		tkf->kf_keyword = (char *) (lcc.strtab + lcc.nstrtab);
		tkf->kf_func = kf_class_func;
		tkf->kf_list = (void **) getmem(NULL,sizeof(void *) * 2,FALSE,TRUE);
		tkf->kf_list[0] = (void *) (tcclass);
		tkf->kf_list[1] = NULL;
		cclass+=CLASS_INC;

		/* General scheme is to add new table entry to parsing table in 
		   correct place alphabetically by reallocing and copying as necessary.
		   The first time however we need to copy the whole array into the 
		   malloced  table. */
		if(new_table == NULL) {
			new_table = (struct keyword_func *) getmem(NULL,sizeof(struct keyword_func)*
						++kfh_ctype.kfh_numkf,FALSE,TRUE);

			for(res=0; res <i; res++)
				new_table[res] = kfh_ctype.kfh_arraykf[res];
		}
		else {
			new_table = (struct keyword_func *)getmem(new_table,sizeof(struct keyword_func)*
				++(kfh_ctype.kfh_numkf),FALSE,TRUE);
			kfh_ctype.kfh_arraykf = new_table;
		}
		for(res=kfh_ctype.kfh_numkf-1;res > i; res--) 
			new_table[res] = kfh_ctype.kfh_arraykf[res-1];
		new_table[res]= *tkf;
		kfh_ctype.kfh_arraykf = new_table;

		if(strlen((char *)line) + lcc.nstrtab + 1 > LCC_MAXSTRTAB) {
			diag(LIMITS,TRUE,":134:Too many user defined character classes\n");
			return;
		}
		(void) strcpy((char*) (lcc.strtab + lcc.nstrtab),(char *) line);
		/* create a new entry in the type table */
		lcc.typetab = (struct t_ctype *) getmem(lcc.typetab,++lcc.ntypetab * sizeof(struct t_ctype),FALSE,TRUE);

		lcc.typetab[lcc.ntypetab-1].type = tcclass;
		lcc.typetab[lcc.ntypetab-1].name = lcc.nstrtab;
		lcc.typetab[lcc.ntypetab-1].npair = 0;
		lcc.nstrtab += strlen((char *)line)+1;

		*tline2 = savechar;
		tline=skipspace(tline);
		if(*tline != ';') 
			break;
		line=++tline;
	}
	if(!EOL(tline))
		diag(WARN, TRUE, extra_char_warn);

}

	
			

extern void *kf_ctype_list[];
/* For classes alpha, graph: 
	- The implementation does not support specifying these independent
	    of some underlying classes (e.g., alpha is exactly the combination
	    of upper and lower). See ctype.h for more details.
	- To keep track here, artificial classes (i.e., _A, _G) are
	    used. 
	- These artifical classes are used in checking if any charcater has
	    this class but not one of its underlying classes.  If so, an error
	    is generated.
For class print, characters less than 255 must have one of underlying classes 
set.  For multibyte characters, _E6 is used to indicate printability.  This is 
compatible with Japanese use previously.
*/
void *kf_alpha_list[] = { (void *) (_A|_G|_E6), (void *) (_C|_N|_P|_S|_B)};
void *kf_blank_list[] = { (void *) (_B|_S), (void *) (_U|_L|_A|_N|_X)};
void *kf_cntrl_list[] = { (void *) (_C), (void *) (_U|_L|_A|_N|_P|_X|_G|_E6)};
void *kf_digit_list[] = { (void *) (_X|_G|_N|_E6), (void *) (_A|_C|_L|_U|_S|_P|_B)};
void *kf_graph_list[] = { (void *) (_G|_E6), (void *) _C};
void *kf_lower_list[] = { (void *) (_G|_L|_A|_E6), (void *) (_C|_N|_P|_S|_B)};
void *kf_print_list[] = { (void *) (_E6), (void *) _C};
void *kf_punct_list[] = { (void *) (_P|_E6|_G), (void *) (_C|_U|_L|_A|_N|_X)};
void *kf_space_list[] = { (void *) (_S), (void *) (_U|_L|_A|_N|_X)};
void *kf_upper_list[] = { (void *) (_G|_U|_A|_E6), (void *) (_C|_N|_P|_S|_B)};
void *kf_xdigit_list[] = { (void *) (_G|_X|_E6), (void *) (_C|_P|_S|_B)};
void *kf_toupper_list[] = { (void *) &upperflg, (void *) TOUPPER};
void *kf_tolower_list[] = { (void *) &lowerflg, (void *) 0};

struct keyword_func kf_ctype_tab[] = {
	{ "END", kf_END_func, kf_ctype_list },
	{ "alpha", kf_class_func, kf_alpha_list},
	{ "blank", kf_class_func, kf_blank_list},
	{ "charclass", kf_charclass_func, NULL },
	{ "cntrl", kf_class_func, kf_cntrl_list},
	{ "copy", kf_copy_func, kf_ctype_list},
	{ "digit", kf_class_func, kf_digit_list},
	{ "graph", kf_class_func, kf_graph_list},
	{ "lower", kf_class_func, kf_lower_list},
	{ "print", kf_class_func, kf_print_list},
	{ "punct", kf_class_func, kf_punct_list},
	{ "space", kf_class_func, kf_space_list},
	{ "tolower", kf_conv_func, kf_tolower_list},
	{ "toupper", kf_conv_func, kf_toupper_list},
	{ "upper", kf_class_func, kf_upper_list},
	{ "xdigit", kf_class_func, kf_xdigit_list}
};

struct kf_hdr kfh_ctype = {
	16,
	kf_ctype_tab
};

void *kf_ctype_list[] = { (void *) "LC_CTYPE", (void *)&kfh_ctype, (void *)LC_CTYPE, (void *) "LC_CTYPE", (void *) write_ctype };
