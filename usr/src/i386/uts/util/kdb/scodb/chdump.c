#ident	"@(#)kern-i386:util/kdb/scodb/chdump.c	1.1"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1993.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	L000	scol!nadeem	23apr92
 *	- implement the "dn" command which dumps out memory as symbols,
 *	  four per line.
 *	L001	scol!nadeem	16jun92
 *	- rationalise the movement keys in change memory command.  Remove
 *	  'n' and '-'.  Make 'h' move left a word and 'l' move right a word.
 *	L002	scol!nadeem	3jul92
 *	- when using the "dn" command to dump the stack, highlight the frame
 *	  pointers displayed by the last stack backtrace command.
 *	- make <RETURN> exit the change memory command, rather than stepping
 *	  onto the next word.
 *	L003	scol!nadeem	14jul92
 *	- correct the outputting of addresses in "symbol+offset" form such that
 *	  in a restricted field the symbol is truncated rather than the
 *	  offset.  So, for example, you would now get "a_long_symbol_na+14c"
 *	  instead of the confusing "a_long_symbol_name+1".
 *	L004	scol!nadeem	23jul92
 *	- When dumping/changing memory, ensure that pointers to pointers and
 *	  pointers to functions are not displayed in structure/union format.
 *	L005	scol!nadeem	28may93
 *	- fixed uninitialised automatic variable "sw" in chstun().  Initialised
 *	  it to -1 (according to similar functionality in chmem()).
 *	L006	scol!nadeem	14jun93
 *	- implement forward and backward movement in the dump  command ("d").
 *	  This now supports the 'j' and '^D' commands for moving forwards in
 *	  memory and the 'k' and '^U' commands for moving backwards in memory.
 *	L007	scol!nadeem	29jul93
 *	- addition to L006 - modified dump command ("d") so that SPACE and
 *	  RETURN dump the next line of memory.
 *	L008	nadeem@sco.com	13dec94
 *	- support for user level scodb:
 *	- Don't allow writes to memory unless using /dev/mem with write mode
 *	  specifically enabled on startup.
 *	- generic change: optimise the way in which chdump() moves the cursor
 *	  to a particular column.  This used to be done by moving the cursor
 *	  to the start of the row ('\r') and then calling right() to move
 *	  the cursor non-destructively to the desired column.  Replace this
 *	  code with a call to goto_col() which uses the cursor motion escape
 *	  sequence to move directly to the desired column.
 */

#include	"sys/reg.h"
#include	<syms.h>
#include	"dbg.h"
#include	"sent.h"
#include	"histedit.h"
#include	"stunv.h"
#include	"val.h"

/*
*	nybbles are numbers as such:
*		byte	      21
*		short	    4321
*		long	87654321
*	VAL value of a nybble
*	CLR clear a nybble
*	BOR binary-or a nybble
*	SET set the value of a nybble (clear && bor)
*/
#define	NYB_VAL(v, n)		(((v) >> ((n) - 1)*4) & 0x0F)
#define	NYB_CLR(v, n)		((v) &= ~(0x0F << (((n) - 1)*4)))
#define	NYB_BOR(v, x, n)	((v) |= (x) << (((n) - 1)*4))
#define	NYB_SET(v, x, n)	(NYB_CLR(v, n), NYB_BOR(v, x, n))
#define	BYT_VAL(v, n)		(((v) >> ((((n) - 1)/2)*8)) & 0x0FF)

#define		ASCII_DUMP	63

#define		CHF_QUIT	1
#define		CHF_EDIT	2
#define		CHF_NXF		3
#define		CHF_LSF		4
#define		CHF_UNDOALL	5
#define		CHF_NXFX	6
#define		CHF_LSFX	7

#ifdef STDALONE
# undef		CR0
# undef		CR1
# undef		CR2
# undef		CR3
# include	<sys/termio.h>
# include	<stdio.h>
# define	getbyte		d_getbyte
# define	getshort	d_getshort
# define	getlong		d_getlong
# define	putbyte		d_putbyte
# define	putshort	d_putshort
# define	putlong		d_putlong
# define	badaddr		d_badaddr
# define	getlinear	d_getlinear
# define	validaddr	d_validaddr
# define	prst		d_prst
# define	symname		d_symname
# define	pnz		d_pnz
char *symname();
#endif

/*
*	To test out this stuff , compile this file
*	with -DSTDALONE.
*
*	Standalone support is at end of the file.
*/


