#ident	"@(#)alint:common/formchk.c	1.18"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "p1.h"
#include "lnstuff.h"
#include "lpass2.h"
#include "xl_byte.h"

/*
** Routines to check the format strings and arguments of functions
** in the printf and scanf families.
**
** GLOBAL ENTRY POINTS:
**	fmtinit()
**	fmtcheck()
*/

static char *getfmt(FILEPOS);
static void chkprintf(STAB *, int, char *);
static void chkscanf(STAB *, int, char *);
static int fmtarg(STAB *, unsigned long, TY);
static TY mkgeneric(TY);
static TY mkungeneric(TY);


	/* Determine generic wide character type. */
#if T_wchar_t == TY_CHAR || T_wchar_t == TY_SCHAR || T_wchar_t == TY_UCHAR
#define LN_GWCHAR LN_GCHAR
#elif T_wchar_t == TY_SHORT || T_wchar_t == TY_SSHORT || T_wchar_t == TY_USHORT
#define LN_GWCHAR LN_GSHORT
#elif T_wchar_t == TY_INT || T_wchar_t == TY_SINT || T_wchar_t == TY_UINT
#define LN_GWCHAR LN_GINT
#elif T_wchar_t == TY_LONG || T_wchar_t == TY_SLONG || T_wchar_t == TY_ULONG
#define LN_GWCHAR LN_GLONG
#elif T_wchar_t == TY_LLONG || T_wchar_t == TY_SLLONG || T_wchar_t == TY_ULLONG
#define LN_GWCHAR LN_GLLONG
#else
#error "T_wchar_t does not match any of TY_{S,U,}{CHAR,SHORT,INT,LONG,LLONG}"
#endif

#define PTRTY(x)	((x) < 0) /* negative indicates "pointer to ..." */
#define PTRTO(x)	(-((x)+1))
#define UNPTR(x)	PTRTO(x)

	/*
	* For the following, we take the current state of the
	* amended ISO C standard as "portable" (-p) [Spring '96]. 
	* This includes the wide character conversions and wide
	* character format strings.  The X/Open, NCEG and our
	* extensions (such as positional arguments and hex floating)
	* are taken as "nonportable", and are not recognized with -p.
	*/

typedef struct {
	const char	*flags;   /* flags other than "-+ " (printf) */
	TY		tlist[4]; /* okay: [1]'h', [2]'l', [3]'L'/'ll' */
} FmtOK;

#define F_SCAN	0x1	/* scanf format v. printf format */
#define F_NONP	0x2	/* non '-p' v. portable */

static const struct {
	unsigned int	kind;	/* F_xxxx bits */
	const char	*clist;	/* list of similar conversions */
	FmtOK		info;
} fmtinfo[] = {
	/*
	* The F_NONP version for a conversion character MUST come
	* after the portable version to override it.
	*/
/*printf*/
	0,	"diu",	{"0.", LN_GINT, LN_GINT, LN_GLONG},
	F_NONP,	"diu",	{"'0.", LN_GINT, LN_GINT, LN_GLONG, LN_GLLONG},
	0,	"oxX",	{"0#.", LN_GINT, LN_GINT, LN_GLONG},
	F_NONP,	"oxXbB", {"0#.", LN_GINT, LN_GINT, LN_GLONG, LN_GLLONG},
	0,	"eEfgG", {"0#.", LN_DOUBLE, 0, 0, LN_LDOUBLE},
	F_NONP,	"fFgG",	{"'0#.", LN_DOUBLE, 0, 0, LN_LDOUBLE},
	F_NONP,	"aA",	{"0#.", LN_DOUBLE, 0, 0, LN_LDOUBLE},
	0,	"n",	{"", PTRTO(LN_INT), PTRTO(LN_SHORT), PTRTO(LN_LONG)},
	F_NONP,	"n",	{"", PTRTO(LN_GINT), PTRTO(LN_GSHORT), PTRTO(LN_GLONG)},
	0,	"c",	{"", LN_GINT, 0, LN_GWCHAR},
	F_NONP,	"C",	{"", LN_GWCHAR},
	0,	"s",	{".", PTRTO(LN_GCHAR), 0, PTRTO(LN_GWCHAR)},
	F_NONP,	"S",	{"", PTRTO(LN_GWCHAR)},
	0,	"p",	{"", PTRTO(LN_VOID)},
/*scanf*/
	F_SCAN,	       "din",      {0, PTRTO(LN_GINT), PTRTO(LN_GSHORT),
					PTRTO(LN_GLONG)},
	F_SCAN,	       "ouxX",     {0, PTRTO(LN_UINT), PTRTO(LN_USHORT),
					PTRTO(LN_ULONG)},
	F_SCAN|F_NONP, "diouxXnbB", {0, PTRTO(LN_GINT), PTRTO(LN_GSHORT),
					PTRTO(LN_GLONG), PTRTO(LN_GLLONG)},
	F_SCAN,	       "eEfgG",	   {0, PTRTO(LN_FLOAT), 0,
					PTRTO(LN_DOUBLE), PTRTO(LN_LDOUBLE)},
	F_SCAN|F_NONP, "aA",	   {0, PTRTO(LN_FLOAT), 0,
					PTRTO(LN_DOUBLE), PTRTO(LN_LDOUBLE)},
	F_SCAN,	       "cs[",	   {0, PTRTO(LN_GCHAR), 0, PTRTO(LN_GWCHAR)},
	F_SCAN|F_NONP, "CS",	   {0, PTRTO(LN_GWCHAR)},
	F_SCAN,	       "p",	   {0, PTRTO(LN_PTR_VOID)},
};

