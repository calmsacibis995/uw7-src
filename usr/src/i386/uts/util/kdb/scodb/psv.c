#ident	"@(#)kern-i386:util/kdb/scodb/psv.c	1.1"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1994.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Modification History:
 *
 *	L000	scol!nadeem	22apr92
 *	- when dumping a structure, try to ensure that the value field does
 *	  not become mis-aligned if the name field happens to overflow
 *	  into it.  Prompted by the mis-alignment of u.u_exdvr_callouts field.
 *	- do not output '&' in front of function names.
 *	L001	scol!nadeem	23apr92
 *	- added new mode 2 to symname() for use by "dn" command.  If symbol
 *	  found then output symbol+offset.  If symbol not found then return
 *	  NULL.
 *	L002	scol!nadeem	12may92
 *	- added support for BTLD.  Use the sent_name() function to access
 *	  the name field of a symbol table entry rather than accessing
 *	  it directly (see sym.c).
 *	L003	scol!nadeem	31jul92
 *	- added support for static text symbols.  Make symname() take a new
 *	  option SYM_GLOBAL to its mode argument.  This tells it to convert
 *	  the address to symbolic form using global symbols only.
 *	L004	scol!hughd	06oct93
 *	- #ifdef'ed out display of bitfields in structures and unions: wrapped
 *	  into the next line, and it's hard to make sense of many binary digits
 *	L005	nadeem@sco.com	18aug94
 *	- added support for dumping out function arguments symbolically during
 *	  a stack backtrace.
 */

#include	<storclass.h>
#include	<syms.h>
#include	<ctype.h>
#include	"dbg.h"
#include	"stunv.h"
#include	"histedit.h"
#include	"sent.h"
#include	"val.h"

#define T_LONGLONG	T_ARG

NOTSTATIC struct btype btype[] = {
	{ "void",		0			},
	{ "long long",		sizeof(long long)	},
	{ "char",		sizeof(char)		},
	{ "short",		sizeof(short)		},
	{ "int",		sizeof(int)		},
	{ "long",		sizeof(long)		},
	{ "float",		sizeof(float)		},
	{ "double",		sizeof(double)		},
	{ "struct",		0			},
	{ "union",		0			},
	{ "enum",		0			},
	{ "member of enum",	sizeof(int)		},
	{ "unsigned char",	sizeof(unsigned char)	},
	{ "unsigned short",	sizeof(unsigned short)	},
	{ "unsigned int",	sizeof(unsigned int)	},
	{ "unsigned long",	sizeof(unsigned long)	},
	0
};

NOTSTATIC
pcstel(fl, ce, seg, stoff)
	int fl;
	struct cstel *ce;
	long seg, stoff;
{
	int r;
	extern struct cstun *stun;

	r = pinfo(0,
		  fl,
		  ce->ce_names,
		  ce->ce_flags,
		  ce->ce_offset,
		  ce->ce_type,
		  ce->ce_size,
		  ce->ce_index == -1 ? "???" :
		  	stun[ce->ce_index].cs_names,
		  ce->ce_dim,
		  seg, stoff);
	if (r == 0)
		return 0;
	if ((fl & PI_DELTA) == 0)
		putchar('\n');
	return r;
}

NOTSTATIC
pcvari(buf, cv)
	char *buf;
	struct cvari *cv;
{
	char *stn;
	extern struct cstun *stun;

	/* stun? name? */
	if (IS_STUN(BTYPE(cv->cv_type))) {
		if (cv->cv_index == -1)
			stn = "???";
		else
			stn = stun[cv->cv_index].cs_names;
	}
	else
		stn = 0;
	pinfo(	buf,
		0,
		cv->cv_names,
		0,
		0,
		cv->cv_type,
		cv->cv_size,
		stn,
		cv->cv_dim,
		0, 0
	);
}