/*
 *	when printing bytes, we get:
 *				 2	spaces
 *	16 * (2hex+1space) =	48	hex bytes
 *				16	ascii bytes
 *				 1	unused space in 80th column
 *				--
 *				67
 *	leaving 13 spaces for symbol+offset
 */

#define		PSL	13

int db_getbyte(), db_getshort(), db_getlong();
NOTSTATIC int (*getfuncs[])() = {
	db_getbyte,
	db_getshort,
	0,
	db_getlong,
};

int db_putbyte(), db_putshort(), db_putlong();
STATIC int (*putfuncs[])() = {
	db_putbyte,
	db_putshort,
	0,
	db_putlong,
};

#ifndef STDALONE

extern int *REGP;

/*
*	dump memory command
*/
NOTSTATIC
c_dumpmem(c, v)
	int c;
	char **v;
{
	int mode = MD_LONG;
	char *s;
	struct value va;
	extern struct cstun *stun;
	extern int nstun;

	if (v[0][1] == 'b')
		mode = MD_BYTE;
	else if (v[0][1] == 's')
		mode = MD_SHORT;
	else if (v[0][1] == 'n')				/* L000 */
		mode = MD_SYM;					/* L000 */
	
	va.va_seg = REGP[T_DS];
	if (!valuev(v + 1, &va, 1, 1)) {
		perr();
		return DB_ERROR;
	}
	/* if a struct/union and is pointer or array type: */
	if (!strcmp(v[0], "d") && dump_as_stun(&va)) {		/* L004 */
		s = 0;	/* no "only"s */
		/* no follow field */
		pstun(0, &stun[va.va_cvari.cv_index], va.va_seg, &va.va_value, 0, &s);
	}
	else
		dumpm(va.va_seg, va.va_value, mode);
	return DB_CONTINUE;
}

#endif /* STDALONE */

/*
 * Determine whether the variable specified should be dumped out as a
 * structure/union or as hex.  Called from the dump/change memory commands.
 *
 * If the variable refers to a structure and is either a pointer or an array,
 * then dump it out as a structure.  However, don't dump it out as a structure
 * if it's a pointer to a pointer, or a pointer to a function.
 *
 * Returns true if structure dumping is required else false.
 */

dump_as_stun(va)						/* L004 v */
struct value *va;
{
	int t_basic, t_derived, t_derived2;

	t_basic = BTYPE(va->va_cvari.cv_type);
	t_derived = va->va_cvari.cv_type >> N_BTSHFT;
	t_derived2 = t_derived >> N_TSHIFT;

	return (IS_STUN(t_basic)
		&& (IS_ARY(t_derived)
		    || (IS_PTR(t_derived) && !IS_PTR(t_derived2) &&
			!IS_FCN(t_derived2)))
		&& va->va_cvari.cv_index >= 0);
}								/* L004 ^ */

STATIC
dumpm(seg, off, mode)
	long seg, off;
	int mode;
{
	int cur_row, min_row, max_row, errflag;			/* L006v */
	int dir, nlines;
	char cx;
	extern int scodb_nlines;

	errflag = 0;

	/*
	 * min_row is the lowest row (numerically) that the dump has reached,
	 * max_row is the highest row that the dump has reached,
	 * cur_row is the current row of the dump.
	 */

	min_row = scodb_nlines - 1;
	max_row = 0;
	nlines = 1;

	for(;;) {
		if (min_row < 0)
			min_row = 0;
		if (max_row >= scodb_nlines)
			max_row = scodb_nlines - 1;

		cur_row = p_row();
		if (cur_row < min_row)
			min_row = cur_row;
		if (cur_row > max_row)
			max_row = cur_row;

		if (!dumpl(seg, off, mode, 0x10, &off)) {
			/*
			 * Quit if we got an error reading memory.
			 */
			while (cur_row <= max_row) {
				putchar('\n');
				cur_row++;
			}
			badaddr(seg, off);
			return;
		}

		nlines--;

		if (nlines == 0) {
			if (get_dumpcmd(&dir, &nlines) == 0) {
				/*
				 * Quit
				 */
				while (cur_row <= max_row) {
					putchar('\n');
					cur_row++;
				}
				return;
			}
		}

		if (dir < 0) {			/* move backwards */
			if (cur_row > min_row)
				up();
			else {
				insline();
				max_row++;
			}
			off -= 0x10;
		} else {			/* move forwards */
			putchar('\n');
			if (cur_row < scodb_nlines - 1)
				max_row++;
			else
				min_row--;
			off += 0x10;
		}
		putchar('\r');
	}
}

