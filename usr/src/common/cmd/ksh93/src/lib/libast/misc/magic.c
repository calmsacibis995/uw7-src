#ident	"@(#)ksh93:src/lib/libast/misc/magic.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * library interface to file
 *
 * the sum of the hacks {s5,v10,planix} is _____ than the parts
 */

static const char id[] = "\n@(#)magic (AT&T Bell Laboratories) 05/09/95\0\n";

static const char lib[] = "magic";

#include <ast.h>
#include <ctype.h>
#include <hash.h>
#include <modex.h>
#include <error.h>
#include <re.h>
#include <swap.h>

/*
 * 950509 not sure about vmalloc for all systems
 */

#if USE_VMALLOC

#include <vmalloc.h>

#define vmpush(v)	{ Vmalloc_t* _VM_region = Vmregion; Vmregion = v;
#define vmpop()		Vmregion = _VM_region; }

#else

#include <align.h>

#define vmfree(v,p)
#define vmnewof(v,o,t,n,x)	(t*)vmalloc(v,sizeof(t)*(n)+(x))
#define vmopen(a,b,c)		_vmopen()
#define vmpush(v)
#define vmpop()

typedef struct Vmchunk
{
	struct Vmchunk*	next;
	char		data[ALIGN_CHUNK - ALIGN_BOUND];
} Vmchunk_t;

typedef struct
{
	Vmchunk_t	base;
	Vmchunk_t*	current;
	char*		data;
	ssize_t		size;
} Vmalloc_t;

static Vmalloc_t*
_vmopen(void)
{
	Vmalloc_t*	vp;

	if (vp = newof(0, Vmalloc_t, 1, 0))
	{
		vp->current = &vp->base;
		vp->data = vp->current->data;
		vp->size = sizeof(vp->current->data);
	}
	return(vp);
}

static void
vmclose(register Vmalloc_t* vp)
{
	register Vmchunk_t*	cp;
	register Vmchunk_t*	np;

	if (vp)
	{
		np = vp->base.next;
		while (cp = np)
		{
			np = cp->next;
			free(cp);
		}
		free(vp);
	}
}

static void*
vmalloc(register Vmalloc_t* vp, size_t size)
{
	char*	p;
	size_t	n;

	if (size > vp->size)
	{
		n = (size > sizeof(vp->current->data)) ? (size - sizeof(vp->current->data)) : 0;
		if (!(vp->current->next = newof(0, Vmchunk_t, 1, n)))
			return(0);
		vp->current = vp->current->next;
		vp->data = vp->current->data;
		vp->size = n ? 0 : sizeof(vp->current->data);
	}
	p = vp->data;
	size = roundof(size, ALIGN_BOUND);
	if (size >= vp->size)
		vp->size = 0;
	else
	{
		vp->size -= size;
		vp->data += size;
	}
	return(p);
}

static char*
vmstrdup(Vmalloc_t* vp, const char* s)
{
	char*	p;
	int	n;

	n = strlen(s) + 1;
	if (p = (char*)vmalloc(vp, n))
		strcpy(p, s);
	return(p);
}

#endif

#define MAXNEST		10		/* { ... } nesting limit	*/
#define MINITEM		4		/* magic buffer rounding	*/

typedef struct				/* identifier dictionary entry	*/
{
	char*		name;		/* identifier name		*/
	int		type;		/* identifier type		*/
} Dict_t;

typedef struct Edit			/* edit substitution		*/
{
	struct Edit*	next;		/* next in list			*/
	reprogram*	from;		/* from pattern			*/
	char*		into;		/* into this			*/
	int		flags;		/* RE_* flags			*/
} Edit_t;

struct Entry;

typedef struct				/* loop info			*/
{
	struct Entry*	lab;		/* call this function		*/
	int		start;		/* start here			*/
	int		size;		/* increment by this amount	*/
	int		count;		/* dynamic loop count		*/
	int		offset;		/* dynamic offset		*/
} Loop_t;

typedef struct Entry			/* magic file entry		*/
{
	struct Entry*	next;		/* next in list			*/
	char*		expr;		/* offset expression		*/
	union
	{
	unsigned long	num;
	char*		str;
	struct Entry*	lab;
	Edit_t*		sub;
	Loop_t*		loop;
	}		value;		/* comparison value		*/
	char*		desc;		/* file description		*/
	unsigned long	offset;		/* offset in bytes		*/
	unsigned long	mask;		/* mask before compare		*/
	char		cont;		/* continuation operation	*/
	char		type;		/* datum type			*/
	char		op;		/* comparison operation		*/
	char		nest;		/* { or } nesting operation	*/
	char		swap;		/* forced swap order		*/
} Entry_t;

#define ID_NONE		0
#define ID_ASM		1
#define ID_C		2
#define ID_CPLUSPLUS	3
#define ID_FORTRAN	4
#define ID_INCL1	5
#define ID_INCL2	6
#define ID_INCL3	7
#define ID_MAM1		8
#define ID_MAM2		9
#define ID_MAM3		10
#define ID_NOTEXT	11
#define ID_YACC		12

#define ID_MAX		ID_YACC

#define INFO_atime	1
#define INFO_blocks	2
#define INFO_ctime	3
#define INFO_fstype	4
#define INFO_gid	5
#define INFO_mode	6
#define INFO_mtime	7
#define INFO_name	8
#define INFO_nlink	9
#define INFO_size	10
#define INFO_uid	11

#define _MAGIC_PRIVATE_ \
	Vmalloc_t*	region;			/* vmalloc region	*/ \
	Entry_t*	magic;			/* parsed magic table	*/ \
	Entry_t*	magiclast;		/* last entry in magic	*/ \
	char		fbuf[SF_BUFSIZE + 1];	/* file data		*/ \
	char		xbuf[SF_BUFSIZE + 1];	/* indirect file data	*/ \
	char		tbuf[2 * PATH_MAX];	/* type string		*/ \
	int		count[UCHAR_MAX + 1];	/* char frequency count	*/ \
	int		multi[UCHAR_MAX + 1];	/* muti char count	*/ \
	int		keep[MAXNEST];		/* ckmagic nest stack	*/ \
	char*		msg[MAXNEST];		/* ckmagic text stack	*/ \
	Entry_t*	ret[MAXNEST];		/* ckmagic return stack	*/ \
	char		tmp[PATH_MAX];		/* temp buffer		*/ \
	int		fbfd;			/* fbuf fd		*/ \
	int		fbsz;			/* fbuf size		*/ \
	int		fbmx;			/* fbuf max size	*/ \
	int		xbsz;			/* xbuf size		*/ \
	int		swap;			/* swap() operation	*/ \
	long		xoff;			/* xbuf offset		*/ \
	int		identifier[ID_MAX + 1];	/* Dict_t identifier	*/ \
	Hash_table_t*	idtab;			/* identifier hash	*/ \
	Hash_table_t*	infotab;		/* info keyword hash	*/

