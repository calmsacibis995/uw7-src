#ident	"@(#)ksh93:src/lib/libast/port/astconf.c	1.1"
#pragma prototyped

/*
 * string interface to confstr(),pathconf(),sysconf()
 * extended to allow some features to be set
 */

static const char id[] = "\n@(#)getconf (AT&T Bell Laboratories) 07/17/95\0\n";

static const char lib[] = "";

#define ID		"getconf"

#include <ast.h>
#include <error.h>
#include <stk.h>
#include <fs3d.h>
#include <ctype.h>

#include "conftab.h"
#include "univlib.h"

extern char	*gettxt();

#define CONF_ERROR	(CONF_USER<<0)

#define OP_fs_3d	1
#define OP_path_resolve	2
#define OP_universe	3

#define MAXVAL		16

#if MAXVAL <= UNIV_SIZE
#undef	MAXVAL
#define	MAXVAL		(UNIV_SIZE+1)
#endif

typedef struct Feature
{
	struct Feature*	next;
	const char*	name;
	char		value[MAXVAL];
	short		length;
	short		standard;
	short		op;
} Feature_t;

typedef struct
{
	Conf_t*		conf;
	const char*	name;
	short		error;
	short		flags;
	short		call;
	short		standard;
	short		section;
} Lookup_t;

static Feature_t	dynamic[] =
{
{ &dynamic[1],	"FS_3D",	"",	5, 	CONF_AST, OP_fs_3d	},
{ &dynamic[2],	"PATH_RESOLVE",	"",	12,	CONF_AST, OP_path_resolve },
{ 0,		"UNIVERSE",	"",	8,	CONF_AST, OP_universe	},
{ 0 }
};

static Feature_t*	features = dynamic;

static Ast_confdisc_t	notify;

/*
 * synthesize state for fp
 * value==0 just does lookup
 * otherwise state is set to value
 */

static char*
synthesize(register Feature_t* fp, const char* path, const char* value)
{
	register char*		s;
	register char*		d;
	register char*		v;
	register int		n;

	static char*		data;
	static char*		last;

	static const char	state[] = "_AST_FEATURES";

	if (!fp)
		return(data);
	if (!data)
	{
		n = sizeof(state) + 3 * MAXVAL;
		if (s = getenv(state))
			n += strlen(s);
		n = roundof(n, 32);
		if (!(data = newof(0, char, n, 0)))
			return(0);
		last = data + n - 1;
		strcpy(data, state);
		data += sizeof(state) - 1;
		*data++ = '=';
		if (s) strcpy(data, s);
	}
	s = (char*)fp->name;
	n = fp->length;
	d = data;
	for (;;)
	{
		while (isspace(*d)) d++;
		if (!*d) break;
		if (strneq(d, s, n) && isspace(d[n]))
		{
			if (!value)
			{
				d += n + 1;
				while (*d && !isspace(*d)) d++;
				while (isspace(*d)) d++;
				s = d;
				while (*s && !isspace(*s)) s++;
				n = s - d;
				value = (const char*)d;
				goto ok;
			}
			s = d + n + 1;
			while (*s && !isspace(*s)) s++;
			while (isspace(*s)) s++;
			v = s;
			while (*s && !isspace(*s)) s++;
			n = s - v;
			if (strneq(v, value, n))
				goto ok;
			while (isspace(*s)) s++;
			if (*s) while (*d = *s++) d++;
			else if (d != data) d--;
			break;
		}
		while (*d && !isspace(*d)) d++;
		while (isspace(*d)) d++;
		while (*d && !isspace(*d)) d++;
		while (isspace(*d)) d++;
		while (*d && !isspace(*d)) d++;
	}
	if (!value)
	{
		fp->value[0] = 0;
		return(0);
	}
	if (!value[0])
		value = "0";
	if (!path || !path[0] || path[0] == '/' && !path[1])
		path = "-";
	n += strlen(path) + strlen(value) + 3;
	if (d + n >= last)
	{
		int	c;
		int	i;

		i = d - data;
		data -= sizeof(state);
		c = n + last - data + 3 * MAXVAL;
		c = roundof(c, 32);
		if (!(data = newof(data, char, c, 0)))
			return(0);
		last = data + c - 1;
		data += sizeof(state);
		d = data + i;
	}
	s = (char*)fp->name;
	if (d != data)
		*d++ = ' ';
	while (*d = *s++) d++;
	*d++ = ' ';
	s = (char*)path;
	while (*d = *s++) d++;
	*d++ = ' ';
	s = (char*)value;
	while (*d = *s++) d++;
	setenviron(data - sizeof(state));
	n = s - (char*)value - 1;
 ok:
	if (n >= sizeof(fp->value))
		n = sizeof(fp->value) - 1;
	else if (n == 1 && (*value == '0' || *value == '-'))
		n = 0;
	strncpy(fp->value, value, n);
	fp->value[n] = 0;
	return(fp->value);
}