static const FmtOK *pftab[1 << CHAR_BIT], *sftab[1 << CHAR_BIT];


/*
** Initialize the conversion character tables for printf and scanf.
*/
void
fmtinit(void)
{
	int i, j, ch;
	LNBUG(ln_dbflag > 1, ("fmtinit"));

	for (i = 0; i < sizeof(fmtinfo) / sizeof(fmtinfo[0]); i++) {
		if ((fmtinfo[i].kind & F_NONP) && LN_FLAG('p'))
			continue; /* extension: ignore when '-p' */
		j = 0;
		ch = fmtinfo[i].clist[0];
		do {
			if (fmtinfo[i].kind & F_SCAN)
				sftab[ch] = &fmtinfo[i].info;
			else
				pftab[ch] = &fmtinfo[i].info;
		} while ((ch = fmtinfo[i].clist[++j]) != '\0');
	}
}


/*
** Check format of entry just read in (in r.l), with the definition
** provided by the symbol table entry q.
*/
void
fmtcheck(STAB *q)
{
    char *fmt;
    int arg = q->nargs; 
    ATYPE *a;
    LNBUG(ln_dbflag > 1, ("fmtcheck: %s %d %d", q->name, arg, r.l.nargs));

    /* 
    ** "function called with variable number of arguments"
    ** The number of arguments is less than the number required.
    */ 
    if (r.l.nargs < arg) {
	BWERROR(7, q->name, q->fno, q->fline, cfno, r.l.fline);
	return;
    }
    if (arg < 1)	/* should be caught in pass1 */
	return;

    /*
    ** Find the string in the string file.  If there is no string
    ** associated with the item just read in (in r), then return -
    ** no check can be made.
    */
    a = &getarg(arg-1);
    /* CONSTCOND */
    if (sizeof(a->extra.pos) != sizeof(a->extra.ty)) {
	/* reconvert the extra field since we first assumed it was a ty */
	a->extra.ty = XL_XLATE(a->extra.ty);	/* undo the translation */
	a->extra.pos = XL_XLATE(a->extra.pos);
    }
    if ((fmt = getfmt(getarg(arg-1).extra.pos)) == 0)
	return;
    if (q->decflag & LPF)
	chkprintf(q, arg, fmt);
    else chkscanf(q, arg, fmt);
}