STATIC
pinfo(buf, pifl, name, flags, fldoff, type, size, tag, dims, seg, stoff)
	char *buf;
	int pifl, flags, fldoff, type, size;
	char *name, *tag;
	short dims[];
	long seg, stoff;
{
	int t_basic, t_derived, out = 0;
	int sz, ll = 0, r, vloff = VLOFF, i;
	long v, v2;
	char *s, *ptyp(), *nm, *symname(), bff[DBIBFL], obuf[DBIBFL];
	char *bbuf;
	char charbuf[(80 - VLOFF) - 1];
	extern struct btype btype[];
	extern int (*getfuncs[])();

	if (!buf) {
		buf = obuf;
		++out;
	}
	bbuf = buf;	/* begin of buffer */
	t_basic = BTYPE(type);
	t_derived = type >> N_BTSHFT;
	if (pifl & PI_STEL) {
		*buf++ = '\t';			ll += 8;
		if ((flags & SNF_FIELD) == 0) {
			pnzb(&buf, fldoff, 4);	ll += 4;
			*buf++ = ' ';		ll += 1;
			*buf++ = ' ';		ll += 1;
			if (IS_PTR(t_derived))
				sz = sizeof(char *);
			else if (size)
				sz = size;
			else
				sz = btype[t_basic].bt_sz;
			pnzb(&buf, sz, 4);	ll += 4;
		}
		else {
			/* bit field - offset in bits */
			*buf++ = ' ';		ll += 1;
			*buf++ = ' ';		ll += 1;
			*buf++ = ' ';		ll += 1;
			*buf++ = ' ';		ll += 1;
			pnzb(&buf, fldoff, 4);	ll += 4;
			*buf++ = ' ';		ll += 1;
			*buf++ = ' ';		ll += 1;
			sz = -1;
		}
		*buf++ = ' ';			ll += 1;
	}
	strcpy(buf, s = btype[t_basic].bt_nm);
	buf += strlen(s);			ll += strlen(s);
	*buf++ = ' ';				ll += 1;
	if (IS_STUN(t_basic)) {
		if (tag[0]) {
			strcpy(buf, s = tag);
			buf += strlen(s);	ll += strlen(s);
			*buf++ = ' ';		ll += 1;
		}
		else {
			*buf++ = '?';		ll += 1;
			*buf++ = '?';		ll += 1;
			*buf++ = '?';		ll += 1;
			*buf++ = ' ';		ll += 1;
		}
	}
	nm = ptyp(name, t_derived, bff, sizeof bff, dims);
	strcpy(buf, nm);
	buf += strlen(nm);			ll += strlen(nm);
	if (flags & SNF_FIELD) {
		*buf++ = ':';			ll += 1;
		/* print the field length, in decimal */
		if (size > 9) {
			*buf++ = (size / 10) + '0';
						ll += 1;
			*buf++ = (size % 10) + '0';
						ll += 1;
		}
		else
			*buf++ = size + '0',	ll += 1;
	}
	if (pifl & PI_DELTA) {
		for (;ll < vloff;ll++)
			*buf++ = ' ';
	}
	if (seg) {
		if (sz == 1 || sz == 2 || sz == 4) {
			/* integral type */
			v = 0;
			r = (*getfuncs[sz-1])(seg, stoff + fldoff, &v);
			if (!r) {
				if (out)
					badaddr(seg, stoff + fldoff);
				return 0;
			}
			r = 0;					/* L000 */
			if ((pifl & PI_DELTA) == 0) {
				*buf++ = ' ';	ll++;	/* at least 1 */
				for (;ll < vloff;ll++)
					*buf++ = ' ';
				if (ll > vloff)			/* L000 */
					r = ll - vloff;		/* L000 */
			}
				
			for (ll = sz*2;ll < 8-r;ll++)		/* L000 */
				*buf++ = ' ';
			pnzb(&buf, v, sz*2);
			if (pifl & PI_PSYM) {
				if (s = symname(v, 1)) {
					*buf++ = ' ';
					*buf++ = '<';
					strcpy(buf, s);
					buf += strlen(s);
					*buf++ = '>';
				}
			}
		}
		else if ((t_basic == T_CHAR || t_basic == T_UCHAR) && (t_derived == DT_ARY)) {
			/* (char []) */

			r = MIN(sizeof charbuf, sz);
			if (!db_getmem(seg, stoff + fldoff, charbuf, r)) {
				if (out)
					badaddr(seg, stoff + fldoff);
				return 0;
			}
			if ((pifl & PI_DELTA) == 0) {
				*buf++ = ' ';	ll++;	/* at least 1 */
				for (;ll < vloff;ll++)
					*buf++ = ' ';
				for (i = 0;i < r;i++)
					*buf++ = ascii(charbuf[i]);
			}
		}
		else if (flags & SNF_FIELD) {
			/* bit field */
			/* move to nearest long */
			stoff += (fldoff / 32) * 4;
			fldoff %= 32;
			r = (*getfuncs[4-1])(seg, stoff, &v);
			if (!r) {
				if (out)
					badaddr(seg, stoff + fldoff);
				return 0;
			}
			v <<= 32 - (fldoff + size);
			v >>= 32 - size;
			if ((pifl & PI_DELTA) == 0) {
				*buf++ = ' ';	ll++;	/* at least 1 */
				for (;ll < vloff;ll++)
					*buf++ = ' ';
				for (i = (size-1)/4+1;i < 8;i++)
					*buf++ = ' ';
				pnzb(&buf, v, (size-1)/4 + 1);
#ifdef SHOWBINARY						/* L004 */
				*buf++ = ' ';
				*buf++ = '=';
				*buf++ = ' ';
				for (i = 1;i <= size;i++, ll++)
					*buf++ = (v & (1 << (size - i))) ? '1' : '0';
#endif /* SHOWBINARY */						/* L004 */
			}
		} else
		if (t_basic == T_LONGLONG) {
			v = 0;
			r = (*getfuncs[4-1])(seg, stoff + fldoff, &v);
			if (!r) {
				if (out)
					badaddr(seg, stoff + fldoff);
				return 0;
			}
			r = (*getfuncs[4-1])(seg, stoff + fldoff + 4, &v2);
			if (!r) {
				if (out)
					badaddr(seg, stoff + fldoff + 4);
				return 0;
			}
			r = 0;
			if ((pifl & PI_DELTA) == 0) {
				*buf++ = ' ';	ll++;	/* at least 1 */
				for (;ll < vloff;ll++)
					*buf++ = ' ';
				if (ll > vloff)
					r = ll - vloff;
			}
				
			pnzb(&buf, v2, 8);
			pnzb(&buf, v, 8);
		}
		r = sz;	/* always > 0 */
	}
	else
		r = 1;
	*buf = '\0';
	if (out)
		printf("%s", bbuf);
	return r;
}