#include <magic.h>

static Dict_t		dict[] =		/* sorted dictionary	*/
{
	"TEXT",		ID_ASM,
	"attr",		ID_MAM3,
	"binary",	ID_YACC,
	"block",	ID_FORTRAN,
	"bss",		ID_ASM,
	"byte",		ID_ASM,
	"char",		ID_C,
	"class",	ID_CPLUSPLUS,
	"clr",		ID_NOTEXT,
	"comm",		ID_ASM,
	"common",	ID_FORTRAN,
	"data",		ID_ASM,
	"dimension",	ID_FORTRAN,
	"done",		ID_MAM2,
	"double",	ID_C,
	"even",		ID_ASM,
	"exec",		ID_MAM3,
	"extern",	ID_C,
	"float",	ID_C,
	"function",	ID_FORTRAN,
	"globl",	ID_ASM,
	"h",		ID_INCL3,
	"include",	ID_INCL1,
	"int",		ID_C,
	"integer",	ID_FORTRAN,
	"jmp",		ID_NOTEXT,
	"left",		ID_YACC,
	"libc",		ID_INCL2,
	"long",		ID_C,
	"make",		ID_MAM1,
	"mov",		ID_NOTEXT,
	"private",	ID_CPLUSPLUS,
	"public",	ID_CPLUSPLUS,
	"real",		ID_FORTRAN,
	"register",	ID_C,
	"right",	ID_YACC,
	"sfio",		ID_INCL2,
	"static",	ID_C,
	"stdio",	ID_INCL2,
	"struct",	ID_C,
	"subroutine",	ID_FORTRAN,
	"sys",		ID_NOTEXT,
	"term",		ID_YACC,
	"text",		ID_ASM,
	"tst",		ID_NOTEXT,
	"type",		ID_YACC,
	"typedef",	ID_C,
	"u",		ID_INCL2,
	"union",	ID_YACC,
	"void",		ID_C,
	0,		ID_NONE
};

static Dict_t		info[] =
{
	"atime",	INFO_atime,
	"blocks",	INFO_blocks,
	"ctime",	INFO_ctime,
	"fstype",	INFO_fstype,
	"gid",		INFO_gid,
	"mode",		INFO_mode,
	"mtime",	INFO_mtime,
	"name",		INFO_name,
	"nlink",	INFO_nlink,
	"size",		INFO_size,
	"uid",		INFO_uid,
	0,		0
};

/*
 * return pointer to data at offset off and size siz
 */

static char*
getdata(register Magic_t* mp, register long off, register int siz)
{
	register long	n;

	if (off + siz <= mp->fbsz) return(mp->fbuf + off);
	if (off < mp->xoff || off + siz > mp->xoff + mp->xbsz)
	{
		if (off + siz > mp->fbmx) return(0);
		n = (off / (SF_BUFSIZE / 2)) * (SF_BUFSIZE / 2);
		if (lseek(mp->fbfd, n, 0) != n) return(0);
		if ((mp->xbsz = read(mp->fbfd, mp->xbuf, sizeof(mp->xbuf) - 1)) < 0)
		{
			mp->xoff = 0;
			mp->xbsz = 0;
			return(0);
		}
		mp->xbuf[mp->xbsz] = 0;
		mp->xoff = n;
		if (off + siz > mp->xoff + mp->xbsz) return(0);
	}
	return(mp->xbuf + off - mp->xoff);
}

/*
 * @... evaluator for strexpr()
 */

static long
indirect(const char* cs, char** e, void* handle)
{
	register char*		s = (char*)cs;
	register Magic_t*	mp = (Magic_t*)handle;
	register long		n = 0;
	register char*		p;

	if (!s) liberror(lib, 2, gettxt(":230","%s in indirect expression"), *e);
	else
	{
		if (*s == '@')
		{
			n = *++s == '(' ? strexpr(s, e, indirect, mp) : strtol(s, e, 0);
			switch (*(s = *e))
			{
			case 'b':
			case 'B':
				s++;
				if (p = getdata(mp, n, 1)) n = *(unsigned char*)p;
				else s = (char*)cs;
				break;
			case 'h':
			case 'H':
				s++;
				if (p = getdata(mp, n, 2)) n = swapget(mp->swap, p, 2);
				else s = (char*)cs;
				break;
			case 'q':
			case 'Q':
				s++;
				if (p = getdata(mp, n, 8)) n = swapget(mp->swap, p, 8);
				else s = (char*)cs;
				break;
			default:
				if (isalnum(*s)) s++;
				if (p = getdata(mp, n, 4)) n = swapget(mp->swap, p, 4);
				else s = (char*)cs;
				break;
			}
		}
		*e = s;
	}
	return(n);
}

/*
 * check for magic table match in buf
 */