/*
** Return a pointer to the string at offset 'a' of the current module.
*/
static char *
getfmt(FILEPOS a)
{
    static char buf[FMTSIZE];
    static char *fmt = buf;
    static char *after = &buf[FMTSIZE];
    char *s = fmt;
    int c;
    FILEPOS pos;
    LNBUG(ln_dbflag > 1, ("getfmt: %ld", a));

    /*
    ** The string file starts at a positive non-zero offset.
    ** Anything else means a string was not passed to the function.
    */
    if (a <= 0)
	return 0;

    /*
    ** Save the current file position, and seek to the desired position.
    */
    pos = ftell(stdin);
    if (fseek(stdin, curpos + lout.f1 + lout.f2 + lout.f3 + a, 0)) {
	lerror(0,"cannot fseek string tables");
	(void)fseek(stdin, pos, 0);
	return 0;
    }

    /* 
    ** Get the string.
    */
    while ((c = getc(stdin)) != EOF && c != '\0') {
	*s++ = c;
	if (s == after) {
	    size_t n = after - fmt;

	    if (fmt == buf) {
		fmt = malloc(n + n);
		if (fmt != 0)
		    memcpy(fmt, buf, n);
	    } else {
		fmt = realloc(fmt, n + n);
	    }
	    if (fmt == 0)
		return 0;
	    s = &fmt[n];
	    after = &s[n];
	}
    }
    *s = '\0';

    /* return to old position */
    (void)fseek(stdin, pos, 0);
    LNBUG(ln_dbflag > 1, ("string: %s", fmt));
    return fmt;
}


/*
** We now know we have a legal format argument.
** Format is arg, corresponding argument is ty.
*/
static int
fmtarg(STAB *q, unsigned long arg, TY ty)
{
    ATYPE a;
    ATYPE arg_atype;
    TY ty2;
    LNBUG(ln_dbflag > 1, ("fmtarg: %d %d", arg, ty));

    /*
    ** There were too many "%?" in the format string.
    */
    if (arg >= r.l.nargs) {
	/* "too few arguments for format" */
	BWERROR(16, q->name, cfno, r.l.fline);
	return -1; /* stop processing format string */
    }

    /* 
    ** Get type without qualifiers - make it a pointer if the arg is.
    ** If it's pointer to pointer to void, assign that singular
    ** type.
    */ 
    arg_atype = getarg(arg);
    ty2 = LN_TYPE(arg_atype.aty);
    if (arg_atype.dcl_mod == LN_PTR) 
	ty2 = PTRTO(promote(ty2));
    else if (arg_atype.dcl_mod == ((LN_PTR << 2) | LN_PTR) && ty2 == LN_VOID)
	ty2 = PTRTO(LN_PTR_VOID);
    else ty2 = promote(ty2); /* removes redundant signedness */

    /*
    ** The format is "generic" - make the argument generic as well.
    */
    switch (ty) {
	case LN_GCHAR:  case PTRTO(LN_GCHAR):
	case LN_GSHORT: case PTRTO(LN_GSHORT):
	case LN_GINT:   case PTRTO(LN_GINT):
	case LN_GLONG:  case PTRTO(LN_GLONG):
	case LN_GLLONG: case PTRTO(LN_GLLONG):
	    ty2 = mkgeneric(ty2);
	    break;
    }

    /*
    ** Types should be equivalent now.
    */
    if (ty == ty2)
	return 0;

    /* CONSTANTCONDITION */
    if (SZINT == SZLONG)
	if (   ((ty == PTRTO(LN_INT)) && (ty2 == PTRTO(LN_LONG)))
	    || ((ty == PTRTO(LN_LONG)) && (ty2 == PTRTO(LN_INT)))
	   )
		return 0;

    a.dcl_con = 0;
    a.dcl_vol = 0;
    a.extra.ty = 0;
    ty = mkungeneric(ty);
    if (PTRTY(ty)) {
	if (ty == PTRTO(LN_PTR_VOID)) {
	    a.aty = LN_VOID;
	    a.dcl_mod = (LN_PTR << 2) | LN_PTR;
	}
	else {
	    a.aty = UNPTR(ty);
	    a.dcl_mod = LN_PTR;
	}
    } else {
	a.aty = ty;
	a.dcl_mod = 0;
    }

    /*
    ** str1 is the type passed to the function,
    ** str2 is the type in the format string.
    */
    ptype(str1, getarg(arg), cmno);
    ptype(str2, a, cmno);

    /* "function argument ( number ) type inconsistent with format" */
    BWERROR(17, q->name, arg+1, str1, str2, cfno, r.l.fline);
    return 1; /* complained, but continue processing format string */
}