/*
 * Decode the keystroke on a dump memory command.  Return the number of
 * lines to dump and the direction.
 */

int
get_dumpcmd(p_dir, p_nlines)
int *p_dir, *p_nlines;
{
	extern int scodb_nlines;
	char cx;
	int badkey;

	do {
		badkey = 0;

		cx = getrchr();

		if (quit(cx))
			return(0);

		switch (cx) {
		case 'j':
		case ' ':					/* L007 */
		case '\012':					/* L007 */
		case '\r':
			*p_dir = 1;
			*p_nlines = 1;
			break;
		case 'k':
			*p_dir = -1;
			*p_nlines = 1;
			break;
		case '\004':			/* ^D */
			*p_dir = 1;
			*p_nlines = scodb_nlines / 2;
			break;
		case '\025':			/* ^U */
			*p_dir = -1;
			*p_nlines = scodb_nlines / 2;
			break;
		default:
			dobell();
			badkey = 1;
			break;
		}
	} while (badkey);

	return(1);
}								/* L006^ */

/*
*	dump 0x10 bytes, in
*	mode =	1byte=byte
*		2byte=short
*		4byte=long format
*
*	do not line feed.
*
*	if (ascii) then dump ascii contents
*
*	if an invalid address is found, *badoff = offending address
*/
NOTSTATIC
dumpl(seg, off, mode, mx, badoff)
	long seg;
	register long off;
	int mode, mx;
	long *badoff;
{
	char c, *s, *symname();
	long l;
	register int o;
	int do_syms = 0;					/* v L000 v */

	/*
	 * If symbol dumping was requested, then execute the code which
	 * dumps longs, but set a flag which modifies its output behaviour
	 * so as to print symbols.
	 */

	if (mode == MD_SYM) {
		do_syms = 1;
		mode = MD_LONG;
	}							/* ^ L000 ^ */

	if (!validaddr(getlinear(seg, off))) {
		*badoff = off;
		return 0;
	}
	s = symname(off, 4);
	if (*s == '&')
		++s;
	prst_sym(s, PSL);					/* L003 */
	putchar(' ');
	for (o = 0;o < mx;o += mode) {
		if (!validaddr(getlinear(seg, off+o))) {
			*badoff = off + o;
			return 0;
		}
		putchar(' ');
		(*getfuncs[mode-1])(seg, off+o, &l);
		if (do_syms) {					/* v L000 v */
			char *s;
			int restore_rendition = 0;		/* L001 v */

			if (is_frame_pointer(off+o)) {
				reverse();
				restore_rendition = 1;
			}					/* L001 ^ */

			s = (char *)symname(l, 2);
			if (s)
				printf("%s", s);
			else
				pnz(l, 8);

			if (restore_rendition)			/* L001 */
				normal();			/* L001 */

			putchar(' ');
		} else						/* ^ L000 ^ */
			pnz(l, mode*2);
	}

	if (!do_syms) {						/* L000 */
		o = (ASCII_DUMP - (PSL + 2 + (mx/mode)*(mode*2 + 1))) + 1;
		while (o--)
			putchar(' ');
		for (o = 0;o < mx;o++) {
			db_getbyte(seg, off++, &c);
			putchar(ascii(c));
		}
	}
	return 1;
}

#ifndef STDALONE

/*
*	change memory command
*	c[bsl] address...
*/
NOTSTATIC
c_chmem(c, v)
	int c;
	char **v;
{
	int mode = MD_LONG;
	struct value va;
	extern int nstun;

	if (v[0][1] == 'b')
		mode = MD_BYTE;
	else if (v[0][1] == 's')
		mode = MD_SHORT;
	
	va.va_seg = REGP[T_DS];
	if (!valuev(v + 1, &va, 1, 1)) {
		perr();
		return DB_ERROR;
	}
	if (!strcmp(v[0], "c") && dump_as_stun(&va)) {		/* L004 */
		/*
		*	we know this structure
		*/
		chstun(va.va_seg, va.va_value, va.va_cvari.cv_index);
	}
	else {
		/*
		*	just change arbitrary amounts of memory
		*/
		chmem(va.va_seg, va.va_value, 0, 0xFFFFFFFF, 0x10, mode, 0);
	}
	return DB_CONTINUE;
}