static char*
ckmagic(register Magic_t* mp, const char* file, char* buf, struct stat* st, unsigned long off)
{
	register Entry_t*	ep;
	register char*		p;
	register char*		b;
	register int		level = 0;
	int			call = -1;
	int			c;
	char*			t;
	char*			base = 0;
	unsigned long		num;
	unsigned long		mask;

	NoP(file);
	mp->swap = 0;
	b = mp->msg[0] = buf;
	mp->keep[0] = 0;
	for (ep = mp->magic; ep; ep = ep->next)
	{
	fun:
		if (ep->nest == '{')
		{
			if (++level >= MAXNEST)
			{
				call = -1;
				level = 0;
				mp->keep[0] = 0;
				b = mp->msg[0];
				continue;
			}
			mp->keep[level] = mp->keep[level - 1];
			mp->msg[level] = b;
		}
		switch (ep->cont)
		{
		case '#':
			if (mp->keep[level])
			{
				*b = 0;
				return(buf);
			}
			mp->swap = 0;
			b = mp->msg[0] = buf;
			break;
		case '$':
			if (mp->keep[level] && call < (MAXNEST - 1))
			{
				mp->ret[++call] = ep;
				ep = ep->value.lab;
				goto fun;
			}
			continue;
		case ':':
			ep = mp->ret[call--];
			if (ep->op == 'l') goto fun;
			continue;
		default:
			if (!mp->keep[level])
			{
				b = mp->msg[level];
				goto checknest;
			}
			break;
		}
		if (!ep->expr) num = ep->offset + off;
		else switch (ep->offset)
		{
		case 0:
			num = strexpr(ep->expr, NiL, indirect, mp) + off;
			break;
		case INFO_atime:
			num = st->st_atime;
			ep->type = 'D';
			break;
		case INFO_blocks:
			num = iblocks(st);
			ep->type = 'N';
			break;
		case INFO_ctime:
			num = st->st_ctime;
			ep->type = 'D';
			break;
		case INFO_fstype:
			p = fmtfs(st);
			ep->type = toupper(ep->type);
			break;
		case INFO_gid:
			if (ep->type == 'e' || ep->type == 'm' || ep->type == 's')
			{
				p = fmtgid(st->st_gid);
				ep->type = toupper(ep->type);
			}
			else
			{
				num = st->st_gid;
				ep->type = 'N';
			}
			break;
		case INFO_mode:
			if (ep->type == 'e' || ep->type == 'm' || ep->type == 's')
			{
				p = fmtmode(st->st_mode, 0);
				ep->type = toupper(ep->type);
			}
			else
			{
				num = modex(st->st_mode);
				ep->type = 'N';
			}
			break;
		case INFO_mtime:
			num = st->st_ctime;
			ep->type = 'D';
			break;
		case INFO_name:
			if (!base)
			{
				if (base = strrchr(file, '/')) base++;
				else base = (char*)file;
			}
			p = base;
			ep->type = toupper(ep->type);
			break;
		case INFO_nlink:
			num = st->st_nlink;
			ep->type = 'N';
			break;
		case INFO_size:
			num = st->st_size;
			ep->type = 'N';
			break;
		case INFO_uid:
			if (ep->type == 'e' || ep->type == 'm' || ep->type == 's')
			{
				p = fmtuid(st->st_uid);
				ep->type = toupper(ep->type);
			}
			else
			{
				num = st->st_uid;
				ep->type = 'N';
			}
			break;
		}
		switch (ep->type)
		{

		case 'b':
			if (!(p = getdata(mp, num, 1))) goto next;
			num = *(unsigned char*)p;
			break;

		case 'h':
			if (!(p = getdata(mp, num, 2))) goto next;
			num = swapget(ep->swap ? (~ep->swap ^ mp->swap) : mp->swap, p, 2);
			break;

		case 'd':
		case 'l':
			if (!(p = getdata(mp, num, 4))) goto next;
			num = swapget(ep->swap ? (~ep->swap ^ mp->swap) : mp->swap, p, 4);
			break;

		case 'q':
			if (!(p = getdata(mp, num, 8))) goto next;
			num = swapget(ep->swap ? (~ep->swap ^ mp->swap) : mp->swap, p, 8);
			break;

		case 'e':
			if (!(p = getdata(mp, num, 0))) goto next;
			/*FALLTHROUGH*/
		case 'E':
			if (!reexec(ep->value.sub->from, p)) goto next;
			resub(ep->value.sub->from, p, ep->value.sub->into, mp->tmp, ep->value.sub->flags);
			t = *ep->desc ? ep->desc : mp->tmp;
			if (!mp->keep[level]) mp->keep[level] = 1;
			else if (b > buf && *(b - 1) != ' ' && *t && *t != ',' && *t != '.' && *t != '\b') *b++ = ' ';
			b += sfsprintf(b, PATH_MAX - (b - buf), *ep->desc ? ep->desc : "%s", mp->tmp + (*mp->tmp == '\b'));
			goto checknest;

		case 's':
			if (!(p = getdata(mp, num, ep->mask))) goto next;
			goto checkstr;
		case 'm':
			if (!(p = getdata(mp, num, 0))) goto next;
			/*FALLTHROUGH*/
		case 'M':
		case 'S':
		checkstr:
			if (*ep->value.str == '*' && !*(ep->value.str + 1) && isprint(*p));
			else if ((ep->type == 'm' || ep->type == 'M') ? !strmatch(p, ep->value.str) : !!memcmp(p, ep->value.str, ep->mask)) goto next;
			if (!mp->keep[level]) mp->keep[level] = 1;
			else if (b > buf && *(b - 1) != ' ' && *ep->desc && *ep->desc != ',' && *ep->desc != '.' && *ep->desc != '\b') *b++ = ' ';
			for (t = p; (c = *t) >= 0 && c <= 0177 && isprint(c) && c != '\n'; t++);
			*t = 0;
			b += sfsprintf(b, PATH_MAX - (b - buf), ep->desc + (*ep->desc == '\b'), p);
			*t = c;
			goto checknest;

		}
		if (mask = ep->mask) num &= mask;
		switch (ep->op)
		{
		case '=':
		case '@':
			if (num == ep->value.num) break;
			if (ep->cont != '#') goto next;
			if (!mask) mask = ~0;
			if (ep->type == 'h')
			{
				if ((num = swapget(mp->swap = 1, p, 2) & mask) == ep->value.num)
					goto swapped;
			}
			else if (ep->type == 'l')
			{
				for (c = 1; c < 4; c++)
					if ((num = swapget(mp->swap = c, p, 4) & mask) == ep->value.num)
						goto swapped;
			}
			else if (ep->type == 'q')
			{
				for (c = 1; c < 8; c++)
					if ((num = swapget(mp->swap = c, p, 8) & mask) == ep->value.num)
						goto swapped;
			}
			goto next;

		case '!':
			if (num != ep->value.num) break;
			goto next;

		case '^':
			if (num ^ ep->value.num) break;
			goto next;

		case '>':
			if (num > ep->value.num) break;
			goto next;

		case '<':
			if (num < ep->value.num) break;
			goto next;

		case 'l':
			if (num > 0 && mp->keep[level] && call < (MAXNEST - 1))
			{
				if (!ep->value.loop->count)
				{
					ep->value.loop->count = num;
					ep->value.loop->offset = off;
					off = ep->value.loop->start;
				}
				else if (!--ep->value.loop->count)
				{
					off = ep->value.loop->offset;
					goto next;
				}
				else off += ep->value.loop->size;
				mp->ret[++call] = ep;
				ep = ep->value.loop->lab;
				goto fun;
			}
			goto next;

		case 'm':
			c = mp->swap;
			if (ckmagic(mp, file, b + (b > buf), st, num))
			{
				if (b > buf) *b = ' ';
				b += strlen(b);
			}
			mp->swap = c;
			goto next;

		}
	swapped:
		if (!mp->keep[level]) mp->keep[level] = 1;
		else if (b > buf && *(b - 1) != ' ' && *ep->desc && *ep->desc != ',' && *ep->desc != '.' && *ep->desc != '\b') *b++ = ' ';
		if (ep->type == 'd' || ep->type == 'D')
			b += sfsprintf(b, PATH_MAX - (b - buf), ep->desc + (*ep->desc == '\b'), fmttime("%?%l", (time_t)num));
		else b += sfsprintf(b, PATH_MAX - (b - buf), ep->desc + (*ep->desc == '\b'), num);
	checknest:
		if (ep->nest == '}')
		{
			if (!mp->keep[level]) b = mp->msg[level];
			if (--level < 0)
			{
				level = 0;
				mp->keep[0] = 0;
			}
		}
		continue;
	next:
		if (ep->cont == '&') mp->keep[level] = 0;
		goto checknest;
	}
	if (mp->keep[level])
	{
		*b = 0;
		return(buf);
	}
	return(0);
}