static TY
mkgeneric(TY ty)
{
    TY sty;
    LNBUG(ln_dbflag > 1, ("mkgeneric: %d", ty));

    if (PTRTY(ty))
	sty = UNPTR(ty);
    else sty = ty;

    switch (sty) {
	case LN_CHAR: case LN_UCHAR: case LN_SCHAR:
	    sty =  LN_GCHAR;
	    break;
	case LN_SHORT: case LN_USHORT: case LN_SSHORT:
	    sty =  LN_GSHORT;
	    break;
	case LN_INT: case LN_UINT: case LN_SINT:
	    sty =  LN_GINT;
	    break;
	case LN_LONG: case LN_ULONG: case LN_SLONG:
	    sty =  LN_GLONG;
	    break;
	case LN_LLONG: case LN_ULLONG: case LN_SLLONG:
	    sty =  LN_GLLONG;
	    break;
    }

    if (PTRTY(ty))
	sty = PTRTO(sty);

    LNBUG(ln_dbflag > 1, ("  returning: %d", sty));
    return sty;
}

static TY
mkungeneric(TY ty)
{
    TY sty;
    LNBUG(ln_dbflag > 1, ("mkgeneric: %d", ty));

    if (PTRTY(ty))
	sty = UNPTR(ty);
    else sty = ty;

    switch (sty) {
	case LN_GCHAR:
	    sty =  LN_CHAR;
	    break;
	case LN_GSHORT: 
	    sty =  LN_SHORT;
	    break;
	case LN_GINT:
	    sty =  LN_INT;
	    break;
	case LN_GLONG: 
	    sty =  LN_LONG;
	    break;
	case LN_GLLONG: 
	    sty =  LN_LLONG;
	    break;
    }

    if (PTRTY(ty))
	sty = PTRTO(sty);

    LNBUG(ln_dbflag > 1, ("  returning: %d", sty));
    return sty;
}