STATIC
chstun(seg, off, indx)
	long seg, off;
	int indx;
{
	extern int scodb_nlines;
	int eln, mode = -1, i, r, mxe, sw = -1;			/* L005 */
	int mx_row = 0, cur_row, wd, mi_row = scodb_nlines;
	long olv, orv, ov;
	long eoff;
	struct cstun *cs;
	struct cstel *ce, *cep;
	extern struct cstun *stun;
	extern char *stuns;
	extern int nstun;

	cs = &stun[indx];
	ce = cs->cs_cstel;
	mxe = eln = 0;
	for (;;) {
		olv = ov;
		if (eln > mxe)
			mxe = eln;
		cep = &ce[eln];
		eoff = off + cep->ce_offset;
		if (mode == 1 || mode == 2 || mode == 4)
			(*getfuncs[mode-1])(seg, eoff, &ov);
		if (sw != eln) {
			sw = eln;
			olv = orv = ov;
		}
		putchar('\r');
		clrtoeol();
		/* pcstel takes the offset of the structure */
		mode = pcstel(PI_STEL|PI_DELTA, cep, seg, off);
		if (mode == 0)
			break;
		cur_row = p_row();
		if (cur_row > mx_row)
			mx_row = cur_row;
		if (cur_row < mi_row)
			mi_row = cur_row;
		if (mode == 1 || mode == 2 || mode == 4) {
			/* back up to beginning of field */
			i = mode*2;
			wd = i;
			while (i--)
				putchar('\b');
			for (;;) {
				r = chf(seg, eoff, mode, &wd, 0, 0);
				if (r == CHF_QUIT)
					goto done;
				switch (r) {
					case CHF_EDIT:
						/* uses 2 lines */
						if (cur_row == (scodb_nlines - 2))
							--mi_row;
						else if (cur_row == (scodb_nlines - 1))
							mi_row -= 2;
						if (mx_row > (scodb_nlines - 3))
							mx_row = (scodb_nlines - 3);
						goto nxf;
					
					case CHF_NXFX:
					case CHF_NXF:
						if ((eln + 1) == cs->cs_nmel) {
							dobell();
							break;
						}
						if (cur_row == (scodb_nlines - 1))
							--mi_row;
						putchar('\n');
						if (eln++ == mxe)
							insline();
						goto nxf;
					
					case CHF_LSFX:
					case CHF_LSF:
						if (eln == 0) {
							dobell();
							break;
						}
						--eln;
						if (cur_row == 0)
							insline();
						else
							up();
						goto nxf;

					case CHF_UNDOALL:
						(*putfuncs[mode-1])(seg, eoff, orv);
						break;
				}
			}
		}
		else {
			/* non-integer type size */

			r = do_all(DM_QUIT|DM_LAST|DM_NONL, "Change [ynq]? ");
			switch (r) {
				case DM_QUIT:
					goto done;

				gnxf:
				case DM_NO:	/* ie next */
					if ((eln + 1) != cs->cs_nmel) {
						if (cur_row == (scodb_nlines - 1))
							--mi_row;
						putchar('\n');
						if (eln++ == mxe)
							insline();
					}
					break;

				case DM_LAST:
					if (eln == 0) {
						dobell();
						break;
					}
					--eln;
					if (cur_row == 0)
						insline();
					else
						up();
					goto nxf;

				case DM_YES: {
	/************************************/
	int t_basic, t_derived, md, r, i, n, nl;
	struct cvari *cv;
	extern struct btype btype[];

	if (cur_row == (scodb_nlines - 1))
		--mi_row;
	putchar('\n');
	insline();
	cv = &cep->ce_cvari;
	t_basic = BTYPE(cv->cv_type);
	t_derived = cv->cv_type >> N_BTSHFT;

	if (IS_STUN(t_basic) && (t_derived == 0 || IS_ARY(t_derived)) && cv->cv_index >= 0) {
		/* change stuns */
		r = stun[cv->cv_index].cs_size;
		if (t_derived == 0)
			n = 1;
		else
			n = mode / r;
		for (i = 0;i < n;i++) {
			nl = chstun(seg, off + cep->ce_offset + i*r, cv->cv_index);
			if (nl == 0)
				break;
			r = nl;
		}
	}
	else {
		/* array of something or other, or unknown stun */
		md = btype[t_basic].bt_sz;
		if (md == 0)	/* unknown */
			md = MD_LONG;
		r = chmem(seg,
			off + cep->ce_offset,
			off + cep->ce_offset,
			off + cep->ce_offset + mode,
			0x10,
			md,
			1);
		if (r == 0)
			goto done;
	}
	for (i = 0;i < r;i++) {
		up();
		delline();
	}
	if (mx_row + r > (scodb_nlines - 1)) {
		/*
		* Very arcane.  
		* This crap I wrote a few months ago, and the
		* line deleting was not going right, and now trying
		* to fix it I can't remember what the fuck I was doing
		* before.  The (scodb_nlines - 2) here seems to help; perhaps one of
		* these days (:-)) I'll figure this shit out.
		*/
		i = (mx_row + r) - (scodb_nlines - 2);
		mx_row -= i + 1;
		mi_row -= i;
	}
	up();
	goto gnxf;
	/***********************************/}
					break;
			}
		}
	nxf:	;
	}
done:
	for (i = p_row();i <= mx_row;i++)
		putchar('\n');
	if (mi_row < 0)
		mi_row = 0;
	return mx_row - mi_row + 1;
}

