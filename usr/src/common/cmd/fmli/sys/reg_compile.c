/*		copyright	"%c%" 	*/

#ident	"@(#)fmli:sys/reg_compile.c	1.1"

#ifdef __STDC__
	#pragma weak nbra = _nbra
	#pragma weak regerrno = _regerrno
	#pragma weak reglength = _reglength
	#pragma weak compile = __compile
#endif
#include "synonyms.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include "_wchar.h"
#include "_range.h"
#include "_regexp.h"
/** I18N CODE **/
#include <locale.h>
#include "_locale.h"
/** END I18N CODE **/

#define GETC() ((unsigned char)*sp++)
#define PEEKC() ((unsigned char)*sp)
#define ERROR(c) { \
		 	regerrno = c; \
			goto out; \
		    }
#define Popwchar	if((n = mbtowc(&cl, sp, MB_LEN_MAX)) == -1) \
				ERROR(67) \
			oldsp = sp; \
			sp += n; \
			c = cl;
/** I18N CODE **/
/*  Function to convert a single character into its primary
 *  and secondary collation weights
 */
#define ccol(a, b, c) \
	tmp1[0] = a; \
	strxfrm(tmp2, tmp1, 3); \
	b = (unsigned char)tmp2[0]; \
	c = (unsigned char)tmp2[1];

/*  Variable for locale name and function for MCCE's  */
char __mcce_l_name[LC_NAMELEN] = "";

void __mcce_init();
/** END I18N CODE **/

int	nbra = 0, regerrno = 0, reglength = 0;