/*
 * initialize the value for fp
 * if command!=0 then it is checked for on $PATH
 * synthesize(fp,path,succeed) called on success
 * otherwise synthesize(fp,path,fail) called
 */

static void
initialize(register Feature_t* fp, const char* path, const char* command, const char* succeed, const char* fail)
{
	register char*	p;
	register int	ok = 1;

	if (fp->op != OP_path_resolve || !fs3d(FS3D_TEST))
	{
		if (fp->op == OP_universe)
			ok = streq(_UNIV_DEFAULT, "att");
		if (p = getenv("PATH"))
		{
			register int	r = 1;
			register char*	d = p;
			int		offset = stktell(stkstd);

			for (;;)
			{
				switch (*p++)
				{
				case 0:
					break;
				case ':':
					if (command && (fp->op != OP_universe || !ok))
					{
						if (r = p - d - 1)
						{
							sfwrite(stkstd, d, r);
							sfputc(stkstd, '/');
							sfputr(stkstd, command, 0);
							stkseek(stkstd, offset);
							if (!access(stkptr(stkstd, offset), X_OK))
							{
								ok = 1;
								if (fp->op != OP_universe)
									break;
							}
						}
						d = p;
					}
					r = 1;
					continue;
				case '/':
					if (r)
					{
						r = 0;
						if (fp->op == OP_universe)
						{
							if (strneq(p, "bin:", 4) || strneq(p, "usr/bin:", 8))
								break;
						}
						else if (fp->op == OP_path_resolve)
							if (strneq(p, "ast/bin:", 8))
								break;
					}
					if (fp->op == OP_universe)
					{
						if (strneq(p, "5bin", 4))
						{
							ok = 1;
							break;
						}
						if (strneq(p, "bsd", 3) || strneq(p, "ucb", 3))
						{
							ok = 0;
							break;
						}
					}
					continue;
				default:
					r = 0;
					continue;
				}
				break;
			}
		}
	}
	synthesize(fp, path, ok ? succeed : fail);
}

/*
 * value==0 get feature name
 * value!=0 set feature name
 * 0 returned if error or not defined; otherwise previous value
 */