char *
ptyp(nam, typ, buf, bufsz, dims)
	char *nam, *buf;
	int typ, bufsz;
	short dims[];
{
	int last = 0, i, arn = 0;
	register char *pre, *post;

	/*
	*	1:	put n
	*			set up pointers: pre, post
	*/
	post = pre = buf + bufsz/2;
	for (i = 0;i < NAMEL && (*post++ = *nam++);i++)
		;
	if (i != NAMEL)
		--post;


	/*
	*	2:
	*		P:
	*			put * at pre
	*		F:
	*			if last was P then paren
	*			put () at post
	*		A:
	*			if last was P then paren
	*			put [] at post
	*/
	while (typ) {
		switch (typ & 03) {
			case DT_PTR:
#define		P_INDIR	'*'
#define		P_LPAR	'('
#define		P_RPAR	')'
#define		P_LBRK	'['
#define		P_RBRK	']'
				*--pre = P_INDIR;
				break;
			case DT_FCN:
				if (last == DT_PTR) {
					*--pre = P_LPAR;
					*post++ = P_RPAR;
				}
				*post++ = P_LPAR;
				*post++ = P_RPAR;
				break;
			case DT_ARY:
				if (last == DT_PTR) {
					*--pre = P_LPAR;
					*post++ = P_RPAR;
				}
				*post++ = P_LBRK;
				if (dims)
					post += pn(post, (long)dims[arn++]);
				*post++ = P_RBRK;
				break;
		}
		last = typ & 03;
		typ >>= N_TSHIFT;
	}
	*post = '\0';
	return pre;
}