/*
** Check for valid use of printf.
*/
static void
chkprintf(STAB *q, int arg1, char *fmt)
{
	enum {
		FLG_QUOTE, FLG_MINUS, FLG_PLUS,
		FLG_SHARP, FLG_SPACE, FLAG_ZERO,
		FLG_TOTAL
	};
	static const char flagchars[FLG_TOTAL] = "'-+# 0";
	unsigned char flags[FLG_TOTAL];
	unsigned long val, argno, posarg, maxarg;
	int wasprec, size;
	const FmtOK *fp;
	char *p;
	LNBUG(ln_dbflag > 1, ("chkprintf: %s", fmt));

	argno = arg1;
	maxarg = arg1;
	posarg = 0;
	size = -1;
	memset(flags, 0, sizeof(flags));
	while ((fmt = strchr(fmt, '%')) != 0) {
		/*
		* At the next conversion specification.
		* This assumes that no byte of a multibyte character
		* string will have a byte equal to '%' unless it is
		* really such.  All other characters that make up
		* valid conversion specifications are single-byte
		* characters as well.
		*/
		if (*++fmt == '\0')
			goto badfmt;
		if (*fmt == '%') {
			fmt++;
			continue;
		}
		/*
		* Each conversion specification should match the following,
		* in which D is a sequence of one or more decimal digits,
		* | separates alternatives, and "..." means one or more
		* characters from the quoted set:
		*  %		required
		*  D$		optional--argument number; extension
		*  "'-+# 0"	optional--flags; apostrophe is an extension
		*  D|*|*D$	optional--field width; *D$ is an extension
		*  .D|.*|.*D$	optional--precision; *D$ is an extension
		*  h|l|L|ll	optional--argument size; ll is an extension
		*  <cc>		required--conversion character
		*/
		if (isdigit(*fmt)) {
			/*
			* Check for positional argument number: D$
			*/
			val = strtoul(fmt, &p, 10);
			if (*p != '$') {
				if (posarg != 0)
					goto badfmt;
				goto chk_flags;
			}
			fmt = p;
			if (LN_FLAG('p') || !val || size >= 0 && posarg == 0)
				goto badfmt;
			if ((posarg = val) > r.l.nargs
				|| (posarg += arg1) < val /* overflow */
				|| posarg > r.l.nargs)
			{
				goto badpos;
			}
		} else if (posarg != 0)
			goto badfmt; /* !LN_FLAG('p') */
	chk_flags:
		/*
		* Gather flags.
		* "Undefined behavior" combinations checked later.
		*/
		if ((p = strchr(flagchars, *fmt)) != 0) {
			do {
				val = p - flagchars;
				if (flags[val] != 0)
					goto badfmt;
				flags[val] = 1;
			} while ((p = strchr(flagchars, *++fmt)) != 0);
			flags[FLG_MINUS] = 0;
			flags[FLG_PLUS] = 0;
			flags[FLG_SPACE] = 0;
			if (flags[FLG_QUOTE] && LN_FLAG('p'))
				goto badfmt;
		}
		/*
		* Width: D|*|*D$
		*/
		if (isdigit(*fmt)) {
			val = strtoul(fmt, &p, 10);
			fmt = p;
		gotwidth:;
			if (val == ULONG_MAX)
				goto badfmt;
		} else if (*fmt == '*') {
			if (isdigit(*++fmt)) {
				if (LN_FLAG('p') || posarg == 0)
					goto badfmt;
				val = strtoul(fmt, &p, 10);
				fmt = p;
				if (*p != '$' || val == 0)
					goto badfmt;
				if ((argno = val - 1) >= r.l.nargs
					|| (argno += arg1) < val
					|| argno >= r.l.nargs)
				{
					goto badpos;
				}
			} else if (posarg != 0)
				goto badfmt; /* !LN_FLAG('p') */
			if (fmtarg(q, argno++, LN_GINT) < 0)
				return;
			if (argno > maxarg)
				maxarg = argno;
		}
		/*
		* Precision: .D|.*|.*D$
		*/
		wasprec = 0;
		if (*fmt == '.') {
			wasprec = 1;
			if (isdigit(*++fmt)) {
				if (strtoul(fmt, &p, 10) == ULONG_MAX)
					goto badfmt;
				fmt = p;
			} else if (*fmt == '*') {
				if (isdigit(*++fmt)) {
					if (LN_FLAG('p') || posarg == 0)
						goto badfmt;
					val = strtoul(fmt, &p, 10);
					fmt = p;
					if (*p != '$' || val == 0)
						goto badfmt;
					if ((argno = val - 1) >= r.l.nargs
						|| (argno += arg1) < val
						|| argno >= r.l.nargs)
					{
						goto badpos;
					}
				} else if (posarg != 0)
					goto badfmt; /* !LN_FLAG('p') */
				if (fmtarg(q, argno++, LN_GINT) < 0)
					return;
				if (argno > maxarg)
					maxarg = argno;
			}
		}
		/*
		* Size: h|l|L|ll
		*/
		switch (*fmt++) {
		default:
			fmt--;
			size = 0;
			break;
		case 'h':
			size = 1;
			break;
		case 'l':
			size = 2;
			if (*fmt != 'l')
				break;
			if (LN_FLAG('p'))
				goto badfmt;
			fmt++;
			/*FALLTHROUGH*/
		case 'L':
			size = 3;
			break;
		}
		/*
		* Check the conversion format character and size.
		* See if any "undefined behaviors" due to mismatch
		* of flags or precision.  Finally, check the type
		* of its corresponding argument.
		*/
		if ((fp = pftab[*fmt++]) == 0 || fp->tlist[size] == 0)
			goto badfmt;
		if (wasprec && strchr(fp->flags, '.') == 0)
			goto badfmt;
		val = 0;
		do {
			if (flags[val]) {
				if (strchr(fp->flags, flagchars[val]) == 0)
					goto badfmt;
				flags[val] = 0;
			}
		} while (++val != FLG_TOTAL);
		if (posarg != 0)
			argno = posarg + 1;
		if (fmtarg(q, argno++, fp->tlist[size]) < 0)
			return;
		if (argno > maxarg)
			maxarg = argno;
	}
	/* "too many arguments for format" */
	if (maxarg < r.l.nargs)
		BWERROR(14, q->name, cfno, r.l.fline);
	return;
badfmt:;
	/* "malformed format string" */
	BWERROR(15, q->name, cfno, r.l.fline);
	return;
badpos:;
	/* "missing argument for positional" */
	BWERROR(19, val, q->name, cfno, r.l.fline);
}