/*
 * check english language stats
 */

static int
ckenglish(register Magic_t* mp, int pun, int badpun)
{
	register char*	s;
	register int	vowl = 0;
	register int	freq = 0;
	register int	rare = 0;

	if (5 * badpun > pun)
		return(0);
	if (2 * mp->count[';'] > mp->count['E'] + mp->count['e'])
		return(0);
	if ((mp->count['>'] + mp->count['<'] + mp->count['/']) > mp->count['E'] + mp->count['e'])
		return(0);
	for (s = "aeiou"; *s; s++)
		vowl += mp->count[toupper(*s)] + mp->count[*s];
	for (s = "etaion"; *s; s++)
		freq += mp->count[toupper(*s)] + mp->count[*s];
	for (s = "vjkqxz"; *s; s++)
		rare += mp->count[toupper(*s)] + mp->count[*s];
	return(5 * vowl >= mp->fbsz - mp->count[' '] && freq >= 10 * rare);
}

/*
 * check programming language stats
 */

#define F_latin		(1<<0)
#define F_binary	(1<<1)
#define F_eascii	(1<<2)

static char*
cklang(register Magic_t* mp, const char* file, char* buf, struct stat* st)
{
	register int		c;
	register unsigned char*	b;
	register unsigned char*	e;
	register int		q;
	register char*		s;
	char*			t;
	char*			base;
	char*			suff;
	char*			t1;
	char*			t2;
	char*			t3;
	int			badpun;
	int			flags;
	int			pun;

	b = (unsigned char*)mp->fbuf;
	e = b + mp->fbsz;
	if (b[0] == '#' && b[1] == '!')
	{
		for (b += 2; b < e && isspace(*b); b++);
		for (s = (char*)b; b < e && isprint(*b); b++);
		c = *b;
		*b = 0;
		if ((st->st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)) || strmatch(s, "/*bin*/*") || !access(s, 0))
		{
			if (t = strrchr(s, '/')) s = t + 1;
			if (strmatch(s, "*sh"))
			{
				t1 = "command";
				if (streq(s, "sh")) *s = 0;
				else
				{
					*b++ = ' ';
					*b = 0;
				}
			}
			else
			{
				t1 = "interpreter";
				*b++ = ' ';
				*b = 0;
			}
			sfsprintf(buf, PATH_MAX, "%s%s script", s, t1);
			return(buf);
		}
		*b = c;
		b = (unsigned char*)mp->fbuf;
	}
	memzero(mp->count, sizeof(mp->count));
	memzero(mp->multi, sizeof(mp->multi));
	memzero(mp->identifier, sizeof(mp->identifier));
	badpun = 0;
	flags = 0;
	pun = 0;
	q = 0;
	s = 0;
	t = 0;
	while (b < e)
	{
		c = *b++;
		if (c > 128 + 32)
		{
			flags |= F_binary;
			while (b < e)
				mp->count[*b++]++;
			break;
		}
		if (c >= 128)
		{
			flags |= F_latin;
			q = UCHAR_MAX + 1;
			continue;
		}
		if (!isprint(c) && !isspace(c) && c != '\007')
		{
			flags |= F_eascii;
			q = UCHAR_MAX + 1;
			continue;
		}
		mp->count[c]++;
		if (c == q && (q != '*' || *b == '/' && b++))
		{
			mp->multi[q]++;
			q = 0;
		}
		else if (c == '\\')
		{
			s = 0;
			b++;
		}
		else if (!q)
		{
			if (isalpha(c) || c == '_')
			{
				if (!s) s = (char*)b - 1;
			}
			else if (!isdigit(c))
			{
				if (s)
				{
					if (s > mp->fbuf) switch (*(s - 1))
					{
					case ':':
						if (*b == ':')
							mp->multi[':']++;
						break;
					case '.':
						if (((char*)b - s) == 3 && (s == (mp->fbuf + 1) || *(s - 2) == '\n'))
							mp->multi['.']++;
						break;
					case '\n':
					case '\\':
						if (*b == '{')
							t = (char*)b + 1;
						break;
					case '{':
						if (s == t && *b == '}')
							mp->multi['X']++;
						break;
					}
					if (!mp->idtab)
					{
						vmpush(mp->region);
						if (!(mp->idtab = hashalloc(NiL, HASH_name, "identifiers", 0)))
							liberror(lib, 3, gettxt(":231","out of space"));
						for (q = 0; dict[q].name; q++)
							hashput(mp->idtab, dict[q].name, (void*)dict[q].type);
						vmpop();
						q = 0;
					}
					*(b - 1) = 0;
					mp->identifier[(int)hashget(mp->idtab, s)]++;
					*(b - 1) = c;
					s = 0;
				}
				switch (c)
				{
				case '\t':
					if (b == (unsigned char*)(mp->fbuf + 1) || *(b - 2) == '\n')
						mp->multi['\t']++;
					break;
				case '"':
				case '\'':
					q = c;
					break;
				case '/':
					if (*b == '*') q = *b++;
					else if (*b == '/') q = '\n';
					break;
				case '$':
					if (*b == '(' && *(b + 1) != ' ')
						mp->multi['$']++;
					break;
				case '{':
				case '}':
				case '[':
				case ']':
				case '(':
					mp->multi[c]++;
					break;
				case ')':
					mp->multi[c]++;
					goto punctuation;
				case ':':
					if (*b == ':' && isspace(*(b + 1)) && b > (unsigned char*)(mp->fbuf + 1) && isspace(*(b - 2)))
						mp->multi[':']++;
					goto punctuation;
				case '.':
				case ',':
				case '%':
				case ';':
				case '?':
				punctuation:
					pun++;
					if (*b != ' ' && *b != '\n')
						badpun++;
					break;
				}
			}
		}
	}
	base = (t1 = strrchr(file, '/')) ? t1 + 1 : (char*)file;
	suff = (t1 = strrchr(base, '.')) ? t1 + 1 : "";
	if (!flags)
	{
		if (st->st_mode & (S_IXUSR|S_IXGRP|S_IXOTH))
			return("command script");
		if (strmatch(mp->fbuf, "From * [0-9][0-9]:[0-9][0-9]:[0-9][0-9] *"))
			return("mail message");
		if (strmatch(base, "*([Mm]kfile)"))
			return("mkfile");
		if (strmatch(base, "*@([Mm]akefile|.mk)") || mp->multi['\t'] >= mp->count[':'] && (mp->multi['$'] > 0 || mp->multi[':'] > 0))
			return("makefile");
		if (mp->multi['.'] >= 3)
			return("nroff input");
		if (mp->multi['X'] >= 3)
			return("TeX input");
		if (mp->fbsz < SF_BUFSIZE &&
		    (mp->multi['('] == mp->multi[')'] &&
		     mp->multi['{'] == mp->multi['}'] &&
		     mp->multi['['] == mp->multi[']']) ||
		    mp->fbsz >= SF_BUFSIZE &&
		    (mp->multi['('] >= mp->multi[')'] &&
		     mp->multi['{'] >= mp->multi['}'] &&
		     mp->multi['['] >= mp->multi[']']))
		{
			c = mp->identifier[ID_INCL1];
			if (c >= 2 && mp->identifier[ID_INCL2] >= c && mp->identifier[ID_INCL3] >= c && mp->count['.'] >= c ||
			    mp->identifier[ID_C] >= 5 && mp->count[';'] >= 5 ||
			    mp->count['='] >= 20 && mp->count[';'] >= 20 ||
			    strmatch(suff, "[cClLyY]"))
			{
				t1 = "";
				t2 = "c ";
				t3 = "program";
				switch (*suff)
				{
				case 'c':
				case 'C':
					break;
				case 'l':
				case 'L':
					t1 = "lex ";
					break;
				case 'y':
				case 'Y':
					t1 = "yacc ";
					break;
				default:
					t3 = "header";
					if (mp->identifier[ID_YACC] >= 5 && mp->count['%'] >= 5)
						t1 = "yacc ";
					break;
				}
				if (mp->identifier[ID_CPLUSPLUS] >= 3)
					t2 = "c++ ";
				sfsprintf(buf, PATH_MAX, "%s%s%s", t1, t2, t3);
				return(buf);
			}
		}
		if (mp->identifier[ID_MAM1] >= 2 && mp->identifier[ID_MAM3] >= 2 &&
		    (mp->fbsz < SF_BUFSIZE && mp->identifier[ID_MAM1] == mp->identifier[ID_MAM2] ||
		     mp->fbsz >= SF_BUFSIZE && mp->identifier[ID_MAM1] >= mp->identifier[ID_MAM2]))
			return("mam program");
		if (mp->identifier[ID_FORTRAN] >= 8 || mp->identifier[ID_FORTRAN] > 0 && (*suff == 'f' || *suff == 'F'))
			return("fortran program");
		if (mp->identifier[ID_ASM] >= 4 || mp->identifier[ID_ASM] > 0 && (*suff == 's' || *suff == 'S'))
			return("as program");
		if (ckenglish(mp, pun, badpun))
			return("english text");
	}
	else
	{
		if (streq(base, "core"))
			return("core dump");
	}
	if (flags & F_binary)
	{
		unsigned long	d = 0;

		if ((q = mp->fbsz / UCHAR_MAX) >= 2)
		{
			/*
			 * compression/encryption via standard deviation
			 */


			for (c = 0; c < UCHAR_MAX; c++)
			{
				pun = mp->count[c] - q;
				d += pun * pun;
			}
			d /= mp->fbsz;
		}
		if (d <= 0)
			s = "binary";
		else if (d < 4)
			s = "encrypted";
		else if (d < 16)
			s = "packed";
		else if (d < 64)
			s = "compressed";
		else if (d < 256)
			s = "delta";
		else s = "data";
		return(s);
	}
	if (flags & F_latin)
		return("latin");
	if (flags & F_eascii)
		return("extended ascii");
	return("ascii");
}