static unsigned char	_bittab[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
char *_compile();
char *strchr();

char *
compile(sp, ep, endbuf)
const char *sp;
char *ep, *endbuf;
{
	return _compile(sp, ep, endbuf, 0);
}

char *
_compile(sp, ep, endbuf, viflag)
register const char *sp; 
register char *ep;
char *endbuf;
int viflag;
{
	register wchar_t c;
	register int n;
	wchar_t d;
	register const char *oldsp;
	char *lastep;
	int cclcnt;
	char bracket[NBRA], *bracketp;
	int closed;
	int neg;
	int alloc;
	wchar_t lc, lc_s;
	int i, cflg;
	char *expbuf = ep;
	register char *start;

	/** I18N CODE **/
	int mcce_flag;	/*  MCCE is present  */
	register const char *optr;	/*  Saved lp pointer  */
	char tmp1[2];	/*  Source string for strxfrm  */
	char tmp2[3];	/*  Transformed string  */
	char *mcce_ep;	/*  End pointer for MCCE's  */
	wchar_t c_xf;	/*  Prim. collation weight of c  */
	wchar_t c_s;		/*  Sec. collation weight of c  */
	wchar_t cl;		/*  Can't take the address of a register  */

	tmp1[1] = NULL;	/*  Set NULLs here so we only do it once  */
	tmp2[2] = NULL;

	/*  The __mcce_init routine should only need to be called once, after
	 *  the first call to compile in a new locale.  Putting the test of 
	 *  __mcce_l_name here instead of in __mcce_init saves on the overhead
	 *  of making an extra function call.
	 */
	if (strcmp(_cur_locale[LC_COLLATE], __mcce_l_name) != 0)
		__mcce_init();
	/** END I18N CODE **/

	regerrno = 0;
	reglength = 0;
	lastep = 0;
	bracketp = bracket;
	closed = 0;
	alloc = 0;
	
	oldsp = sp;
	if((c = *sp++) == '\0' ) {
		if(ep == (char *)0 || ep[1] == 0)
			ERROR(41);
		goto out;
	}
	nbra = 0;	
	if(ep == (char *)0) {
		/* malloc space */
		register const char *startsp = oldsp;
		n = 0;
		while(d = *startsp++) {
			if(d == '[')
				n += 33; /* add room for bitmaps */
		}
		n += 2 * (startsp - oldsp) + 3;
		if((ep = malloc(n)) == (char *)0)
			ERROR(50);
		expbuf = ep;
		alloc = 1;
		endbuf = ep + n;
	}
	
	if(c == '^')
		*ep++ = 1;
	else  {
		*ep++ = 0; 
		sp--;
	}

	endbuf--; /* avoid extra check for overflow */
	while(1) {
		if(ep >= endbuf)
			ERROR(50);
		Popwchar
		if(c != '*' && ((c != '\\') || (PEEKC() != '{')))
			lastep = ep;
		if(c == '\0') {
			*ep++ = CCEOF;
			if (bracketp != bracket)
				ERROR(42);
			goto out;
		}
		switch(c) {

		case '.':
			*ep++ = CDOT;
			continue;

		case '*':
			if(lastep == 0 || *lastep == CBRA || *lastep == CKET || *lastep == CBRC || *lastep == CLET)
				goto defchar;
			*lastep |= STAR;
			continue;

		case '$':
			/* look one character ahead to see if $ means
			   to anchor match at end of line */
			if((d = PEEKC()) != '\0')
				goto defchar;
			*ep++ = CDOL;
			continue;

		case '[':
			/** I18N CODE **/
			mcce_ep = NULL;		/*  Init variables for MCCE's  */
			mcce_flag = 0;
			/** END I18N CODE **/
			start = ep + 34;
			if(start > endbuf)
				ERROR(50);

			*ep++ = CCL;
			lc = 0;
			for(i = 0; i < 32; i++)
				ep[i] = 0;
				
			neg = 0;
			Popwchar
			if(c == '^') {
				neg = 1;
				Popwchar
			}
			if(multibyte) {
				if(neg) {
					/* do not negate bitmap for 
					   for multibyte characters */
					neg = 0;
					ep[-1] = NMCCL;
					/* turn off null byte */
					ep[0] |= 01; 
				} else {
					if (ep[-1] != NMCCL)
						ep[-1] = MCCL;
				}
			}
			do {
				if(c == '\0')
					ERROR(49);
				if(c == '-' && lc != 0) {
					Popwchar
					if(c == '\0')
						ERROR(49);
					if(c == ']') {
						PLACE('-');
						break;
					}
					/** I18N CODE **/
					/*  XPG3 extensions for I18N allow character classes,
					 *  equivalence classes and multi-character collating
					 *  elements (MCCE's) inside rannges.
					 */
					if (c == '[')
					{
						optr = sp;	/*  Save current position  */
						Popwchar;	/*  Lose the [  */

						/*  Check that the syntax is valid  */
						if (c == '\0')
							ERROR(49);
						if (c == ']')	/*  We're really looking for a [  */
						{
							c = '[';
							sp = optr;
						}
						else if (c == '=')	/*  Equivalence class  */
						{
							char test_mcce[3];
							char res_mcce[5];

							test_mcce[0] = 0;
							Popwchar;	/*  Lose =  */
							
							/*  Extra check for MCCE  */
							if (*sp != '=')
							{
								test_mcce[0] = c;
								Popwchar;
							}

							/*  Check for valid syntax  */
							if (*sp != '=' || *(sp+1) != ']')
							{
								c = '[';
								sp = optr;
							}
							else
							{
								/*  Convert the character to its 
								 *  collation weights 
								 */
								if (test_mcce[0] != 0)
								{
									test_mcce[1] = c;
									test_mcce[2] = NULL;

									if (__is_mcce(test_mcce, res_mcce) == 0)
										ERROR(48);

									c_xf = res_mcce[0];
								}
								else
								{
									ccol(c, c_xf, c_s);
								}

								/*  Set the secondary weight to -1 to
								 *  to indicate we don't care
								 */
								c_s = -1;
								Popwchar;
								Popwchar;
							}
						}
						else if (c == '.')		/*  MCCE  */
						{
							int n;	/*  Local vartiables  */

							char test_mcce[3];
							char res_mcce[5];

							Popwchar;	/*  Lose the .  */
							i = 0;

							/*  Extract the MCCE  */
							while (c != '.' && c != NULL && i < 2)
							{
								test_mcce[i] = c;
								Popwchar;
								i++;
							}

							/*  Only two character MCCE's are allowed  */
							if (i != 2)
								ERROR(48);

							/*  Must end with .]  */
							if (c != '.' || *sp != ']')
							{
								c = '[';
								sp = optr;
							}
							else
							{
								if (neg)
								{
									/*  We switch off neg here, so that the bit
									 *  map does not get inverted afterwards.
									 */
									neg = 0;
									ep[-1] = NMCCE;
								}
								else
								{
									if (ep[-1] != NMCCE)
											ep[-1] = MCCE;
								}

								if (__is_mcce(test_mcce, res_mcce) == 0)
									ERROR(48);

								/*  Record the primary and secondary
								 *  collation weights.
								 */
								c_xf = res_mcce[0];
								c_s = res_mcce[1];
								*start++ = c_xf;
								*start++ = c_s;
								mcce_flag = 1;
								mcce_ep = start;
								*(ep + 32) = (start - ep - 32);
								Popwchar;
							}
						}
					}
					/*  Normal character - we still record the 
					 *  collation weights for use when evaluating the 
					 *  range.
					 */
					else
					{
						ccol(c, c_xf, c_s);
					}
					/** END I18N CODE **/
								
					/*
					 * ranges do not span code sets 
					 */
					if(!multibyte || c <= 0177)
					{
						/** I18N CODE **/
						/*  For I18N we have to evaluate the range in 
						 *  terms of collation value rather than the 
						 *  characters.  We also need to detect whether
						 *  there are any MCCE's within this range, and
						 *  then add them to the list.
						 */
						register wchar_t i;
						register wchar_t i_xf; 
						register wchar_t i_s;

						char mcce_bmap[32];

						/*  Clear collation bit map  */
						for (i = 0; i <= 32; i++)
							mcce_bmap[i] = 0;

						/*  Mark all collation weights in the range  */
						for (i = lc; i <= c_xf; i++)
							M_PLACE(i);
						/*  Now go through all 256 chars looking for ones
						 *  which match our range
						 */
						for (i = 0; i <= 0xFF; i++)
						{
							/*  Need to compare collation weights  */
							ccol(i, i_xf, i_s);

							/*  Collation weight is in our range  */
							if (i_xf >= lc && i_xf <= c_xf)
							{
								/*  Clear the collation weight from
								 *  the bit map.
								 */
								M_UNPLACE(i_xf);

								/*  End points of the range need to be
								 *  treated carefully as equivalence 
								 *  classes match all chars with this
								 *  primary weight.
								 */
								if (i_xf == c_xf)
								{
									if (c_s == -1 || c_s == i_s)
										PLACE(i);
								}
								else if (i_xf == lc)
								{
									if (lc_s == -1 || lc_s == i_s)
										PLACE(i);
								}
								else
									PLACE(i);
							}
						}

						/*  We now scan the bit map to see if any 
						 *  collation weights were not found.  We 
						 *  assume that these are MCCE's, and so we
						 *  add them to the end of the bit map.
						 */
						/*  XXX - Not really a bug, but if a MCCE is
						 *  specified as the start or end point of the
						 *  list it gets added to the compiled string
						 *  twice.
						 */
						for (i = 0; i <= 0xFF; i++)
						{
							if (M_ISTHERE(i))
							{
								/*  Again switch off neg to avoid
								 *  getting the bit map inverted.
								 */
								if (neg)
								{
									neg = 0;
									ep[-1] = NMCCE;
								}
								else
								{
									if (ep[-1] != NMCCE)
										ep[-1] = MCCE;
								}

								*start++ = i;
								*start++ = -1;
								*(ep + 32) = (start - ep - 32);
								mcce_ep = start;
							}
						}
						/** END I18N CODE **/

						Popwchar;
						continue;
					}
					else if(valid_range(lc, c) && lc < c)
						/* insert '-' for range */
						*start++ = '-';
					if(viflag & 1)
						lc = 0;
					else
					{
						/** I18N CODE **/
						/*  Remember the collation weights  */
						if (mcce_flag == 1)
						{
							lc = c_xf;
							lc_s = c_s;
						}
						else if (!multibyte)
						{
							ccol(c, lc, lc_s);
						}
						else
							lc = c;
					}
					/** END I18N CODE **/
				} else if(c == '\\' && (viflag & 1) &&
					strchr("\\^-]", PEEKC())) {
					c = GETC();
					/** I18N CODE **/
					ccol(c, lc, lc_s);
				} 
				else if (c == '[') {
					Popwchar;
					optr = sp;

					if (c == ':')	/*  Character class  */
					{
						register int i = 0;
						int bmap;

						char ch_class[7];

						/*  The class can only be specified using
						 *  lower case characters.
						 */
						while (islower(*sp) && i < 6)
							ch_class[i++] = *sp++;

						ch_class[i] = NULL;

						if (*sp != ':' || *(sp+1) != ']')
						{
							sp = optr;
							c = '[';
						}
						else
						{
							/*  Verify the class, and get the bit
							 *  mask for this class.
							 */
							if ((bmap = class_chk(ch_class)) == 0)
								ERROR(48);

							/*  Loop through all 256 chars and find
							 *  those that match
							 */
							for (i = 0; i <= 0xFF; i++)
								if (((__ctype + 1)[i] & bmap) != 0)
									PLACE(i);

							Popwchar;
							Popwchar;
							Popwchar;
							lc = 0;		/*  Can't be used as start of range  */
							continue;
						}
					}
					else if (c == '=')	/*  Equivalence class  */
					{
						register int i;

						char i_xf;
						char i_s;
						char test_mcce[3];
						char res_mcce[5];

						test_mcce[0] = 0;
						Popwchar;

						/*  Extra test for MCCE  */
						if (*sp != '=')
						{
							test_mcce[0] = c;
							Popwchar;
						}

						if (*sp != '=' || *(sp+1) != ']')
						{
							sp = optr;
							c = '[';
						}
						else
						{
							/*  Convert the character to its 
							 *  collation weights 
							 */
							if (test_mcce[0] != 0)
							{
								test_mcce[1] = c;
								test_mcce[2] = NULL;

								if (__is_mcce(test_mcce, res_mcce) == 0)
									ERROR(48);

								lc = res_mcce[0];

								if (neg)
								{
									neg = 0;
									ep[-1] = NMCCE;
								}
								else
								{
									if (ep[-1] != NMCCE)
										ep[-1] = MCCE;
								}

								*start++ = res_mcce[0];
								*start++ = -1;
								*(ep + 32) = (start - ep - 32);
								mcce_ep = start;
							}
							else
							{
								ccol(c, lc, lc_s);

								if (lc == 0)
									ERROR(48);

								/*  Look for all characters with this primary
								 *  collation weight 
								 */
								for (i = 0; i <= 0xFF; i++)
								{
									ccol(i, i_xf, i_s);

									if (i_xf == lc)
										PLACE(i);
								}
							}
						}

						/*  Set lc_s as don't care  */
						lc_s == -1;
						Popwchar;
						Popwchar;
						Popwchar;
						continue;
					}
					else if (c == '.')	/*  MCCE  */
					{
						int n;

						char test_mcce[3];
						char res_mcce[5];

						if (neg)
						{
							neg = 0;
							ep[-1] = NMCCE;
						}
						else
						{
							if (ep[-1] != NMCCE)
								ep[-1] = MCCE;
						}

						i = 0;
						Popwchar;

						while (c != '.' && c != NULL && i < 2)
						{
							test_mcce[i] = c;
							Popwchar;
							i++;
						}

						if (i != 2)
							ERROR(48);

						if (c != '.' || *sp != ']')
							ERROR(48);

						if (__is_mcce(test_mcce, res_mcce) == 0)
							ERROR(48);

						*start++ = res_mcce[0];
						*start++ = res_mcce[1];
						Popwchar;
						Popwchar;
						*(ep + 32) = (start - ep - 32);
						mcce_ep = start;
						lc = res_mcce[0];
						lc_s = res_mcce[1];
						continue;
					}
				} else {
					if (!multibyte)
					{
						ccol(c, lc, lc_s);
					}
					else
						lc = c;
				}
				/** END I18N CODE **/

				/* put eight bit characters into bitmap */
				if(!multibyte || c <= 0177 || c <= 0377 && iscntrl(c)) 
					 PLACE(c);
				else {
					/*
					 * insert individual bytes of 
					 * multibyte characters after 
					 * bitmap
					 */
					if(start + n > endbuf)
						ERROR(50);
					while(n--)
						*start++ = *oldsp++;
				}
				Popwchar
			} while(c != ']');
			
			if(neg) {
				for(cclcnt = 0; cclcnt < 32; cclcnt++)
					ep[cclcnt] ^= 0377;
				ep[0] &= 0376;
			}

			/** I18N CODE **/
			/*  Advance the ep pointer depemding on whether we had an 
			 *  MCCE or not
			 */
			if (mcce_ep == NULL)
				ep += 32;
			else
				ep = mcce_ep;
			/** END I18N CODE **/

			if(multibyte) {
				/*
				 * Only allow 256 bytes to
				 * represent multibyte characters
				 * character class
				 */
				if(start - ep > 255)
					ERROR(50);
				*ep = start - ep;
				ep = start;
			}
			continue;

		case '\\':
			Popwchar
			switch(c) {

			case '(':
				if(nbra >= NBRA)
					ERROR(43);
				*bracketp++ = nbra;
				*ep++ = CBRA;
				*ep++ = nbra++;
				continue;

			case ')':
				if(bracketp <= bracket) 
					ERROR(42);
				*ep++ = CKET;
				*ep++ = *--bracketp;
				closed++;
				continue;

			case '{':
				if(lastep == (char *) 0)
					goto defchar;
				*lastep |= RNGE;
				cflg = 0;
				c = GETC();
			nlim:
				i = 0;
				do {
					if('0' <= c && c <= '9')
						i = 10 * i + c - '0';
					else
						ERROR(16);
				} while(((c = GETC()) != '\\') && (c != ','));
				if(i > 255)
					ERROR(11);
				*ep++ = i;
				if(c == ',') {
					if(cflg++)
						ERROR(44);
					if((c = GETC()) == '\\')
						*ep++ = 255;
					else
						goto nlim;
						/* get 2'nd number */
				}
				if(GETC() != '}')
					ERROR(45);
				if(!cflg)	/* one number */
					*ep++ = i;
				else if((int)(unsigned char)ep[-1] < (int)(unsigned char)ep[-2])
					ERROR(46);
				continue;

			case 'n':
				c = '\n';
				goto defchar;

			case '<':
				*ep++ = CBRC;
				continue;
			
			case '>':
				*ep++ = CLET;
				continue;

			default:
				if(c >= '1' && c <= '9') {
					if((c -= '1') >= closed)
						ERROR(25);
					*ep++ = CBACK;
					*ep++ = c;
					continue;
				}
			}
				
	/* Drop through to default to use \ to turn off special chars */

		defchar:
		default:
			lastep = ep;
			if(!multibyte || c <= 0177) {
				/* 8-bit character */
				*ep++ = CCHR;
				*ep++ = c;
			} else {
				/* multibyte character */
				*ep++ = MCCHR;
				if(ep + n > endbuf)
					ERROR(50);
				while(n--)
					*ep++ = *oldsp++;
			}
		}
	}
out:
	if(regerrno) {
		if(alloc)
			free(expbuf);
		return (char *)0;
	}
	reglength = ep - expbuf;
	if(alloc)
{
register int i;

#ifdef DEBUG
for (i = 0; i < reglength; i++)
{
	printf("%X ", *(expbuf+i) & 0xFF);
}
printf("\n");
#endif
		return expbuf;
}
	return(ep);
}

/** I18N CODE **/
/*
 *  Faster function than strcmp to check whether two strings are equal.
 *  We're not interested in the differences, so we just zip down the strings
 *  until we find a difference or a NULL
 */
static
int 
strcheck(a, b)
char *a;
char *b;
{
	/*  Only check for *a being null, we don't need to check *b,
	 *  as the first part will fail if *b is null and *a is not
	 */
	while (*a == *b && *a != NULL)
	{
		++a;
		++b;
	}

	/*  If *a and *b are not both null then the test fails
	 */
	if (*a == NULL && *b == NULL)
		return(0);
	return(1);
}


/*  Function that checks the name of the character class that the user
 *  supplies, and returns an appropriate bit map to extract these 
 *  characters.
 */
static
int
class_chk(ch_class)
char *ch_class;
{
	if (strcheck(ch_class, "lower") == 0)
		return(_L);
	else if (strcheck(ch_class, "alnum") == 0)
		return(_U|_L|_N);
	else if (strcheck(ch_class, "alpha") == 0)
		return(_U|_L);
	else if (strcheck(ch_class, "digit") == 0)
		return(_N);
	else if (strcheck(ch_class, "upper") == 0)
		return(_U);
	else if (strcheck(ch_class, "cntrl") == 0)
		return(_C);
	else if (strcheck(ch_class, "graph") == 0)
		return(_P|_U|_L|_N);
	else if (strcheck(ch_class, "print") == 0)
		return(_P|_U|_L|_N|_B);
	else if (strcheck(ch_class, "punct") == 0)
		return(_P);
	else if (strcheck(ch_class, "space") == 0)
		return(_S);
	else if (strcheck(ch_class, "xdigit") == 0)
		return(_X);
	else
		return(0);
}
/** END I18N CODE **/