/*
*	make a symbol name from a vaddr
*	if (mode) then a symbol must be found, or else return NULL
*
*	modes:
*		0	return whatever
*		4	If symbol found then return the string "symbol+offset".
*			If symbol not found then return a string containing
*			the number.
*		>=1	must find a symbol
*		3	return in *isv 1/0 if we found a sym and/or
*			sym.field
*		2	If symbol found then return the string "symbol+offset".
*			If symbol not found then return NULL.
*/
NOTSTATIC
char *
symname(vaddr, mode)
	long vaddr;
	int mode;
{
	int t_basic, t_derived, i;
	int offset, sz, n;
	static char buf[DBIBFL];
	char *s = buf;
	struct sent *se, *findsym(), *findsym_global();		/* L003 */
	struct value va;
	struct cstel *ce;
	struct cstun *cs;
	extern struct cstun *stun;
	extern int nstun;

	if (mode & SYM_GLOBAL) {				/* L003 v */
		se = findsym_global(vaddr);
		mode &= ~SYM_GLOBAL;
	} else							/* L003 ^ */
		se = findsym(vaddr);
	if (se == NULL) {
		if (mode && mode != 4)	/* required to have a symbol */
			return NULL;
		else			/* no symbol is ok */
			pn(s, vaddr);
		return buf;
	}
	findvalbysent(se, &va);
	t_basic = BTYPE(va.va_cvari.cv_type);
	t_derived = va.va_cvari.cv_type >> N_BTSHFT;

	/* L000 - code removed */

	/* put the symbol's name */
	strcpy(s, sent_name(se));				/* L002 */
	while (s++[1])
		;

	offset = vaddr - se->se_vaddr;	/* sym+_offset_ */

	if (mode == 4 || mode == 2) {				/* L001 */
		/* no special information */
		goto offt;
	}

	/* add any array subscripts */
	while (IS_ARY(t_derived)) {
		t_derived >>= N_TSHIFT;
		for (i = 1;i < NDIM;i++)
			va.va_cvari.cv_dim[i - 1] = va.va_cvari.cv_dim[i];
		va.va_cvari.cv_type = t_basic|(t_derived << N_BTSHFT);
		*s++ = '[' /*]*/;
		sz = f_sizeof(0, &va.va_cvari, 0);
		n = offset / sz;
		offset -= n * sz;
		pn(s, n);
		while (s++[1])
			;
		*s++ = /*[*/ ']';
	}

	/* find structure/union field */
	if (offset && !IS_FCN(t_derived) && IS_STUN(t_basic)) {
		if (va.va_cvari.cv_index < 0) {
			/* struct unknown */
			goto offt;
		}
		cs = &stun[va.va_cvari.cv_index];
		ce = cs->cs_cstel;
		for (i = 0;i < cs->cs_nmel;i++)
			if (ce[i].ce_offset >= offset)
				break;
		if (i < cs->cs_nmel) {
			if (ce[i].ce_offset != offset)
				--i;
			*s++ = '.';
			strcpy(s, ce[i].ce_names);
			while (s++[1])
				;
			offset -= ce[i].ce_offset;
		}
	}

	/* any leftover offset */
offt:	if (offset) {
		if (mode == 3)	/* no offsets allowed */
			return 0;
		*s++ = '+';
		pn(s, offset);
		while (s++[1])
			;
	}
	*s = '\0';
	return buf;
}

/*
 * Routine to filter out undesirable symbols during symbolic dumping of
 * stack arguments.
 */

int								/* L005v */
extraneous_symbol(s)
char *s;
{
	return 0;
}