/*
 * return the basic magic string for file,st in buf,size
 */

static char*
type(register Magic_t* mp, const char* file, struct stat* st, char* buf, int size)
{
	char*	s;

	if (!S_ISREG(st->st_mode))
	{
		if (S_ISDIR(st->st_mode))
			return("directory");
		if (S_ISLNK(st->st_mode))
		{
			s = buf;
			s += sfsprintf(s, PATH_MAX, "symbolic link to ");
			if (pathgetlink(file, s, size - (s - buf)) < 0)
				return("cannot read symbolic link text");
			return(buf);
		}
		if (S_ISBLK(st->st_mode))
		{
			sfsprintf(buf, PATH_MAX, "block special (%s)", fmtdev(st));
			return(buf);
		}
		if (S_ISCHR(st->st_mode))
		{
			sfsprintf(buf, PATH_MAX, "character special (%s)", fmtdev(st));
			return(buf);
		}
		if (S_ISFIFO(st->st_mode))
			return("fifo");
#ifdef S_ISSOCK
		if (S_ISSOCK(st->st_mode))
			return("socket");
#endif
	}
	if (!(mp->fbmx = st->st_size))
		s = "empty";
	else if ((mp->fbfd = open(file, 0)) < 0)
		s = "cannot read";
	else
	{
		mp->fbsz = read(mp->fbfd, mp->fbuf, sizeof(mp->fbuf) - 1);
		if (mp->fbsz < 0)
			s = fmterror(errno);
		else if (mp->fbsz == 0)
			s = "empty";
		else
		{
			mp->fbuf[mp->fbsz] = 0;
			mp->xoff = 0;
			mp->xbsz = 0;
			if (!(s = ckmagic(mp, file, buf, st, 0)))
				s = cklang(mp, file, buf, st);
		}
		close(mp->fbfd);
	}
	return(s);
}