STATIC
cesz(ce)
	register struct cstel *ce;
{
	int t_basic, t_derived;
	register struct cvari *cv = &ce->ce_cvari;
	extern struct btype btype[];

	t_basic = BTYPE(cv->cv_type);
	t_derived = cv->cv_type >> N_BTSHFT;
	if (IS_PTR(t_derived))
		return sizeof(char *);
	else if (cv->cv_size)
		return cv->cv_size;
	else
		return btype[t_basic].bt_sz;
}

#endif /* STDALONE */

STATIC
chmem(seg, off, minoff, maxoff, mx, mode, doins)
	unsigned long seg, off;
	int minoff, maxoff, mx, mode, doins;
{
	extern int scodb_nlines;
	int which = 0, wd, md, i, sw = -1;
	int mxs, r;
	int mx_row = 0, cur_row, mi_row = scodb_nlines;
	unsigned long mi_off = 0xFFFFFFFF, mx_off = 0;
	long ov, olv, orv;

	md = mode * 2;
newl:	wd = md;
	putchar('\r');
	if (off < mi_off)
		mi_off = off;
	if (off > mx_off)
		mx_off = off;

	/*
	*	dump line, first:
	*/
	mxs = MIN(maxoff - off, mx);
	if (!dumpl(seg, off, mode, mxs, &off)) {
		putchar('\n');
		badaddr(seg, off);
		return 0;
	}

	goto_col((PSL + 2 + (which * (md + 1))) + 1);		/* L008 */

	cur_row = p_row();
	if (cur_row > mx_row)
		mx_row = cur_row;
	if (cur_row < mi_row)
		mi_row = cur_row;
	for (;;) {
		olv = ov;
		(*getfuncs[mode-1])(seg, off+which*mode, &ov);
		if (sw != which) {
			sw = which;
			olv = orv = ov;
		}
		r = chf(seg, off+which*mode, mode, &wd, 1, which*mode);
		if (r == CHF_QUIT)
			break;
		switch (r) {
			case CHF_EDIT:
				if (cur_row == (scodb_nlines - 2))
					--mi_row;
				else if (cur_row == (scodb_nlines - 1))
					mi_row -= 2;
				if (mx_row > (scodb_nlines - 3))
					mx_row = (scodb_nlines - 3);
				goto newl;
			
			case CHF_NXFX:
				if (off + which*mode + mxs >= maxoff) {
					dobell();
					break;
				}
				goto nxfx;

			case CHF_NXF:
				if ((which + 1)*mode == mxs) {
					if (off + mxs >= maxoff) {
						dobell();
						break;
					}
				nxfx:	if (cur_row == (scodb_nlines - 1))
						--mi_row;
					putchar('\n');
					off += mxs;
					if (off > mx_off || doins)
						insline();
					mxs = MIN(maxoff - off, mx);
					if (r != CHF_NXFX)
						which = 0;
					goto newl;
				}
				else
					++which;
				while (wd != 1) {
					--wd;
					right();
				}
				wd = md;
				right();
				right();
				break;
			
			case CHF_LSFX:
				if (off + which*mode - mxs < minoff) {
					dobell();
					break;
				}
				goto lsfx;

			case CHF_LSF:
				if (which == 0 && off == minoff) {
					dobell();
					break;
				}
				--which;
				if (which < 0) {	/* last line */
					which = (mx / mode) - 1;
					putchar('\r');
			lsfx:		off -= mx;
					if (cur_row == 0 || off < mi_off) {
						insline();
						if (mx_row < (scodb_nlines - 1))
							++mx_row;
					}
					else
						up();
					mxs = MIN(maxoff - off, mx);
					goto newl;
				}
				while (wd != md) {		/* L001 v */
					backsp();
					wd++;
				}
				for (i = 0 ; i <= md ; i++)	/* L001 ^ */
					backsp();
				break;

			case CHF_UNDOALL:
				(*putfuncs[mode-1])(seg, off, orv);
				goto newl;
		}
	}

	for (i = p_row();i <= mx_row;i++)
		putchar('\n');
	if (mi_row < 0)
		mi_row = 0;
	return mx_row - mi_row + 1;
}