/*
** Check for valid use of scanf.
*/
static void
chkscanf(STAB *q, int arg1, char *fmt)
{
	unsigned long val, argno, posarg, maxarg;
	int suppress, size;
	const FmtOK *fp;
	char *p;
	LNBUG(ln_dbflag > 1, ("chkscanf: %s", fmt));

	argno = arg1;
	maxarg = arg1;
	posarg = 0;
	size = -1;
	while ((fmt = strchr(fmt, '%')) != 0) {

		suppress = 0;

		/*
		* At the next conversion specification.
		* This assumes that no byte of a multibyte character
		* string will have a byte equal to '%' unless it is
		* really such.  All other characters that make up
		* valid conversion specifications are single-byte
		* characters as well.
		*/
		if (*++fmt == '\0')
			goto badfmt;
		if (*fmt == '%') {
			fmt++;
			continue;
		}
		/*
		* Each conversion specification should match the following,
		* in which D is a sequence of one or more decimal digits,
		* | separates alternatives, and "..." means one or more
		* characters from the quoted set:
		*  %		required
		*  D$		optional--argument number; extension
		*  *		optional--suppress assignment flag
		*  h|l|L|ll	optional--argument size; ll is an extension
		*  <cc>|[...]	required--conversion character(s)
		*/
		if (isdigit(*fmt)) {
			/*
			* Check for positional argument number: D$
			*/
			val = strtoul(fmt, &p, 10);
			fmt = p;
			if (*p != '$') {
				if (posarg != 0)
					goto badfmt;
				goto gotwidth;
			}
			if (LN_FLAG('p') || !val || size >= 0 && posarg == 0)
				goto badfmt;
			if ((posarg = val) > r.l.nargs
				|| (posarg += arg1) < val /* overflow */
				|| posarg > r.l.nargs)
			{
				goto badpos;
			}
		} else if (posarg != 0)
			goto badfmt; /* !LN_FLAG('p') */
		/*
		* Note whether there's a '*'
		*/
		if (*fmt == '*') {
			suppress = 1;
			fmt++;
		}
		/*
		* Width: D
		*/
		if (isdigit(*fmt)) {
			val = strtoul(fmt, &p, 10);
			fmt = p;
		gotwidth:;
			if (val == ULONG_MAX)
				goto badfmt;
		}
		/*
		* Size: h|l|L|ll
		*/
		switch (*fmt++) {
		default:
			fmt--;
			size = 0;
			break;
		case 'h':
			size = 1;
			break;
		case 'l':
			size = 2;
			if (*fmt != 'l')
				break;
			if (LN_FLAG('p'))
				goto badfmt;
			fmt++;
			/*FALLTHROUGH*/
		case 'L':
			size = 3;
			break;
		}
		/*
		* Check the conversion format character and size.
		* Do special processing of [...] specifications.
		* And, if not suppressed, check the type of its
		* corresponding argument.
		*/
		if ((fp = sftab[*fmt++]) == 0 || fp->tlist[size] == 0)
			goto badfmt;
		if (fmt[-1] == '[') {
			if (*fmt == '^')
				fmt++;
			/*
			* An initial ']' doesn't end the specification;
			* an initial '-' is portable.
			*/
			if (*fmt == ']' || *fmt == '-')
				fmt++;
			for (;;) {
				switch (*fmt++) {
				case '\0':
					goto badfmt;
				case ']':
					break;
				case '-':
					/*
					* A '-' at list end is portable;
					* otherwise it depends on character
					* set encodings.
					*/
					if (*fmt == ']') {
						fmt++;
						break;
					}
					if (LN_FLAG('p'))
						goto badfmt;
					/*FALLTHROUGH*/
				default:
					continue;
				}
				break; /* reached ending ']' */
			}
		}
		if (!suppress) {
			if (posarg != 0)
				argno = posarg + 1;
			if (fmtarg(q, argno++, fp->tlist[size]) < 0)
				return;
			if (argno > maxarg)
				maxarg = argno;
		}
	}
	/* "too many arguments for format" */
	if (maxarg < r.l.nargs)
		BWERROR(14, q->name, cfno, r.l.fline);
	return;
badfmt:;
	/* "malformed format string" */
	BWERROR(15, q->name, cfno, r.l.fline);
	return;
badpos:;
	/* "missing argument for positional" */
	BWERROR(19, val, q->name, cfno, r.l.fline);
}