/*
 * load a magic file into mp
 */

int
magicload(register Magic_t* mp, const char* file, unsigned long flags)
{
	register Entry_t*	ep;
	register Sfio_t*	fp;
	register char*		p;
	int			n;
	int			lge;
	int			lev;
	int			ent;
	int			old;
	Entry_t*		ret;
	Entry_t*		first;
	Entry_t*		last = 0;
	Entry_t*		fun['z' - 'a' + 1];

	flags |= mp->flags;
	if ((!(p = (char*)file) || !*p || *p == '-' && !*(p + 1)) && (!(p = getenv("MAGICFILE")) || !*p) && (!(p = pathpath(mp->fbuf, MAGIC_FILE, "", PATH_REGULAR|PATH_READ)) || !(p = vmstrdup(mp->region, mp->fbuf))))
	{
		liberror(lib, 3, gettxt(":232","%s: cannot find magic file"), MAGIC_FILE);
		return(-1);
	}
	if (!(fp = sfopen(NiL, p, "r")))
	{
		liberror(lib, ERROR_SYSTEM|3, gettxt(":233","%s: cannot open magic file"), p);
		return(-1);
	}
	memzero(fun, sizeof(fun));
	ent = 0;
	lev = 0;
	old = 0;
	ret = 0;
	error_info.file = p;
	error_info.line = 0;
	first = ep = vmnewof(mp->region, 0, Entry_t, 1, 0);
	while (p = sfgetr(fp, '\n', 1))
	{
		register char*	p2;
		char*		next;

		error_info.line++;
		while (isspace(*p)) p++;

		/*
		 * nesting
		 */

		switch (*p)
		{
		case 0:
		case '#':
			continue;
		case '{':
			if (++lev < MAXNEST)
				ep->nest = *p;
			else if (flags & MAGIC_VERBOSE)
				liberror(lib, 1, gettxt(":234","{ ... } operator nesting too deep -- %d max"), MAXNEST);
			continue;
		case '}':
			if (!last || lev <= 0)
				liberror(lib, 2, gettxt(":235","`%c': invalid nesting"), *p);
			else if (lev-- == ent)
			{
				ent = 0;
				ep->cont = ':';
				ep->offset = ret->offset;
				ep->nest = ' ';
				ep->type = ' ';
				ep->op = ' ';
				ep->desc = "[RETURN]";
				last = ep;
				ep = ret->next = vmnewof(mp->region, 0, Entry_t, 1, 0);
				ret = 0;
			}
			else last->nest = *p;
			continue;
		default:
			if (*(p + 1) == '{' || *(p + 1) == '(' && *p != '+' && *p != '>' && *p != '&')
			{
				n = *p++;
				if (n >= 'a' && n <= 'z') n -= 'a';
				else
				{
					liberror(lib, 2, gettxt(":236","%c: invalid function name"), n);
					n = 0;
				}
				if (ret) liberror(lib, 2, gettxt(":237","%c: function has no return"), ret->offset + 'a');
				if (*p == '{')
				{
					ent = ++lev;
					ret = ep;
					ep->desc = "[FUNCTION]";
				}
				else
				{
					if (*(p + 1) != ')')
						liberror(lib, 2, gettxt(":238","%c: invalid function call argument list"), n + 'a');
					ep->desc = "[CALL]";
				}
				ep->cont = '$';
				ep->offset = n;
				ep->nest = ' ';
				ep->type = ' ';
				ep->op = ' ';
				last = ep;
				ep = ep->next = vmnewof(mp->region, 0, Entry_t, 1, 0);
				if (ret) fun[n] = last->value.lab = ep;
				else if (!(last->value.lab = fun[n])) liberror(lib, 2, gettxt(":239","%c: function not defined"), n + 'a');
				continue;
			}
			if (!ep->nest) ep->nest = (lev > 0 && lev != ent) ? ('0' + lev - !!ent) : ' ';
			break;
		}

		/*
		 * continuation
		 */

		switch (*p)
		{
		case '>':
			old = 1;
			if (*(p + 1) == *p)
			{
				/*
				 * old style nesting push
				 */

				p++;
				old = 2;
				if (!lev && last)
				{
					lev = 1;
					last->nest = '{';
					if (last->cont == '>')
						last->cont = '&';
					ep->nest = '1';
				}
			}
			/*FALLTHROUGH*/
		case '+':
		case '&':
			ep->cont = *p++;
			break;
		default:
			if ((flags & MAGIC_VERBOSE) && !isalpha(*p))
				liberror(lib, 1, gettxt(":240","`%c': invalid line continuation operator"), *p);
			/*FALLTHROUGH*/
		case '*':
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			ep->cont = (lev > 0) ? '&' : '#';
			break;
		}
		switch (old)
		{
		case 1:
			old = 0;
			if (lev)
			{
				/*
				 * old style nesting pop
				 */

				lev = 0;
				if (last)
					last->nest = '}';
				ep->nest = ' ';
				if (ep->cont == '&')
					ep->cont = '#';
			}
			break;
		case 2:
			old = 1;
			break;
		}
		if (isdigit(*p))
		{
			/*
			 * absolute offset
			 */

			ep->offset = strton(p, &next, NiL, 0);
			p2 = next;
		}
		else
		{
			for (p2 = p; *p2 && !isspace(*p2); p2++);
			if (!*p2)
			{
				if (flags & MAGIC_VERBOSE)
					liberror(lib, 1, gettxt(":241","not enough fields: `%s'"), p);
				continue;
			}

			/*
			 * offset expression
			 */

			*p2++ = 0;
			ep->expr = vmstrdup(mp->region, p);
			if (isalpha(*p))
				ep->offset = (int)hashget(mp->infotab, p);
			else if (*p == '(' && ep->cont == '>')
			{
				/*
				 * convert old style indirection to @
				 */

				p = ep->expr + 1;
				for (;;)
				{
					switch (*p++)
					{
					case 0:
					case '@':
					case '(':
						break;
					case ')':
						break;
					default:
						continue;
					}
					break;
				}
				if (*--p == ')')
				{
					*p = 0;
					*ep->expr = '@';
				}
			}
		}
		while (isspace(*p2)) p2++;
		for (p = p2; *p2 && !isspace(*p2); p2++);
		if (!*p2)
		{
			if (flags & MAGIC_VERBOSE)
				liberror(lib, 1, gettxt(":241","not enough fields: `%s'"), p);
			continue;
		}
		*p2++ = 0;

		/*
		 * type
		 */

		if ((*p == 'b' || *p == 'l') && *(p + 1) == 'e')
		{
			ep->swap = ~(*p == 'l' ? 3 : 0);
			p += 2;
		}
		if (*p == 's')
		{
			if (*(p + 1) == 'h') ep->type = 'h';
			else ep->type = 's';
		}
		else ep->type = *p;
		if (p = strchr(p, '&'))
		{
			/*
			 * old style mask
			 */

			ep->mask = strton(++p, NiL, NiL, 0);
		}
		while (isspace(*p2)) p2++;
		if (ep->mask) *--p2 = '=';

		/*
		 * comparison operation
		 */

		p = p2;
		if (p2 = strchr(p, '\t')) *p2++ = 0;
		else
		{
			int	qe = 0;
			int	qn = 0;

			/*
			 * assume balanced {}[]()\\""'' field
			 */

			for (p2 = p;;)
			{
				switch (n = *p2++)
				{
				case 0:
					break;
				case '{':
					if (!qe) qe = '}';
					if (qe == '}') qn++;
					continue;
				case '(':
					if (!qe) qe = ')';
					if (qe == ')') qn++;
					continue;
				case '[':
					if (!qe) qe = ']';
					if (qe == ']') qn++;
					continue;
				case '}':
				case ')':
				case ']':
					if (qe == n && qn > 0) qn--;
					continue;
				case '"':
				case '\'':
					if (!qe) qe = n;
					else if (qe == n) qe = 0;
					continue;
				case '\\':
					if (*p2) p2++;
					continue;
				default:
					if (!qe && isspace(n))
						break;
					continue;
				}
				if (n) *(p2 - 1) = 0;
				else p2--;
				break;
			}
		}
		lge = 0;
		if (ep->type == 'e' || ep->type == 'm' || ep->type == 's') ep->op = '=';
		else
		{
			if (*p == '&')
			{
				ep->mask = strton(++p, &next, NiL, 0);
				p = next;
			}
			switch (*p)
			{
			case '=':
			case '>':
			case '<':
			case '*':
				ep->op = *p++;
				if (*p == '=')
				{
					p++;
					switch (ep->op)
					{
					case '>':
						lge = -1;
						break;
					case '<':
						lge = 1;
						break;
					}
				}
				break;
			case '!':
			case '@':
				ep->op = *p++;
				if (*p == '=') p++;
				break;
			default:
				ep->op = '=';
				if (ep->mask) ep->value.num = ep->mask;
				break;
			}
		}
		if (ep->op != 'x' && !ep->value.num)
		{
			if (ep->type == 'e')
			{
				char*	b;
				int	d;

				ep->value.sub = vmnewof(mp->region, 0, Edit_t, 1, 0);
				b = p;
				if (!(d = *p++))
				{
					liberror(lib, 3, gettxt(":242","empty substitution"));
					return(-1);
				}
				while (*p && *p != d)
					if (*p++ == '\\' && !*p++)
					{
						liberror(lib, 3, gettxt(":243","`\\' cannot terminate lhs of substitution"));
						return(-1);
					}
				if (*p != d)
				{
					liberror(lib, 3, gettxt(":244","missing `%c' delimiter for lhs of substitution"), d);
					return(-1);
				}
				*p++ = 0;
				vmpush(mp->region);
				ep->value.sub->from = recomp(b + 1, RE_EDSTYLE|RE_MATCH);
				vmpop();
				b = p;
				while (*p && *p != d)
					if (*p++ == '\\' && !*p++)
					{
						liberror(lib, 3, gettxt(":245","`\\' cannot terminate rhs of substitution"));
						return(-1);
					}
				if (*p != d)
				{
					liberror(lib, 3, gettxt(":246","missing `%c' delimiter for rhs of substitution"), d);
					return(-1);
				}
				*p++ = 0;
				ep->value.sub->into = vmstrdup(mp->region, b);
				while (*p) switch (*p++)
				{
				case 'g':
					ep->value.sub->flags |= RE_ALL;
					break;
				case 'l':
					ep->value.sub->flags |= RE_LOWER;
					break;
				case 'u':
					ep->value.sub->flags |= RE_UPPER;
					break;
				default:
					if (*p)
						liberror(lib, 3, gettxt(":247","extra characters `%s' after substitution"), p - 1);
					else
						liberror(lib, 3, gettxt(":248","extra character `%s' after substitution"), p - 1);
					return(-1);
				}
			}
			else if (ep->type == 'm')
			{
				ep->mask = stresc(p) + 1;
				ep->value.str = vmnewof(mp->region, 0, char, ep->mask + 1, 0);
				memcpy(ep->value.str, p, ep->mask);
				if ((!ep->expr || !ep->offset) && !strmatch(ep->value.str, "\\!\\(*\\)"))
					ep->value.str[ep->mask - 1] = '*';
			}
			else if (ep->type == 's')
			{
				ep->mask = stresc(p);
				ep->value.str = vmstrdup(mp->region, p);
			}
			else if (*p == '\'')
			{
				stresc(p);
				ep->value.num = *(unsigned char*)(p + 1) + lge;
			}
			else if (strmatch(p, "+([a-z])\\(*\\)"))
			{
				char*	t;

				t = p;
				ep->op = *p;
				while (*p++ != '(');
				switch (ep->op)
				{
				case 'l':
					n = *p++;
					if (n < 'a' || n > 'z')
						liberror(lib, 2, gettxt(":236","%c: invalid function name"), n);
					else if (!fun[n -= 'a'])
						liberror(lib, 2, gettxt(":239","%c: function not defined"), n + 'a');
					else
					{
						ep->value.loop = vmnewof(mp->region, 0, Loop_t, 1, 0);
						ep->value.loop->lab = fun[n];
						while (*p && *p++ != ',');
						ep->value.loop->start = strton(p, &t, NiL, 0);
						while (*t && *t++ != ',');
						ep->value.loop->size = strton(t, &t, NiL, 0);
					}
					break;
				case 'm':
					break;
				default:
					if (flags & MAGIC_VERBOSE)
						liberror(lib, 1, gettxt(":249","%-.*s: unknown function"), t, p - t);
					break;
				}
			}
			else
			{
				ep->value.num = strton(p, NiL, NiL, 0) + lge;
				if (ep->op == '@') ep->value.num = swapget(0, (char*)&ep->value.num, sizeof(ep->value.num));
			}
		}

		/*
		 * file description
		 */

		if (p2)
		{
			while (isspace(*p2)) p2++;
			stresc(p2);
			ep->desc = vmstrdup(mp->region, p2);
		}
		else ep->desc = "";

		/*
		 * get next entry
		 */

		last = ep;
		ep = ep->next = vmnewof(mp->region, 0, Entry_t, 1, 0);
	}
	sfclose(fp);
	if (last)
	{
		last->next = 0;
		if (mp->magiclast) mp->magiclast->next = first;
		else mp->magic = first;
		mp->magiclast = last;
	}
	vmfree(mp->region, ep);
	if (flags & MAGIC_VERBOSE)
	{
		if (lev < 0) liberror(lib, 1, gettxt(":250","too many } operators"));
		else if (lev > 0) liberror(lib, 1, gettxt(":251","not enough } operators"));
		if (ret) liberror(lib, 2, gettxt(":237","%c: function has no return"), ret->offset + 'a');
	}
	error_info.file = 0;
	error_info.line = 0;
	return(0);
}