/*
*	change a field (mode type) at this address (seg:off)
*/
STATIC
chf(seg, off, mode, wdp, dumpb, str)
	long seg, off;
	int *wdp, dumpb, str;
{
	unsigned char c, newv, oldv;
	int wd, md, bytn, np, i, ldch, ret = 0;
	char buf[DBIBFL];
	register char *s;
	long v, vs;

	ldch = -1;
	wd = md = mode*2;
	(*getfuncs[mode-1])(seg, off, &v);
	while (ret == 0) {
		c = getrchr();
		if (quit(c)) {
			ret = CHF_QUIT;
			break;
		}
		switch (c) {
			case '\033':
				putchar('\n');
				insline();
				getistre("Value: ", buf, 1);
				up();
				delline();
				s = buf;
				while (white(*s))
					++s;
				if (*s)
					getaddr(buf, &vs, &v);
				up();
				ret = CHF_EDIT;
				break;

			case ' ':
				if (wd != 1) {
					--wd;
					right();
				}
				break;
			
			case 'j':
				ret = CHF_NXFX;
				break;
			
			case 'k':
				ret = CHF_LSFX;
				break;
			
			case '\r':
			case '\n':				/* L001 v */
				ret = CHF_QUIT;
				break;

			case 'h':
				*wdp = wd;
				ret = CHF_LSF;
				break;

			case 'l':				/* L001 ^ */
			case '\t':
				*wdp = wd;
				ret = CHF_NXF;
				break;

			
			case '\b':
				if (wd == md)
					ret = CHF_LSF;
				else {
					++wd;
					backsp();
				}
				break;

			case '0': case '1': case '2':
			case '3': case '4': case '5':
			case '6': case '7': case '8': case '9':
				newv = c - '0';
				goto val;

			case 'a': case 'b': case 'c':
			case 'd': case 'e': case 'f':
				c = c - 'a' + 'A';
				/*FALLTHROUGH*/

			case 'A': case 'B': case 'C':
			case 'D': case 'E': case 'F':
				newv = c - 'A' + 10;
			val:
				oldv = NYB_VAL(v, wd);
				ldch = wd;
				NYB_SET(v, newv, wd);
				putchar(c);
				if (wd == 1)
					backsp();
				else
					--wd;
				if (dumpb) {
					c = BYT_VAL(v, (wd+1));
				dpb:	bytn = str + wd/2;
					np = (ASCII_DUMP + bytn) - (PSL + 2 + str*2 + str/mode + (md - wd));
					for (i = 0;i < np;i++)
						right();
					putchar(ascii(c));
					for (i = -1;i < np;i++)
						backsp();
				}
				break;
		
			case 'u':
				if (ldch == -1) {
					/* no previous mod? */
					dobell();
					break;
				}

				/*
				*	move to appropriate digit
				*/
				while (wd < ldch) {
					++wd;
					backsp();
				}
				while (wd > ldch) {
					--wd;
					right();
				}

				NYB_SET(v, oldv, wd);
				c = oldv;
				oldv = newv;
				newv = c;
				putchar(c + (c < 10 ? '0' : 'A'-10));
				backsp();
				if (dumpb) {
					c = BYT_VAL(v, wd);
					goto dpb;
				}
				break;

			case 'U':
				ret = CHF_UNDOALL;
				break;
			
			default:
				dobell();
				break;
		}
	}
	(*putfuncs[mode-1])(seg, off, v);
	return ret;
}