static char*
feature(const char* name, const char* path, const char* value)
{
	register Feature_t*	fp;
	register int		n;

	if (value)
	{
		if (streq(value, "-") || streq(value, "0"))
			value = "";
		if (notify && !(*notify)(name, path, value))
			return(0);
	}
	for (fp = features; fp && !streq(fp->name, name); fp = fp->next);
	if (!fp)
	{
		if (!value)
			return(0);
		n = strlen(name);
		if (!(fp = newof(0, Feature_t, 1, n + 1)))
			return(0);
		fp->name = (const char*)fp + sizeof(Feature_t);
		strcpy((char*)fp->name, name);
		fp->length = n;
		fp->next = features;
		features = fp;
	}
	switch (fp->op)
	{

	case OP_fs_3d:
		fp->value[0] = fs3d(value ? value[0] ? FS3D_ON : FS3D_OFF : FS3D_TEST) ? '1' : 0;
		break;

	case OP_path_resolve:
		if (!synthesize(fp, path, value))
			initialize(fp, path, "hostinfo", "logical", "physical");
		break;

	case OP_universe:
#if _lib_universe
		if (getuniverse(fp->value) < 0)
			strcpy(fp->value, "att");
		if (value)
			setuniverse(value);
#else
#ifdef UNIV_MAX
		n = 0;
		if (value)
		{
			while (n < univ_max && !streq(value, univ_name[n])
				n++;
			if (n >= univ_max)
				return(0);
		}
#ifdef ATT_UNIV
		n = setuniverse(n + 1);
		if (!value && n > 0)
			setuniverse(n);
#else
		n = universe(value ? n + 1 : U_GET);
#endif
		if (n <= 0 || n >= univ_max)
			n = 1;
		strcpy(fp->value, univ_name[n - 1]);
#else
		if (!synthesize(fp, path, value))
			initialize(fp, path, "echo", "att", "ucb");
#endif
#endif
		break;

	default:
		synthesize(fp, path, value);
		break;

	}
	return(fp->value);
}

/*
 * binary search for name in conf[]
 */

static int
lookup(register Lookup_t* look, const char* name)
{
	register Conf_t*	mid = (Conf_t*)conf;
	register Conf_t*	lo = mid;
	register Conf_t*	hi = mid + conf_elements;
	register int		v;
	register int		c;
	const char*		oldname = name;
	const Prefix_t*		p;

	look->flags = 0;
	look->call = -1;
	look->standard = -1;
	look->section = -1;
	while (*name == '_')
		name++;
	for (p = prefix; p < &prefix[prefix_elements]; p++)
		if (strneq(name, p->name, p->length) && ((c = name[p->length] == '_') || isdigit(name[p->length]) && name[p->length + 1] == '_'))
		{
			if ((look->call = p->call) < 0)
			{
				look->flags |= CONF_MINMAX;
				look->standard = p->standard;
			}
			name += p->length + c;
			if (isdigit(name[0]) && name[1] == '_')
			{
				look->section = name[0] - '0';
				name += 2;
			}
			else look->section = 1;
			break;
		}
	look->name = name;
	c = *((unsigned char*)name);
	while (lo <= hi)
	{
		mid = lo + (hi - lo) / 2;
		if (!(v = c - *((unsigned char*)mid->name)) && !(v = strcmp(name, mid->name)))
		{
			lo = (Conf_t*)conf;
			hi = lo + conf_elements - 1;
			if (look->standard >= 0 && look->standard != mid->standard) do
			{
				if (look->standard > mid->standard)
				{
					if (mid >= hi)
						goto badstandard;
					mid++;
				}
				else if (mid <= lo)
					goto badstandard;
				else mid--;
				if (!streq(name, mid->name))
					goto badstandard;
			} while (look->standard != mid->standard);
			if (look->section >= 0 && look->section != mid->section) do
			{
				if (look->section > mid->section)
				{
					if (mid >= hi)
						goto badsection;
					mid++;
				}
				else if (mid <= lo)
					goto badsection;
				else mid--;
				if (!streq(name, mid->name))
					goto badsection;
			} while (look->section != mid->section);
			if (look->call >= 0 && look->call != mid->call)
				goto badcall;
			look->conf = mid;
			return(1);
		}
		else if (v > 0)
			lo = mid + 1;
		else hi = mid - 1;
	}
	look->error = 0;
	return(0);
 badcall:
	look->error = 1;
	return(0);
 badstandard:
	look->error = 2;
	return(0);
 badsection:
	look->error = 3;
	return(0);
}

/*
 * print value line for p
 * if !name then value prefixed by "p->name="
 * if (flags & CONF_MINMAX) then default minmax value used
 */

static char*
print(Sfio_t* sp, register Lookup_t* look, const char* name, const char* path)
{
	register Conf_t*	p = look->conf;
	register int		flags = look->flags|CONF_DEFINED;
	char*			call;
	int			offset;
	long			v;
	int			olderrno;
	char			buf[PATH_MAX];

	if (!name && p->call != CONF_confstr && (p->flags & (CONF_FEATURE|CONF_LIMIT)) && (p->flags & (CONF_LIMIT|CONF_PREFIXED)) != CONF_LIMIT)
	{
		flags |= CONF_PREFIXED;
		if (p->flags & CONF_DEFINED)
			flags |= CONF_MINMAX;
	}
	olderrno = errno;
	errno = 0;
	switch ((flags & CONF_MINMAX) && (p->flags & CONF_DEFINED) ? 0 : p->call)
	{
	case 0:
		if (p->flags & CONF_DEFINED)
			v = p->value;
		else
		{
			flags &= ~CONF_DEFINED;
			v = -1;
		}
		break;
	case CONF_confstr:
		call = "confstr";
		if (!(v = confstr(p->op, buf, sizeof(buf))))
		{
			v = -1;
			errno = EINVAL;
		}
		break;
	case CONF_pathconf:
		call = "pathconf";
		v = pathconf(path, p->op);
		break;
	case CONF_sysconf:
		call = "sysconf";
		v = sysconf(p->op);
		break;
	default:
		call = "synthesis";
		errno = EINVAL;
		v = -1;
		break;
	}
	if (v == -1)
	{
		if (!errno)
		{
			if ((p->flags & CONF_FEATURE) || !(p->flags & (CONF_LIMIT|CONF_MINMAX)))
				flags &= ~CONF_DEFINED;
		}
		else if (!(flags & CONF_PREFIXED))
		{
			if (!sp)
			{
				liberror(lib, ERROR_SYSTEM|2, gettxt(":252","%s: %s error"), p->name, call);
				return("");
			}
			flags &= ~CONF_DEFINED;
			flags |= CONF_ERROR;
		}
		else flags &= ~CONF_DEFINED;
	}
	errno = olderrno;
	if (sp) offset = -1;
	else
	{
		sp = stkstd;
		offset = stktell(sp);
	}
	if (!(flags & CONF_PREFIXED))
	{
		if (!name)
			sfprintf(sp, "%s=", p->name);
		if (flags & CONF_ERROR)
			sfprintf(sp, gettxt(":253","error"));
		else if (p->call == CONF_confstr)
			sfprintf(sp, "%s", buf);
		else if (v != -1)
			sfprintf(sp, "%ld", v);
		else if (flags & CONF_DEFINED)
			sfprintf(sp, "%lu", v);
		else sfprintf(sp, gettxt(":254","undefined"));
		if (!name)
			sfprintf(sp, "\n");
	}
	if (!name && p->call != CONF_confstr && (p->flags & (CONF_FEATURE|CONF_MINMAX)))
	{
		if (p->flags & CONF_UNDERSCORE)
			sfprintf(sp, "_");
		sfprintf(sp, "%s", prefix[p->standard].name);
		if (p->section > 1)
			sfprintf(sp, "%d", p->section);
		sfprintf(sp, "_%s=", p->name);
		if (p->flags & CONF_DEFINED)
		{
			if ((v = p->value) == -1 && ((p->flags & CONF_FEATURE) || !(p->flags & (CONF_LIMIT|CONF_MINMAX))))
				flags &= ~CONF_DEFINED;
			else flags |= CONF_DEFINED;
		}
		if (v != -1)
			sfprintf(sp, "%ld", v);
		else if (flags & CONF_DEFINED)
			sfprintf(sp, "%lu", v);
		else sfprintf(sp, "undefined");
		sfprintf(sp, "\n");
	}
	if (offset >= 0)
	{
		sfputc(sp, 0);
		stkseek(stkstd, offset);
		return(stkptr(sp, offset));
	}
	return("");
}

/*
 * value==0 gets value for name
 * value!=0 sets value for name and returns previous value
 * path==0 implies path=="/"
 *
 * settable return values are in permanent store
 * non-settable return values are on stkstd
 *
 *	if (!strcmp(astconf("PATH_RESOLVE", NiL, NiL), "logical"))
 *		our_way();
 *
 *	universe = astconf("UNIVERSE", NiL, "att");
 *	astconf("UNIVERSE", NiL, universe);
 */

char*
astconf(const char* name, const char* path, const char* value)
{
	register char*	s;
	Lookup_t	look;

	if (!path)
		path = "/";
	if (lookup(&look, name))
	{
		if (value)
		{
			liberror(lib, 2, gettxt(":255","%s: cannot set value"), name);
			errno = EINVAL;
			return("");
		}
		return(print(NiL, &look, name, path));
	}
	if (look.error)
		switch (look.error) {
			case 1:
				liberror(lib, 2, gettxt(":256","%s: invalid call prefix"), name);
				break;
			case 2:
				liberror(lib, 2, gettxt(":257","%s: invalid standard prefix"), name);
				break;
			case 3:
				liberror(lib, 2, gettxt(":258","%s: invalid section prefix"), name);
				break;
		}
	else if ((look.standard < 0 || look.standard == CONF_AST) && look.call <= 0 && look.section <= 1 && (s = feature(look.name, path, value)))
		return(s);
	else liberror(lib, 2, gettxt(":259","%s: invalid symbol"), name);
	return("");
}

/*
 * set discipline function to be called when features change
 * old discipline function returned
 */

Ast_confdisc_t
astconfdisc(Ast_confdisc_t new_notify)
{
	Ast_confdisc_t	old_notify = notify;

	notify = new_notify;
	return(old_notify);
}

/*
 * list all name=value entries on sp
 * path==0 implies path=="/"
 * flags==0 lists all values
 * flags&R_OK lists readonly values
 * flags&W_OK lists writeable values
 * flags&X_OK lists writeable values in inputable form
 */

void
astconflist(Sfio_t* sp, const char* path, int flags)
{
	char*		s;
	Feature_t*	fp;
	Lookup_t	look;
	int		olderrno;

	if (!path)
		path = "/";
	else if (access(path, F_OK))
	{
		liberror(lib, 2, gettxt(":260","%s: not found"), path);
		return;
	}
	olderrno = errno;
	look.flags = 0;
	if (!flags)
		flags = R_OK|W_OK;
	else if (flags & X_OK)
		flags = W_OK|X_OK;
	if (flags & R_OK)
		for (look.conf = (Conf_t*)conf; look.conf < (Conf_t*)&conf[conf_elements]; look.conf++)
			print(sp, &look, NiL, path);
	if (flags & W_OK)
		for (fp = features; fp; fp = fp->next)
		{
#if HUH950401 /* don't get prefix happy */
			if (fp->standard >= 0)
				sfprintf(sp, "_%s_", prefix[fp->standard].name);
#endif
			if (!*(s = feature(fp->name, path, NiL)))
				s = "0";
			if (flags & X_OK) sfprintf(sp, "%s %s - %s\n", ID, fp->name, s); 
			else sfprintf(sp, "%s=%s\n", fp->name, s);
		}
	errno = olderrno;
}