/*
 * open a magic session
 */

Magic_t*
magicopen(unsigned long flags)
{
	register Magic_t*	mp;
	register int		n;
	register Vmalloc_t*	vp;

	if (!(vp = vmopen(Vmdcheap, Vmbest, 0)))
		return(0);
	if (!(mp = vmnewof(vp, 0, Magic_t, 1, 0)))
	{
		vmclose(vp);
		return(0);
	}
	mp->region = vp;
	vmpush(mp->region);
	if (!(mp->infotab = hashalloc(NiL, HASH_name, "info", 0)))
		goto bad;
	for (n = 0; info[n].name; n++)
		hashput(mp->infotab, info[n].name, (void*)info[n].type);
	vmpop();
	mp->flags = flags;
	return(mp);
 bad:
	magicclose(mp);
	return(0);
}

/*
 * close a magicopen() session
 */

void
magicclose(register Magic_t* mp)
{
#if USE_VMALLOC
	if (mp) vmclose(mp->region);
#else
	if (mp)
	{
		if (mp->infotab)
			hashfree(mp->infotab);
		if (mp->idtab)
			hashfree(mp->idtab);
		vmclose(mp->region);
	}
#endif
}

/*
 * return the magic string for file with optional stat info st
 */

char*
magictype(register Magic_t* mp, const char* file, struct stat* st)
{
	char*		s;
	struct stat	statb;

	if ((!st && (st = &statb) || !(mp->flags & MAGIC_STAT)) && ((mp->flags & MAGIC_PHYSICAL) || stat(file, st)) && lstat(file, st))
	{
		st->st_mode = 0;
		s = "cannot stat";
	}
	else
	{
		s = type(mp, file, st, mp->tbuf, sizeof(mp->tbuf));
		if (S_ISREG(st->st_mode) && (st->st_size > 0) && (st->st_size < 128))
		{
			sfsprintf(mp->tmp, sizeof(mp->tmp) - 1, "short %s", s);
			s = mp->tmp;
		}
	}
	return(s);
}

/*
 * list the magic table in mp on sp
 */

int
magiclist(register Magic_t* mp, register Sfio_t* sp)
{
	register Entry_t*	ep = mp->magic;
	register Entry_t*	rp = 0;

	sfprintf(sp, "cont\toffset\ttype\top\tmask\tvalue\tdesc\n");
	while (ep)
	{
		sfprintf(sp, "%c %c\t", ep->cont, ep->nest);
		if (ep->expr) sfprintf(sp, "%s", ep->expr);
		else sfprintf(sp, "%ld", ep->offset);
		sfprintf(sp, "\t%s%c\t%c\t%lo\t", ep->swap == ~3 ? "L" : ep->swap == ~0 ? "B" : "", ep->type, ep->op, ep->mask);
		if (ep->type != 'm' && ep->type != 's') sfprintf(sp, "%lo", ep->value.num);
		else sfputr(sp, fmtesc(ep->value.str), -1);
		sfprintf(sp, "\t%s\n", fmtesc(ep->desc));
		if (ep->cont == '$' && !ep->value.lab->mask)
		{
			rp = ep;
			ep = ep->value.lab;
		}
		else
		{
			if (ep->cont == ':')
			{
				ep = rp;
				ep->value.lab->mask = 1;
			}
			ep = ep->next;
		}
	}
	return(0);
}
